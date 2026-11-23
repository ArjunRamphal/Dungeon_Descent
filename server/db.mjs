// SQLite (built-in node:sqlite) storage layer for Dungeon Descent Accounts, Cloud Codex, and Global Leaderboards.
// Zero external dependencies — ideal for an Oracle Free Tier Micro VM (1 GB RAM).

import fs from "node:fs";
import path from "node:path";
import crypto from "node:crypto";
import { DatabaseSync } from "node:sqlite";
import { defaultCodex, sanitizeCodex, mergeCodex } from "./codex_merge.mjs";

const SESSION_TTL_MS = 90 * 24 * 60 * 60 * 1000; // 90 days
const USERNAME_RE = /^[A-Za-z0-9][A-Za-z0-9_-]{2,19}$/;

function hashPassword(password, saltHex) {
  return crypto.scryptSync(password, saltHex, 64, { N: 16384, r: 8, p: 1 }).toString("hex");
}

function verifyPassword(password, saltHex, storedHashHex) {
  const derived = Buffer.from(hashPassword(password, saltHex), "hex");
  const expected = Buffer.from(storedHashHex, "hex");
  if (derived.length !== expected.length) return false;
  return crypto.timingSafeEqual(derived, expected);
}

function hashToken(token) {
  return crypto.createHash("sha256").update(token).digest("hex");
}

export function validateUsername(username) {
  const clean = String(username ?? "").trim();
  if (!USERNAME_RE.test(clean)) {
    throw Object.assign(
      new Error("Username must be 3–20 characters, start with a letter or number, and use only letters, numbers, _ or -."),
      { status: 400 }
    );
  }
  return clean;
}

export function validatePassword(password) {
  const raw = String(password ?? "");
  if (raw.length < 6 || raw.length > 128) {
    throw Object.assign(
      new Error("Password must be between 6 and 128 characters."),
      { status: 400 }
    );
  }
  return raw;
}

export class GameDatabase {
  constructor(dbPath) {
    if (dbPath !== ":memory:") {
      fs.mkdirSync(path.dirname(path.resolve(dbPath)), { recursive: true });
    }
    this.db = new DatabaseSync(dbPath);
    this.db.exec(`
      PRAGMA journal_mode = WAL;
      PRAGMA foreign_keys = ON;
      PRAGMA synchronous = NORMAL;
      PRAGMA busy_timeout = 5000;

      CREATE TABLE IF NOT EXISTS users (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        username TEXT NOT NULL,
        username_lower TEXT NOT NULL UNIQUE,
        password_salt TEXT NOT NULL,
        password_hash TEXT NOT NULL,
        codex_json TEXT NOT NULL,
        created_at INTEGER NOT NULL,
        updated_at INTEGER NOT NULL
      );

      CREATE TABLE IF NOT EXISTS sessions (
        token_hash TEXT PRIMARY KEY,
        user_id INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,
        created_at INTEGER NOT NULL,
        expires_at INTEGER NOT NULL,
        last_seen_at INTEGER NOT NULL
      );

      CREATE TABLE IF NOT EXISTS leaderboard_entries (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        user_id INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,
        username TEXT NOT NULL,
        difficulty INTEGER NOT NULL CHECK (difficulty BETWEEN 0 AND 3),
        score INTEGER NOT NULL CHECK (score BETWEEN 0 AND 50000),
        hero INTEGER NOT NULL CHECK (hero BETWEEN 0 AND 3),
        ending INTEGER NOT NULL CHECK (ending BETWEEN 0 AND 4),
        seed INTEGER NOT NULL CHECK (seed >= 0),
        duration_seconds INTEGER NOT NULL CHECK (duration_seconds BETWEEN 0 AND 86400),
        created_at INTEGER NOT NULL
      );

      CREATE INDEX IF NOT EXISTS idx_lb_rank
        ON leaderboard_entries(difficulty, score DESC, duration_seconds ASC, created_at ASC);
      CREATE INDEX IF NOT EXISTS idx_lb_user
        ON leaderboard_entries(user_id, created_at DESC);
      CREATE INDEX IF NOT EXISTS idx_sessions_user
        ON sessions(user_id);
      CREATE INDEX IF NOT EXISTS idx_sessions_exp
        ON sessions(expires_at);
    `);

    this.stmts = {
      findUserByLower: this.db.prepare("SELECT * FROM users WHERE username_lower = ?"),
      findUserById: this.db.prepare("SELECT * FROM users WHERE id = ?"),
      insertUser: this.db.prepare(`
        INSERT INTO users (username, username_lower, password_salt, password_hash, codex_json, created_at, updated_at)
        VALUES (?, ?, ?, ?, ?, ?, ?)
      `),
      updateCodex: this.db.prepare("UPDATE users SET codex_json = ?, updated_at = ? WHERE id = ?"),
      insertSession: this.db.prepare(`
        INSERT INTO sessions (token_hash, user_id, created_at, expires_at, last_seen_at)
        VALUES (?, ?, ?, ?, ?)
      `),
      findSession: this.db.prepare(`
        SELECT s.token_hash, s.expires_at, u.id, u.username, u.codex_json, u.created_at
        FROM sessions s
        JOIN users u ON u.id = s.user_id
        WHERE s.token_hash = ?
      `),
      touchSession: this.db.prepare("UPDATE sessions SET last_seen_at = ?, expires_at = ? WHERE token_hash = ?"),
      deleteSession: this.db.prepare("DELETE FROM sessions WHERE token_hash = ?"),
      purgeExpiredSessions: this.db.prepare("DELETE FROM sessions WHERE expires_at < ?"),
      findRecentDuplicate: this.db.prepare(`
        SELECT id FROM leaderboard_entries
        WHERE user_id = ? AND difficulty = ? AND score = ? AND hero = ? AND ending = ? AND seed = ? AND duration_seconds = ? AND created_at > ?
        LIMIT 1
      `),
      insertLeaderboard: this.db.prepare(`
        INSERT INTO leaderboard_entries (user_id, username, difficulty, score, hero, ending, seed, duration_seconds, created_at)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
      `),
      findEntryById: this.db.prepare("SELECT * FROM leaderboard_entries WHERE id = ?"),
      countHigherRank: this.db.prepare(`
        SELECT COUNT(*) AS ahead
        FROM leaderboard_entries
        WHERE difficulty = ?
          AND (
            score > ?
            OR (score = ? AND duration_seconds < ?)
            OR (score = ? AND duration_seconds = ? AND created_at < ?)
            OR (score = ? AND duration_seconds = ? AND created_at = ? AND id < ?)
          )
      `),
      countUsers: this.db.prepare("SELECT COUNT(*) AS c FROM users"),
      countEntries: this.db.prepare("SELECT COUNT(*) AS c FROM leaderboard_entries")
    };
  }

  close() {
    this.db.close();
  }

  stats() {
    return {
      users: Number(this.stmts.countUsers.get().c || 0),
      entries: Number(this.stmts.countEntries.get().c || 0)
    };
  }

  purgeExpiredSessions() {
    this.stmts.purgeExpiredSessions.run(Date.now());
  }

  createSession(userId) {
    const token = crypto.randomBytes(32).toString("base64url");
    const tokenHash = hashToken(token);
    const now = Date.now();
    this.stmts.insertSession.run(tokenHash, userId, now, now + SESSION_TTL_MS, now);
    return token;
  }

  register(usernameRaw, passwordRaw, localCodexRaw = null) {
    const username = validateUsername(usernameRaw);
    const password = validatePassword(passwordRaw);
    const lower = username.toLowerCase();

    if (this.stmts.findUserByLower.get(lower)) {
      throw Object.assign(new Error("That username is already taken."), { status: 409 });
    }

    const salt = crypto.randomBytes(16).toString("hex");
    const hash = hashPassword(password, salt);
    const codex = localCodexRaw ? sanitizeCodex(localCodexRaw) : defaultCodex();
    const now = Date.now();

    const info = this.stmts.insertUser.run(
      username, lower, salt, hash, JSON.stringify(codex), now, now
    );
    const userId = Number(info.lastInsertRowid);
    const token = this.createSession(userId);
    return {
      token,
      user: { id: userId, username, createdAt: now },
      codex
    };
  }

  login(usernameRaw, passwordRaw, localCodexRaw = null) {
    const username = String(usernameRaw ?? "").trim();
    const password = String(passwordRaw ?? "");
    if (!username || !password) {
      throw Object.assign(new Error("Enter both username and password."), { status: 400 });
    }

    const row = this.stmts.findUserByLower.get(username.toLowerCase());
    if (!row || !verifyPassword(password, row.password_salt, row.password_hash)) {
      throw Object.assign(new Error("Invalid username or password."), { status: 401 });
    }

    let codex = sanitizeCodex(JSON.parse(row.codex_json));
    if (localCodexRaw && typeof localCodexRaw === "object") {
      codex = mergeCodex(codex, localCodexRaw);
      this.stmts.updateCodex.run(JSON.stringify(codex), Date.now(), row.id);
    }

    const token = this.createSession(row.id);
    return {
      token,
      user: { id: Number(row.id), username: row.username, createdAt: Number(row.created_at) },
      codex
    };
  }

  authenticate(token) {
    if (!token || typeof token !== "string") return null;
    const tokenHash = hashToken(token);
    const row = this.stmts.findSession.get(tokenHash);
    if (!row) return null;
    const now = Date.now();
    if (Number(row.expires_at) < now) {
      this.stmts.deleteSession.run(tokenHash);
      return null;
    }
    this.stmts.touchSession.run(now, now + SESSION_TTL_MS, tokenHash);
    return {
      tokenHash,
      user: { id: Number(row.id), username: row.username, createdAt: Number(row.created_at) },
      codex: sanitizeCodex(JSON.parse(row.codex_json))
    };
  }

  logout(token) {
    if (!token) return;
    this.stmts.deleteSession.run(hashToken(token));
  }

  syncCodex(userId, incomingCodexRaw) {
    const row = this.stmts.findUserById.get(userId);
    if (!row) throw Object.assign(new Error("Account not found."), { status: 404 });
    const current = sanitizeCodex(JSON.parse(row.codex_json));
    const merged = mergeCodex(current, incomingCodexRaw);
    this.stmts.updateCodex.run(JSON.stringify(merged), Date.now(), userId);
    return merged;
  }

  submitScore(user, payload) {
    const difficulty = Math.trunc(Number(payload?.difficulty));
    const score = Math.round(Number(payload?.score));
    const hero = Math.trunc(Number(payload?.hero));
    const ending = Math.trunc(Number(payload?.ending));
    const seed = Number(payload?.seed) >>> 0;
    const durationSeconds = Math.max(0, Math.round(Number(payload?.durationSeconds)));

    if (!Number.isInteger(difficulty) || difficulty < 0 || difficulty > 3) {
      throw Object.assign(new Error("Invalid difficulty."), { status: 400 });
    }
    if (!Number.isFinite(score) || score < 0 || score > 50000) {
      throw Object.assign(new Error("Invalid score."), { status: 400 });
    }
    if (!Number.isInteger(hero) || hero < 0 || hero > 3) {
      throw Object.assign(new Error("Invalid hero."), { status: 400 });
    }
    if (!Number.isInteger(ending) || ending < 0 || ending > 4) {
      throw Object.assign(new Error("Invalid ending."), { status: 400 });
    }
    if (!Number.isFinite(durationSeconds) || durationSeconds < 0 || durationSeconds > 86400) {
      throw Object.assign(new Error("Invalid duration."), { status: 400 });
    }

    const now = Date.now();
    const dup = this.stmts.findRecentDuplicate.get(
      user.id, difficulty, score, hero, ending, seed, durationSeconds, now - 15000
    );
    let entryId;
    if (dup) {
      entryId = Number(dup.id);
    } else {
      const info = this.stmts.insertLeaderboard.run(
        user.id, user.username, difficulty, score, hero, ending, seed, durationSeconds, now
      );
      entryId = Number(info.lastInsertRowid);
    }

    const row = this.stmts.findEntryById.get(entryId);
    const rankRow = this.stmts.countHigherRank.get(
      row.difficulty,
      row.score,
      row.score, row.duration_seconds,
      row.score, row.duration_seconds, row.created_at,
      row.score, row.duration_seconds, row.created_at, row.id
    );
    const rank = Number(rankRow.ahead || 0) + 1;

    return {
      rank,
      entry: {
        id: Number(row.id),
        rank,
        userId: Number(row.user_id),
        username: row.username,
        difficulty: Number(row.difficulty),
        score: Number(row.score),
        hero: Number(row.hero),
        ending: Number(row.ending),
        seed: Number(row.seed),
        durationSeconds: Number(row.duration_seconds),
        createdAt: Number(row.created_at)
      }
    };
  }

  queryLeaderboard({ difficulty = 0, hero = -1, ending = 0, page = 1, pageSize = 10, userId = null } = {}) {
    const diff = Math.max(0, Math.min(3, Math.trunc(Number(difficulty) || 0)));
    const heroFilter = Math.trunc(Number(hero));
    const endingFilter = Math.trunc(Number(ending));
    const size = Math.max(1, Math.min(50, Math.trunc(Number(pageSize) || 10)));
    const reqPage = Math.max(1, Math.trunc(Number(page) || 1));

    const where = ["difficulty = ?"];
    const params = [diff];

    if (Number.isInteger(heroFilter) && heroFilter >= 0 && heroFilter <= 3) {
      where.push("hero = ?");
      params.push(heroFilter);
    }
    if (Number.isInteger(endingFilter) && endingFilter >= 1 && endingFilter <= 4) {
      where.push("ending = ?");
      params.push(endingFilter);
    }
    if (userId !== null && Number.isInteger(userId)) {
      where.push("user_id = ?");
      params.push(userId);
    }

    const whereSql = where.join(" AND ");
    const countRow = this.db
      .prepare(`SELECT COUNT(*) AS total FROM leaderboard_entries WHERE ${whereSql}`)
      .get(...params);
    const totalEntries = Number(countRow.total || 0);
    const totalPages = Math.max(1, Math.ceil(totalEntries / size));
    const safePage = Math.min(reqPage, totalPages);
    const offset = (safePage - 1) * size;

    // Compute each entry's true global rank on its difficulty board using SQLite window function
    const rows = this.db.prepare(`
      WITH ranked AS (
        SELECT
          id, user_id, username, difficulty, score, hero, ending, seed, duration_seconds, created_at,
          ROW_NUMBER() OVER (
            ORDER BY score DESC, duration_seconds ASC, created_at ASC, id ASC
          ) AS global_rank
        FROM leaderboard_entries
        WHERE difficulty = ?
      )
      SELECT *
      FROM ranked
      WHERE ${whereSql}
      ORDER BY global_rank ASC
      LIMIT ? OFFSET ?
    `).all(diff, ...params, size, offset);

    return {
      difficulty: diff,
      hero: Number.isInteger(heroFilter) && heroFilter >= 0 && heroFilter <= 3 ? heroFilter : -1,
      ending: Number.isInteger(endingFilter) && endingFilter >= 1 && endingFilter <= 4 ? endingFilter : 0,
      page: safePage,
      pageSize: size,
      totalPages,
      totalEntries,
      entries: rows.map((r) => ({
        id: Number(r.id),
        rank: Number(r.global_rank),
        userId: Number(r.user_id),
        username: r.username,
        difficulty: Number(r.difficulty),
        score: Number(r.score),
        hero: Number(r.hero),
        ending: Number(r.ending),
        seed: Number(r.seed),
        durationSeconds: Number(r.duration_seconds),
        createdAt: Number(r.created_at)
      }))
    };
  }
}
