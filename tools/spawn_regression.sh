#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
FILE="$ROOT/kernel/program.c"
grep -q 'program_release(slot + 1)' "$FILE" || { echo "spawn fault must call program_release"; exit 1; }
grep -q 'program_release(id)' "$FILE" || { echo "spawn_elf fault must call program_release"; exit 1; }
python3 - <<PY
from pathlib import Path
text = Path("$ROOT/kernel/program.c").read_text()
if "program_enter(program, program->entry_point) != 0" not in text:
    raise SystemExit("missing program_spawn enter check")
chunk = text.split("program_enter(program, program->entry_point) != 0", 1)[1][:120]
if "return -1" not in chunk:
    raise SystemExit("program_spawn must return -1 on fault")
print("spawn regression OK")
PY
