#!/usr/bin/env bash
set -euo pipefail

REPO="${1:-/Users/romanmolodyko/Documents/red-alert-4}"
SOURCE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEST="$REPO/Research/RA3_SAGE_Study"

if [[ ! -d "$REPO" ]]; then
  echo "Ошибка: репозиторий не найден: $REPO" >&2
  exit 1
fi

mkdir -p "$DEST"
for file in \
  README.md \
  01_SOURCE_MATRIX.md \
  02_RA3_DATA_MODEL.md \
  03_SAGE_TO_UNREAL_ARCHITECTURE.md \
  04_CLEAN_ROOM_POLICY.md \
  05_IMPORTER_BLUEPRINT.md \
  06_RESEARCH_BACKLOG.md \
  research_sources.json; do
  cp "$SOURCE_DIR/$file" "$DEST/$file"
done

cat > "$DEST/.gitignore" <<'IGNORE'
ExternalResearch/
RawSources/
SourceCache/
*.w3x
*.tga
*.max
IGNORE

echo "Установлено в: $DEST"
