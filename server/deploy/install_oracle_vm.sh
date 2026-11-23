#!/usr/bin/env bash
# One-command installer for the Dungeon Descent Global API on an Oracle Cloud Free Tier Micro VM
# Supports Ubuntu 22.04/24.04 and Oracle Linux 8/9 (x86_64 or ARM64).
#
# Usage on the Oracle VM (129.151.172.132):
#   curl -fsSL https://raw.githubusercontent.com/ArjunRamphal/Dungeon_Descent/main/server/deploy/install_oracle_vm.sh | sudo bash
#
# Optional overrides:
#   API_DOMAIN=dungeondescent-api.duckdns.org SITE_ORIGIN=https://dungeondescent.duckdns.org sudo -E bash install_oracle_vm.sh

set -euo pipefail

API_DOMAIN="${API_DOMAIN:-dungeondescent-api.duckdns.org}"
SITE_ORIGIN="${SITE_ORIGIN:-https://dungeondescent.duckdns.org}"
REPO_URL="${REPO_URL:-https://github.com/ArjunRamphal/Dungeon_Descent.git}"
INSTALL_DIR="/opt/dungeon-descent"
DATA_DIR="/var/lib/dungeon-descent"

if [[ "$(id -u)" -ne 0 ]]; then
  echo "Please run as root (e.g. with sudo)." >&2
  exit 1
fi

echo "==> Installing Dungeon Descent Global API for https://${API_DOMAIN}"

# 1. Install git, curl, Node.js 22 LTS, and Caddy
if command -v apt-get >/dev/null 2>&1; then
  export DEBIAN_FRONTEND=noninteractive
  apt-get update -y
  apt-get install -y curl git ca-certificates gnupg debian-keyring debian-archive-keyring apt-transport-https

  if ! command -v node >/dev/null 2>&1 || [[ "$(node -v | sed 's/^v//' | cut -d. -f1)" -lt 22 ]]; then
    echo "==> Installing Node.js 22 LTS..."
    curl -fsSL https://deb.nodesource.com/setup_22.x | bash -
    apt-get install -y nodejs
  fi

  if ! command -v caddy >/dev/null 2>&1; then
    echo "==> Installing Caddy (automatic HTTPS)..."
    curl -1sLf 'https://dl.cloudsmith.io/public/caddy/stable/gpg.key' | gpg --dearmor -o /usr/share/keyrings/caddy-stable-archive-keyring.gpg
    curl -1sLf 'https://dl.cloudsmith.io/public/caddy/stable/debian.deb.txt' | tee /etc/apt/sources.list.d/caddy-stable.list >/dev/null
    apt-get update -y
    apt-get install -y caddy
  fi
elif command -v dnf >/dev/null 2>&1; then
  dnf install -y curl git ca-certificates
  if ! command -v node >/dev/null 2>&1 || [[ "$(node -v | sed 's/^v//' | cut -d. -f1)" -lt 22 ]]; then
    echo "==> Installing Node.js 22 LTS..."
    curl -fsSL https://rpm.nodesource.com/setup_22.x | bash -
    dnf install -y nodejs
  fi
  if ! command -v caddy >/dev/null 2>&1; then
    echo "==> Installing Caddy (automatic HTTPS)..."
    dnf install -y 'dnf-command(copr)' || true
    dnf copr enable -y @caddy/caddy
    dnf install -y caddy
  fi
else
  echo "Unsupported Linux distro (expected apt-get or dnf)." >&2
  exit 1
fi

echo "==> Node version: $(node -v)"

# 2. Create dedicated system user & data directory
if ! id -u dungeondescent >/dev/null 2>&1; then
  useradd --system --home-dir "$DATA_DIR" --shell /usr/sbin/nologin dungeondescent
fi
mkdir -p "$DATA_DIR"
chown -R dungeondescent:dungeondescent "$DATA_DIR"
chmod 750 "$DATA_DIR"

# 3. Clone or update repository
if [[ -d "${INSTALL_DIR}/.git" ]]; then
  echo "==> Updating ${INSTALL_DIR}..."
  git -C "$INSTALL_DIR" fetch origin main
  git -C "$INSTALL_DIR" reset --hard origin/main
else
  echo "==> Cloning ${REPO_URL} into ${INSTALL_DIR}..."
  git clone --depth 1 --branch main "$REPO_URL" "$INSTALL_DIR"
fi

# 4. Install systemd service
sed "s|ALLOWED_ORIGINS=.*|ALLOWED_ORIGINS=${SITE_ORIGIN},http://localhost:8000,http://127.0.0.1:8000|" \
  "${INSTALL_DIR}/server/deploy/dungeon-descent-api.service" \
  > /etc/systemd/system/dungeon-descent-api.service

systemctl daemon-reload
systemctl enable --now dungeon-descent-api.service
systemctl restart dungeon-descent-api.service

# 5. Open ports 80 & 443 in OS firewall (Oracle Cloud Ubuntu images block them in iptables by default!)
if command -v firewall-cmd >/dev/null 2>&1 && systemctl is-active --quiet firewalld; then
  echo "==> Opening HTTP/HTTPS in firewalld..."
  firewall-cmd --permanent --add-service=http
  firewall-cmd --permanent --add-service=https
  firewall-cmd --reload
fi

if command -v ufw >/dev/null 2>&1 && ufw status | grep -q "Status: active"; then
  echo "==> Opening HTTP/HTTPS in ufw..."
  ufw allow 80/tcp
  ufw allow 443/tcp
fi

if command -v iptables >/dev/null 2>&1; then
  echo "==> Ensuring ports 80 & 443 are allowed in iptables (Oracle Cloud default rules)..."
  iptables -C INPUT -p tcp --dport 80 -j ACCEPT 2>/dev/null || iptables -I INPUT 6 -m state --state NEW -p tcp --dport 80 -j ACCEPT 2>/dev/null || iptables -I INPUT 1 -p tcp --dport 80 -j ACCEPT
  iptables -C INPUT -p tcp --dport 443 -j ACCEPT 2>/dev/null || iptables -I INPUT 6 -m state --state NEW -p tcp --dport 443 -j ACCEPT 2>/dev/null || iptables -I INPUT 1 -p tcp --dport 443 -j ACCEPT
  if command -v netfilter-persistent >/dev/null 2>&1; then
    netfilter-persistent save || true
  elif [[ -d /etc/iptables ]]; then
    iptables-save > /etc/iptables/rules.v4 || true
  fi
fi

# 6. Configure and start Caddy for automatic Let's Encrypt HTTPS
cat > /etc/caddy/Caddyfile <<EOF
${API_DOMAIN} {
    encode zstd gzip

    reverse_proxy 127.0.0.1:8787 {
        header_up X-Forwarded-For {remote_host}
    }
}
EOF

systemctl enable --now caddy
systemctl reload caddy || systemctl restart caddy

# 7. Verify local API health
sleep 2
echo "==> Local API healthcheck:"
curl -fsS http://127.0.0.1:8787/api/health
echo ""
echo ""
echo "================================================================="
echo " Dungeon Descent Global API is running!"
echo " Local backend:  http://127.0.0.1:8787/api/health"
echo " Public HTTPS:   https://${API_DOMAIN}/api/health"
echo " SQLite DB:      ${DATA_DIR}/dungeon_descent.sqlite"
echo "================================================================="
echo "Make sure:"
echo "  1) DuckDNS has '${API_DOMAIN}' pointing to 129.151.172.132"
echo "  2) Oracle Cloud Console -> VCN -> Security List allows Ingress TCP 80 and TCP 443 from 0.0.0.0/0"
