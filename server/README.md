# Dungeon Descent — Single-Domain Oracle Cloud Hosting (Web Edition + Global API)

Lightweight, zero-dependency Node.js 22+ API server (`node:http`, `node:crypto`, built-in `node:sqlite`) + Caddy static web server designed for an **Oracle Cloud Free Tier Micro VM (1 GB RAM)** using **only ONE DuckDNS domain**: `https://dungeondescent.duckdns.org`.

## Architecture
- `https://dungeondescent.duckdns.org/` → Static Web Edition served by Caddy (with `zstd`/`gzip` compression and smart cache headers)
- `https://dungeondescent.duckdns.org/api/*` → Same-origin Global Leaderboard, Player Accounts, and Cloud Codex Sync API (`127.0.0.1:8787`, SQLite WAL at `/var/lib/dungeon-descent/dungeon_descent.sqlite`)
- **Auto-Updates:** A lightweight `systemd` timer (`dungeon-descent-update.timer`) checks GitHub `main` every 2 minutes and automatically updates static files (and restarts the API only when `server/` changes).
- **Tiny Footprint:** ~35 MB RAM total, zero npm dependencies, built-in IP rate limiting.

## 3-Step Oracle VM Setup (`129.151.172.132`)

### 1. Point `dungeondescent.duckdns.org` to Your Oracle VM
Log in to [duckdns.org](https://www.duckdns.org) and update your existing `dungeondescent` domain IPv4 address to:
- **`129.151.172.132`**

*(Only 1 DuckDNS domain needed — no second domain required!)*

### 2. Open Ports 80 & 443 in Oracle Cloud VCN Security List
In the Oracle Cloud Console:
1. Go to **Compute → Instances → your Micro VM → click your Subnet → click the Default Security List**.
2. Click **Add Ingress Rules** and add two rules:
   - Source CIDR: `0.0.0.0/0`, IP Protocol: `TCP`, Destination Port Range: `80`
   - Source CIDR: `0.0.0.0/0`, IP Protocol: `TCP`, Destination Port Range: `443`

### 3. Run the One-Command Installer on Your Oracle VM
SSH into `129.151.172.132` and run:

```bash
curl -fsSL https://raw.githubusercontent.com/ArjunRamphal/Dungeon_Descent/main/server/deploy/install_oracle_vm.sh | sudo bash
```

Verify from your PC or phone:
```bash
curl https://dungeondescent.duckdns.org/api/health
```

You should see:
```json
{"ok":true,"service":"dungeon-descent-api","version":1,"users":0,"entries":0,"uptimeSeconds":...}
```

Then open **https://dungeondescent.duckdns.org** in your browser — both the game and the Global Leaderboard / Accounts API run over HTTPS from your Oracle VM!

## Local Development
```bash
node server/server.mjs
```
Then open the Web Edition locally with `?api=http://127.0.0.1:8787`.
