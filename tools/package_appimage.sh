#!/usr/bin/env bash
# Official Linux releases use the same shared builder and title recipe.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "${ROOT}"
exec "${PYTHON:-python3}" "${ROOT}/psxrecomp/tools/package_title_release.py" package --platform linux "$@"
