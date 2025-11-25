#!/usr/bin/env bash
set -euo pipefail

# run.sh - 執行對應到 app + action 的命令
# 用法: ./run.sh <app> <action> [--dry-run] [-- <extra args to append>]

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MAP_FILE="$SCRIPT_DIR/commands.json"


if [ $# -lt 2 ]; then
  echo "Usage: $0 <app> <action> [--dry-run] [-- <extra args to append>]"
  exit 2
fi

APP="$1"
ACTION="$2"
shift 2

DRY_RUN=false
EXTRA_ARGS=()
if [ "$#" -gt 0 ]; then
  # allow optional --dry-run and optional `--` before extra args
  while [ "$#" -gt 0 ]; do
    case "$1" in
      --dry-run)
        DRY_RUN=true; shift ;;
      --)
        shift; EXTRA_ARGS=("$@"); break ;;
      *)
        # treat remaining args as extra args to append
        EXTRA_ARGS=("$@"); break ;;
    esac
  done
fi

# Load command from JSON mapping using Python
if [ ! -f "$MAP_FILE" ]; then
  echo "Mapping file not found: $MAP_FILE"
  exit 3
fi

COMMAND=""

# Use Python to read JSON and extract the command for the given app/action.
# Exit code 5 indicates not found in JSON.
COMMAND=$(python3 - "$MAP_FILE" "$APP" "$ACTION" <<'PY'
import sys, json
map_file = sys.argv[1]
app = sys.argv[2]
action = sys.argv[3]
try:
    with open(map_file, 'r') as f:
        data = json.load(f)
except Exception as e:
    print(f"Failed to read mapping file: {e}", file=sys.stderr)
    sys.exit(4)

cmd = None
if isinstance(data, dict):
    app_entry = data.get(app)
    if isinstance(app_entry, dict):
        cmd = app_entry.get(action)

if not cmd:
    # signal not found
    sys.exit(5)
print(cmd)
PY
)
py_exit=$?
if [ $py_exit -eq 4 ]; then
  echo "Failed to read mapping file: $MAP_FILE"
  exit 4
fi
if [ $py_exit -eq 5 ]; then
  echo "No command found for app='$APP' action='$ACTION' in $MAP_FILE"
  exit 5
fi

# Trim whitespace from command
COMMAND="$(echo "$COMMAND" | sed -e 's/^ *//' -e 's/ *$//')"

# Append any extra args (safely)
if [ ${#EXTRA_ARGS[@]} -gt 0 ]; then
  COMMAND="$COMMAND ${EXTRA_ARGS[*]}"
fi

echo "App: $APP  Action: $ACTION"
echo "Command: $COMMAND"

if [ "$DRY_RUN" = true ]; then
  echo "(dry-run) not executing"
  exit 0
fi

# Execute the command. Use eval so commands with complex quoting work as in mapping.
# Warning: eval executes arbitrary shell code; only use mappings you trust.
eval "$COMMAND"
