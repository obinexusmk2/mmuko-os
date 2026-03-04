#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 1 ]]; then
  echo "Usage: $0 <version> [release-label]"
  exit 1
fi

VERSION="$1"
RELEASE="${2:-v$VERSION}"

python3 scripts/export_artifacts.py --version "$VERSION" --release "$RELEASE"
