// Dungeon Descent Global Leaderboard & Accounts API Server
// Zero external dependencies (Node.js 22+ built-ins only) — lightweight for Oracle Free Tier Micro VM.

import fs from "node:fs";
import http from "node:http";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { GameDatabase } from "./db.mjs";

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const MAX_BODY_BYTES = 32 * 1024; // 32 KB cap protects 1 GB Micro VM

const STATIC_MIME = {
  ".html": "text/html; charset=utf-8",
  ".css": "text/css; charset=utf-8",
  ".js": "application/javascript; charset=utf-8",
  ".mjs": "application/javascript; charset=utf-8",
  ".json": "application/json; charset=utf-8",
  ".txt": "text/plain; charset=utf-8",
  ".png": "image/png",
  ".jpg": "image/jpeg",
  ".jpeg": "image/jpeg",
  ".webp": "image/webp",
  ".svg": "image/svg+xml",
  ".ico": "image/x-icon",
  ".ttf": "font/ttf",
  ".woff2": "font/woff2",
  ".ogg": "audio/ogg",
  ".mp3": "audio/mpeg",
  ".wav": "audio/wav"
};

function serveStaticFile(req, res, staticRoot, pathname) {
  let decoded;
  try {
    decoded = decodeURIComponent(pathname);
  } catch {
    res.writeHead(400);
    res.end("Bad Request");
    return true;
  }
  const rel = decoded === "/" ? "index.html" : decoded.replace(/^\/+/, "");
  const fullPath = path.resolve(staticRoot, rel);
  const rootPrefix = staticRoot.endsWith(path.sep) ? staticRoot : staticRoot + path.sep;
  if (fullPath !== staticRoot && !fullPath.startsWith(rootPrefix)) {
    res.writeHead(404);
    res.end("Not Found");
    return true;
  }

  let stat;
  try {
    stat = fs.statSync(fullPath);
  } catch {
    res.writeHead(404);
    res.end("Not Found");
    return true;
  }
  if (!stat.isFile()) {
    res.writeHead(404);
    res.end("Not Found");
    return true;
  }

  const ext = path.extname(fullPath).toLowerCase();
  const contentType = STATIC_MIME[ext] || "application/octet-stream";
  const cacheControl = rel.startsWith("assets/") ? "public, max-age=604800" : "no-cache";
  const size = stat.size;
  const range = req.headers.range;

  res.setHeader("X-Content-Type-Options", "nosniff");
  res.setHeader("Accept-Ranges", "bytes");
  res.setHeader("Cache-Control", cacheControl);
  res.setHeader("Content-Type", contentType);

  if (range && /^bytes=\d*-\d*$/.test(range)) {
    const [startStr, endStr] = range.replace(/^bytes=/, "").split("-");
    const start = startStr ? Number(startStr) : Math.max(0, size - Number(endStr));
    const end = endStr ? Math.min(size - 1, Number(endStr)) : size - 1;
    if (!Number.isFinite(start) || !Number.isFinite(end) || start > end || start >= size) {
      res.writeHead(416, { "Content-Range": `bytes */${size}` });
      res.end();
      return true;
    }
    res.writeHead(206, {
      "Content-Range": `bytes ${start}-${end}/${size}`,
      "Content-Length": end - start + 1
    });
    if (req.method === "HEAD") {
      res.end();
      return true;
    }
    fs.createReadStream(fullPath, { start, end }).pipe(res);
    return true;
  }

  res.writeHead(200, { "Content-Length": size });
  if (req.method === "HEAD") {
    res.end();
    return true;
  }
  fs.createReadStream(fullPath).pipe(res);
  return true;
}

class RateLimiter {
  constructor() {
    this.buckets = new Map();
    this.lastSweep = Date.now();
  }

  check(key, limit, windowMs) {
    const now = Date.now();
    if (now - this.lastSweep > 60_000) {
      for (const [k, b] of this.buckets) {
        if (b.resetAt <= now) this.buckets.delete(k);
      }
      this.lastSweep = now;
    }
    let entry = this.buckets.get(key);
    if (!entry || entry.resetAt <= now) {
      entry = { count: 0, resetAt: now + windowMs };
      this.buckets.set(key, entry);
    }
    entry.count++;
    if (entry.count > limit) {
      const retrySec = Math.max(1, Math.ceil((entry.resetAt - now) / 1000));
      return { allowed: false, retrySec };
    }
    return { allowed: true, retrySec: 0 };
  }
}

function parseAllowedOrigins(envVal) {
  const raw = envVal || [
    "https://dungeondescent.duckdns.org",
    "http://dungeondescent.duckdns.org",
    "https://arjunramphal.github.io",
    "http://localhost:8000",
    "http://127.0.0.1:8000",
    "http://localhost:5173",
    "http://127.0.0.1:5173"
  ].join(",");
  return raw.split(",").map((s) => s.trim()).filter(Boolean);
}

function isOriginAllowed(origin, allowed) {
  if (!origin) return false;
  if (allowed.includes("*")) return true;
  if (allowed.includes(origin)) return true;
  // Allow any localhost / 127.0.0.1 dev port if localhost is enabled
  if (allowed.some((o) => o.startsWith("http://localhost") || o.startsWith("http://127.0.0.1"))) {
    if (/^http:\/\/(localhost|127\.0\.0\.1)(:\d+)?$/.test(origin)) return true;
  }
  return false;
}

function getClientIp(req) {
  const xff = req.headers["x-forwarded-for"];
  if (typeof xff === "string" && xff.trim()) {
    return xff.split(",")[0].trim();
  }
  return req.socket?.remoteAddress || "unknown";
}

function getBearerToken(req) {
  const auth = req.headers.authorization;
  if (typeof auth !== "string") return null;
  const match = /^Bearer\s+(.+)$/i.exec(auth.trim());
  return match ? match[1].trim() : null;
}

async function readJsonBody(req) {
  const chunks = [];
  let total = 0;
  for await (const chunk of req) {
    total += chunk.length;
    if (total > MAX_BODY_BYTES) {
      throw Object.assign(new Error("Request body too large."), { status: 413 });
    }
    chunks.push(chunk);
  }
  if (total === 0) return {};
  try {
    return JSON.parse(Buffer.concat(chunks).toString("utf8"));
  } catch {
    throw Object.assign(new Error("Invalid JSON body."), { status: 400 });
  }
}

export function createApiServer({
  dbPath = path.join(__dirname, "data", "dungeon_descent.sqlite"),
  staticDir = process.env.STATIC_DIR || "",
  allowedOrigins = parseAllowedOrigins(process.env.ALLOWED_ORIGINS),
  enableRateLimit = true
} = {}) {
  const db = new GameDatabase(dbPath);
  const resolvedStaticDir = staticDir ? path.resolve(staticDir) : "";
  const limiter = new RateLimiter();
  const startedAt = Date.now();

  const purgeTimer = setInterval(() => {
    try { db.purgeExpiredSessions(); } catch {}
  }, 6 * 60 * 60 * 1000);
  purgeTimer.unref();

  function applyHeaders(req, res, noStore = false) {
    const origin = req.headers.origin;
    if (origin && isOriginAllowed(origin, allowedOrigins)) {
      res.setHeader("Access-Control-Allow-Origin", origin);
      res.setHeader("Vary", "Origin");
      res.setHeader("Access-Control-Allow-Methods", "GET, POST, PUT, OPTIONS");
      res.setHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
      res.setHeader("Access-Control-Max-Age", "86400");
    }
    res.setHeader("X-Content-Type-Options", "nosniff");
    res.setHeader("X-Frame-Options", "DENY");
    res.setHeader("Referrer-Policy", "no-referrer");
    if (noStore) res.setHeader("Cache-Control", "no-store");
  }

  function sendJson(req, res, status, data, noStore = false) {
    applyHeaders(req, res, noStore);
    const body = JSON.stringify(data);
    res.writeHead(status, {
      "Content-Type": "application/json; charset=utf-8",
      "Content-Length": Buffer.byteLength(body)
    });
    res.end(body);
  }

  const server = http.createServer(async (req, res) => {
    const method = req.method || "GET";
    const url = new URL(req.url || "/", "http://localhost");
    const pathname = url.pathname;
    const ip = getClientIp(req);

    // Serve static Web Edition files for non-/api/* requests when STATIC_DIR is configured
    if (resolvedStaticDir && !pathname.startsWith("/api/") && (method === "GET" || method === "HEAD")) {
      return serveStaticFile(req, res, resolvedStaticDir, pathname);
    }

    if (method === "OPTIONS") {
      applyHeaders(req, res);
      res.writeHead(204);
      res.end();
      return;
    }

    try {
      if (enableRateLimit) {
        const general = limiter.check(`gen:${ip}`, 240, 60_000);
        if (!general.allowed) {
          res.setHeader("Retry-After", String(general.retrySec));
          return sendJson(req, res, 429, { error: "Too many requests. Please slow down." }, true);
        }
      }

      // GET /api/health
      if (method === "GET" && pathname === "/api/health") {
        const stats = db.stats();
        return sendJson(req, res, 200, {
          ok: true,
          service: "dungeon-descent-api",
          version: 1,
          users: stats.users,
          entries: stats.entries,
          uptimeSeconds: Math.floor((Date.now() - startedAt) / 1000)
        });
      }

      // POST /api/auth/register
      if (method === "POST" && pathname === "/api/auth/register") {
        if (enableRateLimit) {
          const rl = limiter.check(`auth:${ip}`, 25, 15 * 60_000);
          if (!rl.allowed) {
            res.setHeader("Retry-After", String(rl.retrySec));
            return sendJson(req, res, 429, { error: "Too many account attempts. Try again later." }, true);
          }
        }
        const body = await readJsonBody(req);
        const result = db.register(body.username, body.password, body.localCodex);
        return sendJson(req, res, 201, result, true);
      }

      // POST /api/auth/login
      if (method === "POST" && pathname === "/api/auth/login") {
        if (enableRateLimit) {
          const rl = limiter.check(`auth:${ip}`, 30, 15 * 60_000);
          if (!rl.allowed) {
            res.setHeader("Retry-After", String(rl.retrySec));
            return sendJson(req, res, 429, { error: "Too many sign-in attempts. Try again later." }, true);
          }
        }
        const body = await readJsonBody(req);
        const result = db.login(body.username, body.password, body.localCodex);
        return sendJson(req, res, 200, result, true);
      }

      // POST /api/auth/logout
      if (method === "POST" && pathname === "/api/auth/logout") {
        const token = getBearerToken(req);
        if (token) db.logout(token);
        return sendJson(req, res, 200, { ok: true }, true);
      }

      // GET /api/auth/me
      if (method === "GET" && pathname === "/api/auth/me") {
        const auth = db.authenticate(getBearerToken(req));
        if (!auth) return sendJson(req, res, 401, { error: "Not signed in." }, true);
        return sendJson(req, res, 200, { user: auth.user, codex: auth.codex }, true);
      }

      // PUT /api/codex
      if (method === "PUT" && pathname === "/api/codex") {
        const auth = db.authenticate(getBearerToken(req));
        if (!auth) return sendJson(req, res, 401, { error: "Sign in to sync your Codex." }, true);
        const body = await readJsonBody(req);
        const merged = db.syncCodex(auth.user.id, body.codex ?? body);
        return sendJson(req, res, 200, { codex: merged }, true);
      }

      // GET /api/leaderboard
      if (method === "GET" && pathname === "/api/leaderboard") {
        const mineOnly = url.searchParams.get("mine") === "1";
        let filterUserId = null;
        if (mineOnly) {
          const auth = db.authenticate(getBearerToken(req));
          if (!auth) return sendJson(req, res, 401, { error: "Sign in to filter your own runs." }, true);
          filterUserId = auth.user.id;
        }
        const result = db.queryLeaderboard({
          difficulty: url.searchParams.get("difficulty") ?? "0",
          hero: url.searchParams.get("hero") ?? "-1",
          ending: url.searchParams.get("ending") ?? "0",
          page: url.searchParams.get("page") ?? "1",
          pageSize: url.searchParams.get("pageSize") ?? "10",
          userId: filterUserId
        });
        return sendJson(req, res, 200, result);
      }

      // POST /api/leaderboard
      if (method === "POST" && pathname === "/api/leaderboard") {
        if (enableRateLimit) {
          const rl = limiter.check(`score:${ip}`, 40, 15 * 60_000);
          if (!rl.allowed) {
            res.setHeader("Retry-After", String(rl.retrySec));
            return sendJson(req, res, 429, { error: "Too many score submissions. Please wait." }, true);
          }
        }
        const auth = db.authenticate(getBearerToken(req));
        if (!auth) return sendJson(req, res, 401, { error: "Sign in to post scores to the Global Leaderboard." }, true);
        const body = await readJsonBody(req);
        const result = db.submitScore(auth.user, body);
        return sendJson(req, res, 201, result, true);
      }

      return sendJson(req, res, 404, { error: "Not found." });
    } catch (err) {
      const status = Number(err?.status) || 500;
      const message = status >= 500 ? "Internal server error." : err.message;
      if (status >= 500) {
        console.error("[dungeon-descent-api]", err);
      }
      return sendJson(req, res, status, { error: message }, true);
    }
  });

  server.on("close", () => {
    clearInterval(purgeTimer);
    db.close();
  });

  return { server, db };
}

// Run directly when executed as main entry point
if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  const host = process.env.HOST || "127.0.0.1";
  const port = Number(process.env.PORT || 8787);
  const dbPath = process.env.DB_PATH || path.join(__dirname, "data", "dungeon_descent.sqlite");
  const { server } = createApiServer({ dbPath });
  server.listen(port, host, () => {
    console.log(`Dungeon Descent API listening on http://${host}:${port} (DB: ${dbPath})`);
  });
  const shutdown = () => {
    server.close(() => process.exit(0));
  };
  process.on("SIGINT", shutdown);
  process.on("SIGTERM", shutdown);
}
