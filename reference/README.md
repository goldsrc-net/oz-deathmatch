# reference/ - original Oz Deathmatch artifacts

Source-of-truth binaries from the original mod, kept for reconstructing the game library as C++ on
this SDK. Reference only, not linked into any build.

| File | What it is |
|---|---|
| `hl_i386.so` | 3.02, Linux 32-bit server library, egcs 1.1.2 (egcs-2.91.66), **not stripped**: 6,969 function symbols with names and addresses, and the names of the 95 source files it was built from. The library to reconstruct. |
| `mp.dll` | 3.02, Windows 32-bit server library, MSVC, built 2004-08-02 00:37 UTC. |
| `mp_i386.so` | 3.0, Linux 32-bit server library, pgcc-2.91.66 (egcs 1.1.2), not stripped, 87 source files. The release before 3.02, for diffs. |
| `sourcefiles.txt` | The 95 source file names from `hl_i386.so`'s symbol table. Most are stock SDK files, which OZ changed in place; its own are the RhoBot files (`bot_*.cpp`), `flyingcrowbar.cpp`, `grapple.cpp`, `globals.cpp` and `hl_wpn_glock.cpp`. |

```
c82afdb958f21959c10e1b9f9ce666db  hl_i386.so
16277e1a9bc89d65ae5b165456cc9994  mp.dll
de580e01be665d7d2f88b09648dc70b3  mp_i386.so
```

## Provenance

Downloaded 2026-11-07 from the sparksonline.org mirror that the official download page
(http://ozdeathmatch.com/download.html) links:

```
fbaa9200c88065590744b0738bfd5e6c9843b567f702443cf305be64e79e0d18  oz_3.02.tar.gz          hl_i386.so ("3.02 update for Linux Servers using Steam")
4a43e8c4f3e64d64fccbb161a1f26f4b4fe2d015c411700c5b46f0e84ace0b4f  oz-win32-3_02.zip       hl.dll, = mp.dll
39f0db26d0538715bf2db53db4f039517f1750caf20a243b3e7068b3fc4edcae  oz3-02_for_steam.zip    the full 3.02 Steam server: content (oz/) and mp.dll
305c316d2b74ce2b3a250ee292e8cccb29ebd0712411e8e12791367df55f3a42  oz3_nobots_linux.tar.gz the full 3.0 Linux server (WON era): content and mp_i386.so
1e0f8fd4418df0093fb6ee2973046199d8cc36fa9ef72640d04dde8f53a914df  Oz3_info.zip            3.0 tech info and FAQs
ff02f232f4719817eec88710f38f07f172bc6dafec6c7ff7209700f2087563fc  links.txt               the mirror's own list
```

The 3.02 Windows DLL in the Win32 update and in the Steam package are byte-identical.

The OZ 1.7 source the page links (FilePlanet's `Oz17src.zip`, gone) is mirrored on GitHub as
nekonomicon/OZDM: AfterShock Productions' own release, every OZ change marked
`// Oz`. Its readme gives everyone use of it on two terms: credit Oz Deathmatch for what is used,
and don't release modifications under the Oz name.
