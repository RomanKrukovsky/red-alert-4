import json
import pathlib
import shutil
import subprocess
import tempfile
import unittest

import sys


TESTS_DIR = pathlib.Path(__file__).resolve().parent
COMPLIANCE_DIR = TESTS_DIR.parent
REPO_POLICY_DIR = COMPLIANCE_DIR / "policy"
sys.path.insert(0, str(COMPLIANCE_DIR))

import compliance_scan as scan  # noqa: E402


class ComplianceScanTests(unittest.TestCase):
    maxDiff = None

    def make_repo(self, fixture_name: str, *, add_all: bool = True) -> pathlib.Path:
        temp_root = pathlib.Path(tempfile.mkdtemp(prefix="compliance-scan-"))
        self.addCleanup(shutil.rmtree, temp_root, ignore_errors=True)
        subprocess.run(["git", "init", "-q", str(temp_root)], check=True)

        fixture_root = TESTS_DIR / "corpus" / fixture_name
        if fixture_root.exists():
            shutil.copytree(fixture_root, temp_root, dirs_exist_ok=True)

        policy_root = temp_root / "Build" / "Compliance" / "policy"
        policy_root.parent.mkdir(parents=True, exist_ok=True)
        shutil.copytree(REPO_POLICY_DIR, policy_root, dirs_exist_ok=True)

        if add_all:
            subprocess.run(["git", "-C", str(temp_root), "add", "-A"], check=True)
        return temp_root

    def scan_repo(self, repo_root: pathlib.Path, scope: str = "both") -> list[scan.Violation]:
        return scan.scan_repository(
            repo_root=repo_root,
            scope=scope,
            policy_dir=repo_root / "Build" / "Compliance" / "policy",
        )

    def test_clean_repo_passes(self) -> None:
        repo_root = self.make_repo("clean_repo")
        violations = self.scan_repo(repo_root)
        self.assertEqual([], violations)

    def test_external_research_in_index_fails(self) -> None:
        repo_root = self.make_repo("external_research")
        violations = self.scan_repo(repo_root, scope="index")
        self.assertTrue(any(item.rule_id == "external_research_in_index" for item in violations))

    def test_forbidden_identifier_in_docs_fails(self) -> None:
        repo_root = self.make_repo("forbidden_identifier")
        violations = self.scan_repo(repo_root)
        self.assertTrue(any(item.rule_id == "command_and_conquer_identifier" for item in violations))
        self.assertTrue(any(item.rule_id == "red_alert_3_identifier" for item in violations))

    def test_xml_policy_cases_fail(self) -> None:
        repo_root = self.make_repo("xml_policy")
        violations = self.scan_repo(repo_root)
        rule_ids = {item.rule_id for item in violations}
        self.assertIn("forbidden_extension", rule_ids)
        self.assertIn("xml_doctype", rule_ids)
        self.assertIn("xml_entity", rule_ids)
        self.assertIn("xml_external_system", rule_ids)
        self.assertIn("path_traversal_reference", rule_ids)

    def test_missing_provenance_for_binary_fails(self) -> None:
        repo_root = self.make_repo("missing_provenance")
        violations = self.scan_repo(repo_root)
        self.assertTrue(any(item.rule_id == "missing_provenance" for item in violations))

    def test_valid_suppression_hides_known_violation(self) -> None:
        repo_root = self.make_repo("forbidden_identifier")
        suppressions_path = repo_root / "Build" / "Compliance" / "policy" / "suppressions.json"
        suppressions_doc = json.loads(suppressions_path.read_text(encoding="utf-8"))
        suppressions_doc["suppressions"] = [
            {
                "id": "s1",
                "rule_id": "command_and_conquer_identifier",
                "path_glob": "Docs/research/cnc-red-alert/*",
                "comment": "Approved temporary corpus fixture for unit testing.",
                "reviewer": "Compliance QA",
                "expires_on": "2099-12-31"
            }
        ]
        suppressions_path.write_text(json.dumps(suppressions_doc, indent=2) + "\n", encoding="utf-8")

        violations = self.scan_repo(repo_root)
        self.assertFalse(any(item.rule_id == "command_and_conquer_identifier" for item in violations))
        self.assertTrue(any(item.rule_id == "red_alert_3_identifier" for item in violations))

    def test_invalid_suppression_requires_comment_and_reviewer(self) -> None:
        repo_root = self.make_repo("clean_repo")
        suppressions_path = repo_root / "Build" / "Compliance" / "policy" / "suppressions.json"
        suppressions_path.write_text(
            json.dumps(
                {
                    "version": "1.0.0",
                    "suppressions": [
                        {
                            "id": "bad",
                            "rule_id": "missing_provenance",
                            "path_glob": "Assets/**",
                            "comment": "",
                            "reviewer": "",
                            "expires_on": "2099-12-31"
                        }
                    ]
                },
                indent=2,
            )
            + "\n",
            encoding="utf-8",
        )

        violations = self.scan_repo(repo_root)
        self.assertTrue(any(item.rule_id == "invalid_suppression_entry" for item in violations))


if __name__ == "__main__":
    unittest.main()
