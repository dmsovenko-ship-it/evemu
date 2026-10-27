# Client patch tool (compiled.code)

`patch_news_url.py` rewrites CCP URLs inside the Crucible client's
`script/compiled.code` (3DES+zlib, CryptoAPI exponent-one RSA key from
`bin/blue.dll`) so in-game news/browser links point at our portal:

- `holoscreenSvc.RSS_FEEDS`        (CQ news ticker)
- `billboardMgr`                   (station billboard headlines + advert texture)
- `browserutil.DefaultHomepage`    (in-game browser home page)
- `menusvc`                        (billboard "Read News" menu item)

Configure `patch_config.json` (`news_url_base`, `old_url_base`). Usage:

    set PATCH_CODE_PATH=<path to compiled.code>   (or edit CODE_PATH)
    python patch_news_url.py [--verify]

It writes `<CODE_PATH>.patched` (never edits the original) and verifies the
round-trip. Requires Windows (ctypes CryptoAPI) + `doc/evedec_crucible.py`.
Distribute the result via the launcher update ZIP (script/compiled.code).