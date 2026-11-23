# Dungeon Descent — Global Leaderboard & Accounts API

Lightweight, zero-dependency Node.js 22+ API server (`node:http`, `node:crypto`, built-in `node:sqlite`) designed for an **Oracle Cloud Free Tier Micro VM (1 GB RAM)** behind **Caddy** automatic HTTPS.

## What It Provides
- **Player Accounts**: Case-insensitive unique usernames (`3–20` characters, `[A-Za-z0-9_-]`) with `scrypt` salted password hashing and 90-day opaque bearer sessions.
- **Global Leaderboards**: Ranked by `difficulty` (`Easy`, `Medium`, `Hard`, `Impossible`), filterable by Hero, Ending, or your own runs, with deterministic seed replay.
- **Cloud Codex Sync**: Lossless monotonic merge of all 4 unlocked endings, 19 milestones, career stats, hero records, and Riddle Gauntlet bests across browsers/devices.
- **Tiny Footprint**: ~30 MB RAM, SQLite WAL database (`/var/lib/dungeon-descent/dungeon_descent.sqlite`), zero npm dependencies, built-in IP rate limiting.

## 3-Step Oracle VM Setup (`129.151.172.132`)

### 1. Create the Free API Subdomain on DuckDNS
Log in to [duckdns.org](https://www.duckdns.org) and add a second free subdomain:
- **Subdomain:** `dungeondescent-api` → `dungeondescent-api.duckdns.org`
- **IPv4 Address:** `129.151.172.132`

*(Keep `dungeondescent.duckdns.org` pointing to GitHub Pages as-is.)*

### 2. Open Ports 80 & 443 in Oracle Cloud VCN Security List
In the Oracle Cloud Console:
1. Go to **Compute → Instances → your Micro VM → click your Subnet → click the Default Security List**.
2. Click **Add Ingress Rules** and add two rules:
   - Source CIDR: `0.0.0.0/0`, IP Protocol: `TCP`, Destination Port Range: `80`
   - Source CIDR: `0.0.0.0/0`, IP Protocol: `TCP`, Destination Port Range: `443`

*(Port 80 is needed by Caddy for Let's Encrypt automatic TLS renewal; port 443 serves HTTPS.)*

### 3. Run the One-Command Installer on Your Oracle VM
SSH into `129.151.172.132` and run:

```bash
curl -fsSL https://raw.githubusercontent.com/ArjunRamphal/Dungeon_Descent/main/server/deploy/install_oracle_vm.sh | sudo bash
```

Verify from your PC or phone:
```bash
curl https://dungeondescent-api.duckdns.org/api/health
```

You should see:
```json
{"ok":true,"service":"dungeon-descent-api","version":1,"users":0,"entries":0,"uptimeSeconds":...}
```

## Updating Later
SSH into the VM and re-run the same command (your SQLite database in `/var/lib/dungeon-descent/` is preserved untouched).

## Local Development
```bash
node server/server.mjs
```
Then open the Web Edition locally with `?api=http://127.0.0.1:8787` to point at your local API server.
