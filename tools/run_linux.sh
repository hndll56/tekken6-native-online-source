#!/usr/bin/env bash
set -euo pipefail
source_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
export T6_SOURCE_DIR="$source_dir"
export T6_HOME="${T6_HOME:-${XDG_DATA_HOME:-$HOME/.local/share}/tekken6-native-online}"
mkdir -p -- "$T6_HOME"
launcher="$T6_HOME/t6-native-launcher"
if [[ ! -x "$launcher" || "${1:-}" == --install ]]; then
    install -m755 -- "$source_dir/build/components/launcher/t6-native-launcher" "$launcher.new"
    mv -f -- "$launcher.new" "$launcher"
    if [[ "${1:-}" == --install ]]; then shift; fi
fi
exec "$launcher" "$@"
