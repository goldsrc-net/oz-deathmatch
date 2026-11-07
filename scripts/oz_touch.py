#!/usr/bin/env python3
"""oz_touch.py - stock-named functions whose original body uses something OZ added.

inventory.py can only see functions OZ added by name. OZ also changed the bodies of stock SDK
functions (the weapons read OZ's damage_* and cycle_* cvars). This flags each stock-named function in the
original whose decompile (recovered/decomp/*.c) references an OZ-only name: an OZ-only class, a
function or method OZ added (funcfile.tsv kind "oz", which covers the virtuals OZ added to SDK
classes), or an OZ cvar (game.h's Oz Deathmatch block), by symbol or by name.

It's a lead, not proof: a changed constant or a removed line references nothing new. Writes
reference/oz_touch.tsv: file, function, size, the OZ names it references.
Usage: scripts/oz_touch.py [--summary]
"""
import argparse, collections, csv, glob, os, re

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
REF = os.path.join(ROOT, "reference")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--summary", action="store_true")
    a = ap.parse_args()
    rows = [r for r in csv.DictReader(open(os.path.join(REF, "funcfile.tsv")), delimiter="\t")
            if r["kind"] != "generated"]
    stock_files = set(open(os.path.join(REF, "sdk-stock-files.txt")).read().split()) | {"globals"}
    stock_methods = collections.defaultdict(set)
    for name in open(os.path.join(REF, "sdk-stock-methods.txt")).read().splitlines():
        cls, _, meth = name.rpartition("::")
        stock_methods[cls].add(meth)
    stock_classes = {c for c in stock_methods if c}

    # Only names the stock SDK never uses for anything: an OZ class overriding ObjectCaps makes
    # "ObjectCaps" an oz row, but it says nothing about OZ. Undemangled gcc2 operators (__as__,
    # __rf__ ...) are skipped too.
    stock_any = set()
    for ms in stock_methods.values():
        stock_any |= ms
    stock_any |= stock_classes
    oz_names = set()
    for r in rows:
        if r["kind"] == "oz":
            for n in (r["method"], r["class"]):
                if n and n not in stock_any and not n.startswith("_") and "__" not in n:
                    oz_names.add(n)
    # OZ's cvars: the symbols in game.h's Oz Deathmatch block (cvars.tsv can't tell any more,
    # since our build now has them too)
    gh = open(os.path.join(ROOT, "dlls", "game.h")).read()
    blk = gh.split("// Oz Deathmatch 3.02", 1)[1] if "// Oz Deathmatch 3.02" in gh else ""
    oz_syms = set(re.findall(r"extern cvar_t (\w+);", blk))
    oz_names |= oz_syms
    oz_names |= {r["name"] for r in csv.DictReader(open(os.path.join(REF, "cvars.tsv")), delimiter="\t")
                 if r["symbol"] in oz_syms}   # read by name too: CVAR_GET_STRING("customrunemodel")
    pat = re.compile(r"\b(" + "|".join(sorted(map(re.escape, oz_names), key=len, reverse=True)) + r")\b")

    # decompile bodies by (class, method) from the "/* Class::Method  @ addr" headers
    bodies = {}
    for f in glob.glob(os.path.join(ROOT, "recovered", "decomp", "*.c")):
        cur, buf = None, []
        for line in open(f, errors="replace"):
            m = re.match(r"/\* (\S+)  @ ([0-9a-f]+) ", line)
            if m:
                if cur:
                    bodies[cur] = "".join(buf)
                cur, buf = int(m.group(2), 16) - 0x10000, []
                continue
            buf.append(line)
        if cur:
            bodies[cur] = "".join(buf)

    out = []
    for r in rows:
        if r["kind"] != "stock" or r["file"].rsplit(".", 1)[0].lower() not in stock_files:
            continue
        body = bodies.get(int(r["addr"], 16))
        if not body:
            continue
        hits = sorted({m.group(0) for m in pat.finditer(body)})
        if hits:
            name = f"{r['class']}::{r['method']}" if r["class"] else r["method"]
            out.append((r["file"], name, int(r["size"]), hits))
    with open(os.path.join(REF, "oz_touch.tsv"), "w") as fo:
        fo.write("file\tfunction\tsize\toz_names\n")
        for f, n, s, h in sorted(out):
            fo.write(f"{f}\t{n}\t{s}\t{' '.join(h)}\n")
    print(f"{len(out)} stock-named functions reference OZ names -> reference/oz_touch.tsv")
    if a.summary:
        per = collections.Counter(); size = collections.Counter()
        for f, n, s, h in out:
            per[f] += 1; size[f] += s
        for f, c in sorted(per.items(), key=lambda x: -size[x[0]]):
            print(f"  {f:26s} {c:3d} fns {size[f]:7d} B")


if __name__ == "__main__":
    main()
