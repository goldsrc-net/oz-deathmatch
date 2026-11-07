# oz/ - mod content directory

The runtime game directory for mod-ozdm, run next to the stock `valve/` directory with `-game oz`.

```
oz/
  liblist.gam     # mod manifest (this repo)
  dlls/hl.so      # server game library built from this repo
  ...             # maps, cfg/, bots/, nav/ and anything else OZ overrides;
                  #   everything not here falls back to valve/
```

The original's content is in the 3.02 Steam server package; see `../../reference/README.md`.
OZ has no client library of its own, so there is no `cl_dlls/` here.
