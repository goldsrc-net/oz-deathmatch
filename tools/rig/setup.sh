#!/usr/bin/env bash
# setup.sh - set up the side-by-side test: the original OZ 3.02 and our rebuild on two servers,
# and the client folder to join them with. Idempotent: re-running refreshes our hl.so and
# leaves everything else alone. Undo: tools/rig/README.md.
#
#   tools/rig/setup.sh      needs the 3.02 Steam server package unpacked (OZ_SRC, below),
#                           build-i386/dlls/hl.so (scripts/build.sh i386), and the 32-bit rig below
#
# Rig: ~/Containers/hlds/hlds-linux32-test, a 32-bit ReHLDS install with valve/ content.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
SRC="${OZ_SRC:-/media/Data3/Projects/mods/oz/x-oz3-02_for_steam/SteamApps/YOUR_steam_username/half-life/oz}"
RIG="${RIG:-$HOME/Containers/hlds/hlds-linux32-test}"     # runs the original, port 27041
RIG2="${RIG2:-$HOME/Containers/hlds/hlds-linux32-ozours}" # runs ours, port 27042
CLIENT="${CLIENT:-$HOME/.local/share/Steam/steamapps/common/Half-Life}"
OURS="$ROOT/build-i386/dlls/hl.so"
ORIG="$ROOT/reference/hl_i386.so"
LIB="$ROOT/tools/rig/lib" # gitignored

for f in "$SRC/liblist.gam" "$OURS" "$RIG/hlds_linux" "$CLIENT/valve/config.cfg"; do
	[ -e "$f" ] || { echo "missing: $f" >&2; exit 1; }
done

# The original needs egcs 1.1.2's C++ runtime, libstdc++-libc6.1-1.so.2, which no current
# system has: Debian 2.2's package of it (built from egcs 1.1.2, OZ's compiler), from Debian's
# archive. md5 as in potato's Packages index.
DEB_URL="http://archive.debian.org/debian/pool/main/e/egcs1.1/libstdc++2.9-glibc2.1_2.91.66-4_i386.deb"
DEB_SHA256="1e71b4caa8d408dde0d3669e474905844e45c2bfc121e96840f0bf2437c6d1bd"
if [ ! -e "$LIB/libstdc++-libc6.1-1.so.2" ]; then
	T="$(mktemp -d)"
	curl -sf -o "$T/p.deb" "$DEB_URL"
	echo "$DEB_SHA256  $T/p.deb" | sha256sum -c --quiet
	dpkg-deb -x "$T/p.deb" "$T/x"
	mkdir -p "$LIB"
	cp "$T/x/usr/lib/libstdc++-2-libc6.1-1-2.9.0.so" "$LIB/libstdc++-libc6.1-1.so.2"
	rm -rf -- "${T:?}"
fi

# Replace a library only when it changed, through a new file renamed into place: a running
# server keeps its mapping of the old one (cp over a mapped .so truncates it under the server).
put_lib() { # <src> <dst>
	cmp -s "$1" "$2" 2>/dev/null && return 0
	cp "$1" "$2.new" && mv -f "$2.new" "$2"
}

server_gamedir() { # <dir> <gamedll path>
	local d="$1"
	if [ ! -d "$d" ]; then
		mkdir -p "$d/dlls"
		rsync -a --exclude 'dlls/' --exclude 'liblist.gam*' --exclude 'logs/' --exclude 'SAVE/' "$SRC/" "$d/"
		# cheats on for testing (stock: 0); GameDLLInit execs cfg/multiplayer.cfg after the command line
		sed -i 's/^sv_cheats 0\(\r\?\)$/sv_cheats 1\1/' "$d/cfg/multiplayer.cfg"
	fi
	printf 'game "Oz Deathmatch"\nversion "3.02"\nsvonly "1"\ncldll "0"\nmpentity "info_player_deathmatch"\ngamedll_linux "%s"\n' "$2" > "$d/liblist.gam"
}

# server 1: the original
server_gamedir "$RIG/oz" "dlls/hl_i386.so"
put_lib "$ORIG" "$RIG/oz/dlls/hl_i386.so"

# server 2: ours, in a folder of links to the rig with its own oz/
if [ ! -d "$RIG2" ]; then
	mkdir "$RIG2"
	for e in "$RIG"/*; do
		b="$(basename "$e")"
		case "$b" in oz | ozours) continue ;; esac
		ln -s "$e" "$RIG2/$b"
	done
fi
server_gamedir "$RIG2/oz" "dlls/hl.so"
put_lib "$OURS" "$RIG2/oz/dlls/hl.so"
# smoke.py's copy of ours (gamedir ozours in the first rig)
server_gamedir "$RIG/ozours" "dlls/hl.so"
put_lib "$OURS" "$RIG/ozours/dlls/hl.so"

# client: the 3.02 content, with the player's valve binds (OZ ships no config.cfg)
if [ ! -d "$CLIENT/oz" ]; then
	mkdir "$CLIENT/oz"
	rsync -a --exclude 'dlls/' --exclude 'logs/' --exclude 'SAVE/' "$SRC/" "$CLIENT/oz/"
	cp "$CLIENT/valve/config.cfg" "$CLIENT/oz/config.cfg"
fi

echo "original: $(md5sum "$RIG/oz/dlls/hl_i386.so" | cut -c1-12)  ours: $(md5sum "$RIG2/oz/dlls/hl.so" | cut -c1-12)"
echo "run: tools/rig/run.sh original | ours"
