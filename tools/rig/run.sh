#!/usr/bin/env bash
# run.sh original|ours [map] - run one of the two test servers in the foreground (Ctrl-C stops it).
#
#   original  ~/Containers/hlds/hlds-linux32-test   port 27041   reference/hl_i386.so
#   ours      ~/Containers/hlds/hlds-linux32-ozours port 27042   build-i386/dlls/hl.so
#
# Join: steam -applaunch 70 -game oz +connect 127.0.0.1:<port>
# -nobreakpad: the engine's crash handler otherwise uploads minidumps to Valve.
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
MAP="${2:-crossfire}"

# A server just stopped can hold its port for a few seconds; started too soon, the engine fails to
# bind it and segfaults on the way out (exit 139). Wait for it.
wait_port() {
	for _ in $(seq 30); do
		ss -uln 2>/dev/null | grep -q ":$1 " || return 0
		sleep 0.5
	done
	echo "port $1 still in use" >&2
	exit 1
}
case "${1:-}" in
	original)
		cd "${RIG:-$HOME/Containers/hlds/hlds-linux32-test}"
		export LD_LIBRARY_PATH=".:$HERE/lib:${LD_LIBRARY_PATH:-}" # egcs 1.1.2's libstdc++ (setup.sh)
		PORT=27041 ;;
	ours)
		cd "${RIG2:-$HOME/Containers/hlds/hlds-linux32-ozours}"
		export LD_LIBRARY_PATH=".:${LD_LIBRARY_PATH:-}"
		PORT=27042 ;;
	*) sed -n '2,8p' "$0"; exit 2 ;;
esac
wait_port "$PORT"
# sv_lan 0 so players carry their Steam IDs; -nomaster keeps it off Valve's master server list
exec ./hlds_linux -nobreakpad -nomaster -game oz +developer 1 +sv_cheats 1 +maxplayers 8 +sv_lan 0 \
	-port "$PORT" +map "$MAP"
