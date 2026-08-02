#!/usr/bin/env python3
"""Validate generated RA4 production-draft models inside Unreal Editor."""

from __future__ import annotations

import json
from datetime import datetime, timezone
from pathlib import Path

import unreal


PROJECT_ROOT = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
MANIFEST_PATH = PROJECT_ROOT / "Tools/Art/model_manifest.json"


def expected_sockets(kind: str) -> list[str]:
    if kind == "building":
        return ["VFX_Impact", "UnitExit", "Rally", "SelectionOrigin", "Exhaust"]
    return ["Turret", "Muzzle", "ProjectileSpawn", "VFX_Impact", "Engine", "Exhaust", "SelectionOrigin"]


def validate(entry: dict) -> dict:
    mesh = unreal.load_asset(entry["target"])
    errors = []
    if not isinstance(mesh, unreal.StaticMesh):
        return {"stable_id": entry["stable_id"], "status": "BLOCKED", "errors": ["target asset is missing"]}
    lod_count = int(mesh.get_num_lods())
    collision_count = int(unreal.EditorStaticMeshLibrary.get_simple_collision_count(mesh))
    sockets = sorted(name for name in expected_sockets(entry["kind"]) if mesh.find_socket(name) is not None)
    material_count = 0
    material_paths = []
    for index in range(16):
        material = mesh.get_material(index)
        if material is None:
            break
        material_count += 1
        material_paths.append(material.get_path_name())
    if lod_count < 4:
        errors.append(f"expected 4 LODs, found {lod_count}")
    if collision_count < 1:
        errors.append("no simple collision")
    missing_sockets = sorted(set(expected_sockets(entry["kind"])) - set(sockets))
    if missing_sockets:
        errors.append(f"missing sockets: {', '.join(missing_sockets)}")
    if material_count < 1:
        errors.append("no material slots")
    source = PROJECT_ROOT / "ArtSource/RA4/Models" / entry["faction"] / f"SRC_{entry['faction']}_{entry['stable_id']}.blend"
    if not source.exists():
        errors.append("editable .blend source is missing")
    return {
        "stable_id": entry["stable_id"],
        "status": "VALIDATED" if not errors else "BLOCKED",
        "target": entry["target"],
        "lod_count": lod_count,
        "simple_collision_count": collision_count,
        "socket_count": len(sockets),
        "material_count": material_count,
        "materials": material_paths,
        "source_blend": str(source.relative_to(PROJECT_ROOT)),
        "errors": errors,
    }


def main() -> None:
    manifest = json.loads(MANIFEST_PATH.read_text(encoding="utf-8"))
    results = [validate(entry) for entry in manifest["assets"]]
    report = {
        "generated_at_utc": datetime.now(timezone.utc).isoformat(),
        "engine_version": str(unreal.SystemLibrary.get_engine_version()),
        "results": results,
        "summary": {
            "requested": len(results),
            "validated": sum(result["status"] == "VALIDATED" for result in results),
            "blocked": sum(result["status"] == "BLOCKED" for result in results),
        },
    }
    path = PROJECT_ROOT / "Saved/Reports/ModelValidationReport.json"
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(report, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    unreal.log(f"RA4ModelValidation: {report['summary']} -> {path}")


if __name__ == "__main__":
    main()
