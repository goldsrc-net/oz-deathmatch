#!/usr/bin/env python3
"""smoke.py - boot a gamedir on the 32-bit HLDS rig, drive its console, save the log.

Runs the original OZ (gamedir oz, with egcs 1.1.2's libstdc++ from tools/rig/lib) or our build
(gamedir ozours) the same way, so the two logs can be diffed:

  tools/rig/smoke.py oz out-orig.log
  tools/rig/smoke.py ozours out-ours.log
  diff <(tools/rig/smoke.py --clean out-orig.log) <(tools/rig/smoke.py --clean out-ours.log)

The script: start hlds_linux (no breakpad, developer 1, LAN), wait for the map, then send each
console command in SCRIPT after its delay, end with "quit", and wait for the exit. Commands go
in on stdin, which HLDS reads as console input.

Rig: ~/Containers/hlds/hlds-linux32-test (32-bit ReHLDS), set up by tools/rig/setup.sh.
"""
import argparse, os, re, subprocess, sys, threading, time

RIG = os.path.expanduser("~/Containers/hlds/hlds-linux32-test")
HERE = os.path.dirname(os.path.abspath(__file__))
LIB = os.path.join(HERE, "lib")

# (seconds after the previous command, command)
SCRIPT = [
    (8, "status"),
    (2, "changelevel bounce"),
    (10, "status"),
    (2, "changelevel datacore"),
    (10, "status"),
    (2, "quit"),
]


def clean(path):
    """A log with run-specific noise removed, for diffing two runs."""
    out = []
    for line in open(path, errors="replace"):
        line = re.sub(r"\d\d/\d\d(/\d{4} -)? \d\d:\d\d:\d\d", "", line)
        line = re.sub(r"\b(oz|ozours)\b", "GAME", line)
        line = re.sub(r"(hl_i386|hl)\.so", "GAMEDLL.so", line)
        line = re.sub(r"\d+(\.\d+)?ms", "Nms", line)
        if re.search(r"ELFCLASS32|minidump|^\s*$", line):
            continue
        out.append(line.rstrip("\n"))
    print("\n".join(out))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("game", nargs="?")
    ap.add_argument("log", nargs="?")
    ap.add_argument("--map", default="crossfire")
    ap.add_argument("--port", default="27043", help="UDP port (default 27043, clear of run.sh's 27041 and 27042)")
    ap.add_argument("--clean", metavar="LOG")
    a = ap.parse_args()
    if a.clean:
        return clean(a.clean)
    env = dict(os.environ, LD_LIBRARY_PATH=".:" + os.environ.get("LD_LIBRARY_PATH", ""))
    if a.game == "oz":
        env["LD_LIBRARY_PATH"] = ".:" + LIB + ":" + os.environ.get("LD_LIBRARY_PATH", "")
    cmd = ["./hlds_linux", "-nobreakpad", "-game", a.game, "+developer", "1", "+maxplayers", "8",
           "+sv_lan", "1", "-port", a.port, "+map", a.map]
    log = open(a.log, "w")
    p = subprocess.Popen(cmd, cwd=RIG, env=env, stdin=subprocess.PIPE, stdout=log,
                         stderr=subprocess.STDOUT, text=True)

    def feed():
        for delay, c in SCRIPT:
            time.sleep(delay)
            if p.poll() is not None:
                return
            p.stdin.write(c + "\n")
            p.stdin.flush()
    threading.Thread(target=feed, daemon=True).start()
    try:
        rc = p.wait(timeout=sum(d for d, _ in SCRIPT) + 30)
    except subprocess.TimeoutExpired:
        p.kill()
        rc = "killed (no exit after quit)"
    log.close()
    print(f"{a.game}: exit {rc}")


if __name__ == "__main__":
    sys.exit(main())
