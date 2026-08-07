#!/usr/bin/env python3
"""Unit tests for the compliance scanner.

The CI workflow has run `unittest discover -s Build/Compliance/tests` since it
was written, against a directory holding only a two-file corpus and no tests --
so the step passed by finding nothing. These are the tests it was supposed to
find, exercised against that same corpus.
"""
import os
import subprocess
import sys
import tempfile
import unittest

TESTS_DIR = os.path.dirname(os.path.abspath(__file__))
COMPLIANCE_DIR = os.path.dirname(TESTS_DIR)
SCANNER = os.path.join(COMPLIANCE_DIR, "compliance_scan.py")
CORPUS = os.path.join(TESTS_DIR, "corpus")

sys.path.insert(0, COMPLIANCE_DIR)
import compliance_scan  # noqa: E402


def run_git(args, cwd):
    return subprocess.run(["git"] + args, cwd=cwd, capture_output=True, text=True)


class ProvenanceScanTests(unittest.TestCase):
    """The corpus directories are named for what they should produce."""

    def test_clean_repo_needs_a_legal_document(self):
        # corpus/clean_repo has an asset but no Docs/, so the scan must report the
        # missing inventory rather than passing. A missing document silently
        # passing is how a provenance check becomes decorative.
        root = os.path.join(CORPUS, "clean_repo")
        violations = compliance_scan.scan_provenance(root)
        self.assertTrue(violations)
        self.assertIn("LEGAL_AND_LICENSES", violations[0])

    def test_undocumented_third_party_pack_is_reported(self):
        with tempfile.TemporaryDirectory() as tmp:
            pack = os.path.join(tmp, "Content", "ThirdParty", "SomePack")
            os.makedirs(pack)
            with open(os.path.join(pack, "mesh.uasset"), "wb") as fh:
                fh.write(b"\0")
            legal = os.path.join(tmp, "Docs", "Production")
            os.makedirs(legal)
            with open(os.path.join(legal, "LEGAL_AND_LICENSES.md"), "w") as fh:
                fh.write("# Legal\n\n| ambientCG | CC0 |\n")

            violations = compliance_scan.scan_provenance(tmp)
            self.assertEqual(len(violations), 1)
            self.assertIn("SomePack", violations[0])

    def test_documented_pack_passes(self):
        with tempfile.TemporaryDirectory() as tmp:
            pack = os.path.join(tmp, "Content", "ThirdParty", "ambientCG")
            os.makedirs(pack)
            with open(os.path.join(pack, "tex.png"), "wb") as fh:
                fh.write(b"\0")
            legal = os.path.join(tmp, "Docs", "Production")
            os.makedirs(legal)
            with open(os.path.join(legal, "LEGAL_AND_LICENSES.md"), "w") as fh:
                fh.write("| **ambientCG Materials** | ambientCG.com | CC0 1.0 |\n")

            self.assertEqual(compliance_scan.scan_provenance(tmp), [])

    def test_stub_pack_without_assets_is_not_a_violation(self):
        # Brushify/Quixel/EpicGames in the real tree are empty directory stubs.
        # Demanding a licence for a folder with no assets would produce noise that
        # trains people to ignore the scanner.
        with tempfile.TemporaryDirectory() as tmp:
            os.makedirs(os.path.join(tmp, "Content", "ThirdParty", "EmptyStub"))
            legal = os.path.join(tmp, "Docs", "Production")
            os.makedirs(legal)
            with open(os.path.join(legal, "LEGAL_AND_LICENSES.md"), "w") as fh:
                fh.write("# Legal\n")
            self.assertEqual(compliance_scan.scan_provenance(tmp), [])


    def test_repository_copies_are_not_scanned(self):
        # Git worktrees under .claude/ are full copies of the tree. Walking them
        # reported the same pack once per worktree: three worktrees turned two real
        # findings into nine lines of output. A scanner that cries wolf gets
        # switched off, so copies must not be walked at all.
        with tempfile.TemporaryDirectory() as tmp:
            legal = os.path.join(tmp, "Docs", "Production")
            os.makedirs(legal)
            with open(os.path.join(legal, "LEGAL_AND_LICENSES.md"), "w") as fh:
                fh.write("# Legal\n")

            for base in (os.path.join(tmp, "Content", "ThirdParty", "RealPack"),
                         os.path.join(tmp, ".claude", "worktrees", "wt1",
                                      "Content", "ThirdParty", "RealPack"),
                         os.path.join(tmp, "Intermediate", "Content", "ThirdParty", "RealPack")):
                os.makedirs(base)
                with open(os.path.join(base, "m.uasset"), "wb") as fh:
                    fh.write(b"\0")

            violations = compliance_scan.scan_provenance(tmp)
            # Exactly one: the real tree. The worktree copy and the build
            # intermediate must not produce their own findings.
            self.assertEqual(len(violations), 1, violations)
            self.assertNotIn(".claude", violations[0])
            self.assertNotIn("Intermediate", violations[0])


class CommitScopeScanTests(unittest.TestCase):
    """Reproduces the real failure: a commit deleting code outside its subject."""

    def _make_repo(self):
        tmp = tempfile.mkdtemp()
        run_git(["init", "-q"], tmp)
        run_git(["config", "user.email", "t@t"], tmp)
        run_git(["config", "user.name", "t"], tmp)
        sim = os.path.join(tmp, "Source", "RA4Simulation")
        os.makedirs(sim)
        # 60 lines, so deleting them clears DELETION_THRESHOLD.
        with open(os.path.join(sim, "SimWorld.cpp"), "w") as fh:
            fh.write("\n".join("line %d" % i for i in range(60)) + "\n")
        os.makedirs(os.path.join(tmp, "Scripts"))
        with open(os.path.join(tmp, "Scripts", "package.sh"), "w") as fh:
            fh.write("echo build\n")
        run_git(["add", "-A"], tmp)
        run_git(["commit", "-q", "-m", "chore: base"], tmp)
        return tmp

    def test_build_commit_deleting_simulation_code_is_reported(self):
        tmp = self._make_repo()
        base = run_git(["rev-parse", "HEAD"], tmp).stdout.strip()
        # Exactly the shape of the real incident: a build-scoped subject that also
        # empties a simulation file.
        with open(os.path.join(tmp, "Source", "RA4Simulation", "SimWorld.cpp"), "w") as fh:
            fh.write("line 0\n")
        with open(os.path.join(tmp, "Scripts", "package.sh"), "w") as fh:
            fh.write("echo build --nozenstore\n")
        run_git(["add", "-A"], tmp)
        run_git(["commit", "-q", "-m", "fix(build): cook to disk instead of ZenStore"], tmp)

        violations = compliance_scan.scan_commit_scope(tmp, base)
        self.assertTrue(violations, "a build commit deleting sim code must be flagged")
        self.assertIn("SimWorld.cpp", violations[0])
        self.assertIn("build", violations[0])

    def test_sim_commit_deleting_sim_code_is_allowed(self):
        tmp = self._make_repo()
        base = run_git(["rev-parse", "HEAD"], tmp).stdout.strip()
        with open(os.path.join(tmp, "Source", "RA4Simulation", "SimWorld.cpp"), "w") as fh:
            fh.write("line 0\n")
        run_git(["add", "-A"], tmp)
        run_git(["commit", "-q", "-m", "perf(sim): bucket target acquisition"], tmp)
        # 'sim' has no entry in SCOPE_ALLOWED_DELETIONS, so it is unjudged rather
        # than flagged: the scanner must not invent rules for scopes it does not
        # know, or every commit becomes a violation.
        self.assertEqual(compliance_scan.scan_commit_scope(tmp, base), [])

    def test_revert_may_delete_anything(self):
        tmp = self._make_repo()
        base = run_git(["rev-parse", "HEAD"], tmp).stdout.strip()
        with open(os.path.join(tmp, "Source", "RA4Simulation", "SimWorld.cpp"), "w") as fh:
            fh.write("line 0\n")
        run_git(["add", "-A"], tmp)
        run_git(["commit", "-q", "-m", "revert(build): undo the bad change"], tmp)
        self.assertEqual(compliance_scan.scan_commit_scope(tmp, base), [])

    def test_small_deletion_is_not_flagged(self):
        tmp = self._make_repo()
        base = run_git(["rev-parse", "HEAD"], tmp).stdout.strip()
        path = os.path.join(tmp, "Source", "RA4Simulation", "SimWorld.cpp")
        with open(path) as fh:
            lines = fh.readlines()
        with open(path, "w") as fh:
            fh.writelines(lines[:-5])   # 5 lines, under the threshold
        with open(os.path.join(tmp, "Scripts", "package.sh"), "w") as fh:
            fh.write("echo build2\n")
        run_git(["add", "-A"], tmp)
        run_git(["commit", "-q", "-m", "fix(build): tidy"], tmp)
        self.assertEqual(compliance_scan.scan_commit_scope(tmp, base), [])


class ScannerCliTests(unittest.TestCase):
    def test_scanner_runs_and_reports_exit_status(self):
        # The CI step is `python3 ... --scope both`; prove that invocation works,
        # since the workflow referenced a script that did not exist at all.
        result = subprocess.run(
            [sys.executable, SCANNER, "--scope", "provenance", "--root",
             os.path.join(CORPUS, "clean_repo")],
            capture_output=True, text=True)
        self.assertEqual(result.returncode, 1)
        self.assertIn("COMPLIANCE VIOLATION", result.stdout)


if __name__ == "__main__":
    unittest.main()
