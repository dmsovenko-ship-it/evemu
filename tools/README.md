# EVEmu Tools

## import_prices.py — Import EVE Online market prices

Fetches sell/buy orders from EVE ESI (Jita/The Forge by default)
and updates `basePrice` in `invTypes`.

Usage:
```
pip install pymysql
python tools/import_prices.py --db-user root --db-pass "" --db-name evemu
```

Options:
- `--region 10000002` — ESI region ID (default: The Forge/Jita)
- `--order-type sell` — use sell or buy orders (default: sell)
- `--median` — use median price (default)
- `--mean` — use mean price instead
- `--db-host`, `--db-port`, `--db-user`, `--db-pass`, `--db-name`

After import, MarketBot (Trader Joe) will use updated prices automatically.
For a live server with real players, disable MarketBot in the server config.
