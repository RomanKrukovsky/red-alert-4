#!/usr/bin/env python3
"""
Blocking clean-room compliance scanner for the Red Alert 4 repository.
"""

import argparse
import datetime as dt
import fnmatch
import json
import os
import pathlib
import re
import subprocess
import sys
from dataclasses import dataclass
from typing import Any, Iterable


SCRIPT_DIR = pathlib.Path(__file__).resolve().parent
DEFAULT_POLICY_DIR = SCRIPT_DIR / "policy"
TEXT_EXTENSIONS = {
    ".bat",
    ".cmake",
    ".cpp",
    ".cs",
    ".csv",
    ".h",
    ".hpp",
    ".html",
    ".ini",
    ".json",
    ".md",
    ".ps1",
    ".py",
    ".sh",
    ".txt",
    ".xml",
    ".xsd",
    ".yaml",
    ".yml",
}


@dataclass(frozen=True)
class Violation:
    rule_id: str
    scope: str
    path: str
    message: str

    def render(self) -> str:
        return f"[{self.scope}] {self.rule_id}: {self.path} -> {self.message}"


def load_json(path: pathlib.Path) -> Any:
    with path.open("r", encoding="utf-8") as handle:
        return json.load(handle)


def normalize_relpath(path: str) -> str:
    return pathlib.PurePosixPath(path.replace("\\", "/")).as_posix()


def add_violation(
    sink: list[Violation],
    seen: set[tuple[str, str, str, str]],
    rule_id: str,
    scope: str,
    path: str,
    message: str,
) -> None:
    normalized_path = normalize_relpath(path)
    key = (rule_id, scope, normalized_path, message)
    if key in seen:
        return
    seen.add(key)
    sink.append(Violation(rule_id=rule_id, scope=scope, path=normalized_path, message=message))


def list_git_index_files(repo_root: pathlib.Path) -> list[str]:
    result = subprocess.run(
        ["git", "-C", str(repo_root), "ls-files", "-z"],
        check=True,
        capture_output=True,
    )
    raw_paths = result.stdout.split(b"\0")
    paths = [normalize_relpath(item.decode("utf-8")) for item in raw_paths if item]
    return sorted(paths)


def read_git_index_bytes(repo_root: pathlib.Path, rel_path: str) -> bytes:
    result = subprocess.run(
        ["git", "-C", str(repo_root), "show", f":{rel_path}"],
        check=True,
        capture_output=True,
    )
    return result.stdout


def iter_workspace_files(repo_root: pathlib.Path, roots: Iterable[str]) -> list[str]:
    files: list[str] = []
    for root_name in roots:
        abs_root = repo_root / root_name
        if not abs_root.exists():
            continue
        for dirpath, dirnames, filenames in os.walk(abs_root):
            dirnames[:] = [name for name in dirnames if name not in {".git", "__pycache__"}]
            for filename in filenames:
                abs_path = pathlib.Path(dirpath) / filename
                rel_path = normalize_relpath(str(abs_path.relative_to(repo_root)))
                files.append(rel_path)
    return sorted(files)


def looks_text_bytes(payload: bytes, suffix: str) -> bool:
    if suffix in TEXT_EXTENSIONS:
        return True
    if b"\0" in payload[:4096]:
        return False
    try:
        payload[:4096].decode("utf-8")
    except UnicodeDecodeError:
        return False
    return True


def read_workspace_bytes(repo_root: pathlib.Path, rel_path: str) -> bytes:
    return (repo_root / rel_path).read_bytes()


def load_policy(policy_dir: pathlib.Path) -> dict[str, Any]:
    return load_json(policy_dir / "denylist.json")


def load_provenance_schema(policy_dir: pathlib.Path) -> dict[str, Any]:
    return load_json(policy_dir / "provenance.schema.json")


def load_suppressions(policy_dir: pathlib.Path) -> dict[str, Any]:
    return load_json(policy_dir / "suppressions.json")


def parse_iso_date(raw_value: str) -> dt.date | None:
    try:
        return dt.date.fromisoformat(raw_value)
    except ValueError:
        return None


def validate_suppressions(
    suppressions_doc: dict[str, Any],
    today: dt.date,
) -> tuple[list[Violation], list[dict[str, Any]]]:
    violations: list[Violation] = []
    seen: set[tuple[str, str, str, str]] = set()
    suppressions = suppressions_doc.get("suppressions")
    if not isinstance(suppressions, list):
        add_violation(
            violations,
            seen,
            "invalid_suppressions_file",
            "config",
            "Build/Compliance/policy/suppressions.json",
            "Top-level 'suppressions' must be a list.",
        )
        return violations, []

    valid_entries: list[dict[str, Any]] = []
    for index, entry in enumerate(suppressions):
        entry_path = f"Build/Compliance/policy/suppressions.json#{index}"
        if not isinstance(entry, dict):
            add_violation(
                violations,
                seen,
                "invalid_suppression_entry",
                "config",
                entry_path,
                "Suppression entry must be an object.",
            )
            continue
        required_fields = ["id", "rule_id", "path_glob", "comment", "reviewer", "expires_on"]
        missing = [field for field in required_fields if not isinstance(entry.get(field), str) or not entry.get(field).strip()]
        if missing:
            add_violation(
                violations,
                seen,
                "invalid_suppression_entry",
                "config",
                entry_path,
                f"Missing required non-empty fields: {', '.join(sorted(missing))}.",
            )
            continue
        expiry = parse_iso_date(entry["expires_on"])
        if expiry is None:
            add_violation(
                violations,
                seen,
                "invalid_suppression_entry",
                "config",
                entry_path,
                "expires_on must be YYYY-MM-DD.",
            )
            continue
        normalized_entry = dict(entry)
        normalized_entry["expires_on_date"] = expiry
        normalized_entry["expired"] = expiry < today
        valid_entries.append(normalized_entry)
    return violations, valid_entries


def is_suppressed(
    rule_id: str,
    rel_path: str,
    suppressions: list[dict[str, Any]],
) -> bool:
    normalized_path = normalize_relpath(rel_path)
    for entry in suppressions:
        if entry.get("expired"):
            continue
        if entry["rule_id"] != rule_id:
            continue
        if fnmatch.fnmatch(normalized_path, entry["path_glob"]):
            return True
    return False


def maybe_add_violation(
    sink: list[Violation],
    seen: set[tuple[str, str, str, str]],
    suppressions: list[dict[str, Any]],
    rule_id: str,
    scope: str,
    path: str,
    message: str,
) -> None:
    if is_suppressed(rule_id, path, suppressions):
        return
    add_violation(sink, seen, rule_id, scope, path, message)


def normalize_provenance_record(raw_record: dict[str, Any]) -> dict[str, Any]:
    return {
        "asset_id": raw_record.get("asset_id") or raw_record.get("assetId"),
        "path": raw_record.get("path") or raw_record.get("localImportPath"),
        "sha256": raw_record.get("sha256") or raw_record.get("checksumSHA256"),
        "artifact_kind": raw_record.get("artifact_kind") or raw_record.get("category"),
        "creator": raw_record.get("creator") or raw_record.get("author"),
        "creation_date": raw_record.get("creation_date") or raw_record.get("creationDate"),
        "source_type": raw_record.get("source_type"),
        "source_uri_or_contract": raw_record.get("source_uri_or_contract") or raw_record.get("url"),
        "license": raw_record.get("license"),
        "commercial_use_allowed": raw_record.get("commercial_use_allowed"),
        "contains_third_party_ip": raw_record.get("contains_third_party_ip"),
        "similarity_review_status": raw_record.get("similarity_review_status"),
        "reviewer": raw_record.get("reviewer"),
        "comment": raw_record.get("comment") or raw_record.get("notes"),
        "release_allowed": raw_record.get("release_allowed"),
    }


def validate_provenance_manifests(
    repo_root: pathlib.Path,
    policy: dict[str, Any],
    schema: dict[str, Any],
    suppressions: list[dict[str, Any]],
) -> tuple[list[Violation], list[str]]:
    violations: list[Violation] = []
    seen: set[tuple[str, str, str, str]] = set()
    valid_paths: list[str] = []
    manifest_paths = policy.get("manifest_paths", [])
    required_fields = schema.get("required_record_fields", {})
    allowed_source_types = set(schema.get("allowed_source_types", []))
    allowed_similarity_statuses = set(schema.get("allowed_similarity_review_statuses", []))

    for manifest_rel in manifest_paths:
        manifest_abs = repo_root / manifest_rel
        if not manifest_abs.exists():
            maybe_add_violation(
                violations,
                seen,
                suppressions,
                "missing_provenance_manifest",
                "workspace",
                manifest_rel,
                "Required provenance manifest is missing.",
            )
            continue
        try:
            document = load_json(manifest_abs)
        except json.JSONDecodeError as exc:
            maybe_add_violation(
                violations,
                seen,
                suppressions,
                "invalid_provenance_manifest",
                "workspace",
                manifest_rel,
                f"Invalid JSON: {exc}.",
            )
            continue

        raw_records = document.get("records")
        if raw_records is None:
            raw_records = document.get("assets")
        if not isinstance(raw_records, list):
            maybe_add_violation(
                violations,
                seen,
                suppressions,
                "invalid_provenance_manifest",
                "workspace",
                manifest_rel,
                "Manifest must contain a top-level 'records' or 'assets' list.",
            )
            continue

        for index, raw_record in enumerate(raw_records):
            record_path = f"{manifest_rel}#{index}"
            if not isinstance(raw_record, dict):
                maybe_add_violation(
                    violations,
                    seen,
                    suppressions,
                    "invalid_provenance_record",
                    "workspace",
                    record_path,
                    "Provenance record must be an object.",
                )
                continue
            record = normalize_provenance_record(raw_record)
            record_valid = True
            for field_name, field_type in required_fields.items():
                value = record.get(field_name)
                if field_type == "string":
                    if not isinstance(value, str) or not value.strip():
                        maybe_add_violation(
                            violations,
                            seen,
                            suppressions,
                            "invalid_provenance_record",
                            "workspace",
                            record_path,
                            f"Missing required string field '{field_name}'.",
                        )
                        record_valid = False
                elif field_type == "boolean":
                    if not isinstance(value, bool):
                        maybe_add_violation(
                            violations,
                            seen,
                            suppressions,
                            "invalid_provenance_record",
                            "workspace",
                            record_path,
                            f"Missing required boolean field '{field_name}'.",
                        )
                        record_valid = False

            sha256 = record.get("sha256")
            if isinstance(sha256, str) and not re.fullmatch(r"[0-9a-fA-F]{64}", sha256):
                maybe_add_violation(
                    violations,
                    seen,
                    suppressions,
                    "invalid_provenance_record",
                    "workspace",
                    record_path,
                    "sha256 must be a 64-character hex string.",
                )
                record_valid = False

            creation_date = record.get("creation_date")
            if isinstance(creation_date, str) and parse_iso_date(creation_date) is None:
                maybe_add_violation(
                    violations,
                    seen,
                    suppressions,
                    "invalid_provenance_record",
                    "workspace",
                    record_path,
                    "creation_date must be YYYY-MM-DD.",
                )
                record_valid = False

            source_type = record.get("source_type")
            if isinstance(source_type, str) and allowed_source_types and source_type not in allowed_source_types:
                maybe_add_violation(
                    violations,
                    seen,
                    suppressions,
                    "invalid_provenance_record",
                    "workspace",
                    record_path,
                    f"source_type '{source_type}' is not allowed.",
                )
                record_valid = False

            similarity_status = record.get("similarity_review_status")
            if isinstance(similarity_status, str) and allowed_similarity_statuses and similarity_status not in allowed_similarity_statuses:
                maybe_add_violation(
                    violations,
                    seen,
                    suppressions,
                    "invalid_provenance_record",
                    "workspace",
                    record_path,
                    f"similarity_review_status '{similarity_status}' is not allowed.",
                )
                record_valid = False

            asset_path = record.get("path")
            if isinstance(asset_path, str) and asset_path.strip():
                normalized_asset_path = normalize_relpath(asset_path)
                if pathlib.PurePosixPath(normalized_asset_path).is_absolute():
                    maybe_add_violation(
                        violations,
                        seen,
                        suppressions,
                        "invalid_provenance_record",
                        "workspace",
                        record_path,
                        "path must be repository-relative, not absolute.",
                    )
                    record_valid = False
                elif not (repo_root / normalized_asset_path).exists():
                    maybe_add_violation(
                        violations,
                        seen,
                        suppressions,
                        "invalid_provenance_record",
                        "workspace",
                        record_path,
                        f"Referenced path does not exist: {normalized_asset_path}.",
                    )
                    record_valid = False
                elif record_valid:
                    valid_paths.append(normalized_asset_path)
            else:
                record_valid = False

    return violations, valid_paths


def path_is_under(rel_path: str, root_name: str) -> bool:
    normalized = normalize_relpath(rel_path)
    return normalized == root_name or normalized.startswith(f"{root_name}/")


def coverage_matches(covered_path: str, target_path: str) -> bool:
    normalized_covered = normalize_relpath(covered_path).rstrip("/")
    normalized_target = normalize_relpath(target_path)
    return normalized_target == normalized_covered or normalized_target.startswith(f"{normalized_covered}/")


def scan_path_inventory(
    repo_root: pathlib.Path,
    rel_paths: list[str],
    scope: str,
    policy: dict[str, Any],
    suppressions: list[dict[str, Any]],
    provenance_paths: list[str],
) -> list[Violation]:
    violations: list[Violation] = []
    seen: set[tuple[str, str, str, str]] = set()
    production_roots = tuple(policy.get("production_roots", []))
    forbidden_directories = tuple(normalize_relpath(item) for item in policy.get("forbidden_directories", []))
    forbidden_extensions = set(policy.get("forbidden_extensions", []))
    binary_extensions = set(policy.get("binary_asset_extensions", []))

    def read_bytes(rel_path: str) -> bytes:
        if scope == "index":
            workspace_path = repo_root / rel_path
            if workspace_path.exists():
                return read_workspace_bytes(repo_root, rel_path)
            return read_git_index_bytes(repo_root, rel_path)
        return read_workspace_bytes(repo_root, rel_path)

    for rel_path in rel_paths:
        normalized_path = normalize_relpath(rel_path)
        suffix = pathlib.PurePosixPath(normalized_path).suffix.lower()

        if scope == "index" and path_is_under(normalized_path, "ExternalResearch"):
            maybe_add_violation(
                violations,
                seen,
                suppressions,
                "external_research_in_index",
                scope,
                normalized_path,
                "ExternalResearch content must never be committed to git index.",
            )

        for forbidden_dir in forbidden_directories:
            if path_is_under(normalized_path, forbidden_dir):
                maybe_add_violation(
                    violations,
                    seen,
                    suppressions,
                    "forbidden_directory",
                    scope,
                    normalized_path,
                    f"Path lives under forbidden directory '{forbidden_dir}'.",
                )

        if not any(path_is_under(normalized_path, root_name) for root_name in production_roots):
            continue

        if suffix in forbidden_extensions:
            maybe_add_violation(
                violations,
                seen,
                suppressions,
                "forbidden_extension",
                scope,
                normalized_path,
                f"Forbidden extension '{suffix}' under production roots.",
            )

        should_read_as_text = suffix in TEXT_EXTENSIONS or suffix in forbidden_extensions
        is_known_binary = suffix in binary_extensions

        if is_known_binary:
            if not any(coverage_matches(covered_path, normalized_path) for covered_path in provenance_paths):
                maybe_add_violation(
                    violations,
                    seen,
                    suppressions,
                    "missing_provenance",
                    scope,
                    normalized_path,
                    "Binary production asset is not covered by a valid provenance record.",
                )
            continue

        file_bytes = b""
        is_text = should_read_as_text
        if should_read_as_text:
            file_bytes = read_bytes(normalized_path)
            is_text = looks_text_bytes(file_bytes, suffix)
        else:
            if not any(coverage_matches(covered_path, normalized_path) for covered_path in provenance_paths):
                maybe_add_violation(
                    violations,
                    seen,
                    suppressions,
                    "missing_provenance",
                    scope,
                    normalized_path,
                    "Binary production asset is not covered by a valid provenance record.",
                )
            continue

        if not is_text:
            if not any(coverage_matches(covered_path, normalized_path) for covered_path in provenance_paths):
                maybe_add_violation(
                    violations,
                    seen,
                    suppressions,
                    "missing_provenance",
                    scope,
                    normalized_path,
                    "Binary production asset is not covered by a valid provenance record.",
                )
            continue

        text = file_bytes.decode("utf-8", errors="ignore")
        for rule in policy.get("identifier_rules", []):
            pattern = re.compile(rule["pattern"], re.IGNORECASE | re.MULTILINE)
            if pattern.search(text):
                maybe_add_violation(
                    violations,
                    seen,
                    suppressions,
                    rule["id"],
                    scope,
                    normalized_path,
                    rule["description"],
                )

        for rule in policy.get("xml_policy_rules", []):
            extensions = set(rule.get("extensions", []))
            if extensions and suffix not in extensions:
                continue
            pattern = re.compile(rule["pattern"], re.IGNORECASE | re.MULTILINE)
            if pattern.search(text):
                maybe_add_violation(
                    violations,
                    seen,
                    suppressions,
                    rule["id"],
                    scope,
                    normalized_path,
                    rule["description"],
                )

    return violations


def scan_repository(
    repo_root: pathlib.Path,
    scope: str = "both",
    policy_dir: pathlib.Path | None = None,
) -> list[Violation]:
    resolved_policy_dir = policy_dir or DEFAULT_POLICY_DIR
    policy = load_policy(resolved_policy_dir)
    schema = load_provenance_schema(resolved_policy_dir)
    suppressions_doc = load_suppressions(resolved_policy_dir)
    today = dt.date.today()

    violations: list[Violation] = []
    suppressions_violations, suppressions = validate_suppressions(suppressions_doc, today)
    violations.extend(suppressions_violations)

    provenance_violations, provenance_paths = validate_provenance_manifests(
        repo_root=repo_root,
        policy=policy,
        schema=schema,
        suppressions=suppressions,
    )
    violations.extend(provenance_violations)

    if scope in {"index", "both"}:
        violations.extend(
            scan_path_inventory(
                repo_root=repo_root,
                rel_paths=list_git_index_files(repo_root),
                scope="index",
                policy=policy,
                suppressions=suppressions,
                provenance_paths=provenance_paths,
            )
        )

    if scope in {"workspace", "both"}:
        violations.extend(
            scan_path_inventory(
                repo_root=repo_root,
                rel_paths=iter_workspace_files(repo_root, policy.get("production_roots", [])),
                scope="workspace",
                policy=policy,
                suppressions=suppressions,
                provenance_paths=provenance_paths,
            )
        )

    violations.sort(key=lambda item: (item.scope, item.path, item.rule_id, item.message))
    return violations


def main() -> int:
    parser = argparse.ArgumentParser(description="Run the blocking clean-room compliance scanner.")
    parser.add_argument(
        "--repo-root",
        default=str(SCRIPT_DIR.parent.parent),
        help="Repository root to scan.",
    )
    parser.add_argument(
        "--scope",
        choices=["index", "workspace", "both"],
        default="both",
        help="Scan git index, workspace production roots, or both.",
    )
    parser.add_argument(
        "--policy-dir",
        default=str(DEFAULT_POLICY_DIR),
        help="Policy directory containing denylist, suppressions, and provenance schema.",
    )
    args = parser.parse_args()

    repo_root = pathlib.Path(args.repo_root).resolve()
    policy_dir = pathlib.Path(args.policy_dir).resolve()

    print(f"[Compliance Scan] Scanning repository at: {repo_root}")
    print(f"[Compliance Scan] Scope: {args.scope}")
    print(f"[Compliance Scan] Policy: {policy_dir}")

    violations = scan_repository(repo_root=repo_root, scope=args.scope, policy_dir=policy_dir)
    if violations:
        print(f"\n[FAIL] Compliance Scan found {len(violations)} violation(s):")
        for violation in violations:
            print(f"  - {violation.render()}")
        return 1

    print("[PASS] Compliance Scan completed with 0 violations.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
