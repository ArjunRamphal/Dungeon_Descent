#!/usr/bin/env bash
# Auto-sync script for Oracle VM: pulls new commits from origin/main, updates /var/www/dungeon-descent,
# and restarts the API service only when files under server/ changed.

set -euo pipefail

INSTALL_DIR="/opt/dungeon-descent"
WEB_ROOT="/var/www/dungeon-descent"

if [[ ! -d "${INSTALL_DIR}/.git" ]]; then
  exit 0
fi

git -C "$INSTALL_DIR" fetch --quiet origin main
LOCAL_SHA="$(git -C "$INSTALL_DIR" rev-parse HEAD)"
REMOTE_SHA="$(git -C "$INSTALL_DIR" rev-parse origin/main)"

if [[ "$LOCAL_SHA" == "$REMOTE_SHA" && -f "${WEB_ROOT}/index.html" && -L "${WEB_ROOT}/assets" ]]; then
  exit 0
fi

SERVER_CHANGED=0
if [[ "$LOCAL_SHA" != "$REMOTE_SHA" ]]; then
  if git -C "$INSTALL_DIR" diff --name-only "$LOCAL_SHA" "$REMOTE_SHA" | grep -q '^server/'; then
    SERVER_CHANGED=1
  fi
  git -C "$INSTALL_DIR" reset --hard --quiet origin/main
fi

mkdir -p "$WEB_ROOT"
# Stage web files atomically (keep symlink to assets to save disk space on Micro VM)
find "$WEB_ROOT" -mindepth 1 -maxdepth 1 ! -name 'assets' -exec rm -rf {} +
cp -r "${INSTALL_DIR}/web/." "$WEB_ROOT/"
ln -sfn "${INSTALL_DIR}/assets" "${WEB_ROOT}/assets"

if [[ "$SERVER_CHANGED" -eq 1 ]]; then
  systemctl restart dungeon-descent-api.service
fi
