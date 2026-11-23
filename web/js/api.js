// Client for the Dungeon Descent Global Leaderboard, Player Accounts, and Cloud Codex API.
// Served same-origin at https://dungeondescent.duckdns.org/api/* on your Oracle Cloud Micro VM.

const AUTH_STORAGE_KEY = "dungeon_descent_auth_v1";
const API_OVERRIDE_KEY = "dungeon_descent_api_override_v1";
const DEFAULT_API_BASE = "https://dungeondescent.duckdns.org";
const REQUEST_TIMEOUT_MS = 7000;

function resolveApiBase() {
  if (typeof window === "undefined") return DEFAULT_API_BASE;
  try {
    const params = new URLSearchParams(window.location.search);
    const paramApi = params.get("api");
    if (paramApi && /^https?:\/\//i.test(paramApi.trim())) {
      const cleaned = paramApi.trim().replace(/\/+$/, "");
      window.sessionStorage?.setItem(API_OVERRIDE_KEY, cleaned);
      return cleaned;
    }
    const stored = window.sessionStorage?.getItem(API_OVERRIDE_KEY);
    if (stored && /^https?:\/\//i.test(stored)) {
      return stored.replace(/\/+$/, "");
    }
  } catch {}
  if (typeof window.DUNGEON_API_BASE === "string" && window.DUNGEON_API_BASE.trim()) {
    return window.DUNGEON_API_BASE.trim().replace(/\/+$/, "");
  }
  const host = (window.location?.hostname || "").toLowerCase();
  // When hosted directly on the Oracle VM (dungeondescent.duckdns.org), call same-origin /api/*
  if (host === "dungeondescent.duckdns.org" || host.endsWith(".dungeondescent.duckdns.org")) {
    return "";
  }
  return DEFAULT_API_BASE;
}

export class GameApiClient {
  constructor(baseUrl = null, customStorage = null) {
    this.baseUrl = (baseUrl || resolveApiBase()).replace(/\/+$/, "");
    this.storage = customStorage || (typeof window !== "undefined" ? window.localStorage : null);
    this.session = this.loadSession();
    this.online = null; // null = unknown, true = reachable, false = unreachable
    this.lastSyncAt = 0;
  }

  loadSession() {
    if (!this.storage) return null;
    try {
      const raw = this.storage.getItem(AUTH_STORAGE_KEY);
      if (!raw) return null;
      const parsed = JSON.parse(raw);
      if (parsed && typeof parsed.token === "string" && parsed.user && typeof parsed.user.username === "string") {
        return { token: parsed.token, user: parsed.user };
      }
      return null;
    } catch {
      return null;
    }
  }

  saveSession(token, user) {
    this.session = { token, user };
    try {
      this.storage?.setItem(AUTH_STORAGE_KEY, JSON.stringify(this.session));
    } catch {}
  }

  clearSession() {
    this.session = null;
    try {
      this.storage?.removeItem(AUTH_STORAGE_KEY);
    } catch {}
  }

  isSignedIn() {
    return Boolean(this.session && this.session.token && this.session.user);
  }

  getUser() {
    return this.session?.user || null;
  }

  async request(path, { method = "GET", body = null, auth = false } = {}) {
    const controller = new AbortController();
    const timer = setTimeout(() => controller.abort(), REQUEST_TIMEOUT_MS);
    const headers = { Accept: "application/json" };
    if (body !== null) headers["Content-Type"] = "application/json";
    if (auth && this.session?.token) {
      headers.Authorization = `Bearer ${this.session.token}`;
    }

    try {
      const res = await fetch(`${this.baseUrl}${path}`, {
        method,
        headers,
        body: body !== null ? JSON.stringify(body) : undefined,
        signal: controller.signal
      });
      this.online = true;
      let data = null;
      try {
        data = await res.json();
      } catch {
        data = null;
      }
      if (!res.ok) {
        const msg = data?.error || `Server returned HTTP ${res.status}.`;
        const err = new Error(msg);
        err.status = res.status;
        if (res.status === 401 && auth) {
          this.clearSession();
        }
        throw err;
      }
      return data;
    } catch (err) {
      if (err && typeof err.status === "number") throw err;
      this.online = false;
      const netErr = new Error(
        err?.name === "AbortError"
          ? "Global server timed out. Playing with local data."
          : "Could not reach the global server. Playing with local data."
      );
      netErr.network = true;
      throw netErr;
    } finally {
      clearTimeout(timer);
    }
  }

  async checkHealth() {
    return this.request("/api/health");
  }

  async register(username, password, localCodex = null) {
    const data = await this.request("/api/auth/register", {
      method: "POST",
      body: { username, password, localCodex }
    });
    this.saveSession(data.token, data.user);
    this.lastSyncAt = Date.now();
    return data;
  }

  async login(username, password, localCodex = null) {
    const data = await this.request("/api/auth/login", {
      method: "POST",
      body: { username, password, localCodex }
    });
    this.saveSession(data.token, data.user);
    this.lastSyncAt = Date.now();
    return data;
  }

  async logout() {
    try {
      if (this.session?.token) {
        await this.request("/api/auth/logout", { method: "POST", auth: true });
      }
    } catch {
      // Clear local session even if server is temporarily offline
    } finally {
      this.clearSession();
    }
  }

  async restoreSession() {
    if (!this.isSignedIn()) return null;
    const data = await this.request("/api/auth/me", { method: "GET", auth: true });
    if (data?.user) {
      this.saveSession(this.session.token, data.user);
      this.lastSyncAt = Date.now();
    }
    return data;
  }

  async syncCodex(localCodex) {
    if (!this.isSignedIn()) return null;
    const data = await this.request("/api/codex", {
      method: "PUT",
      auth: true,
      body: { codex: localCodex }
    });
    this.lastSyncAt = Date.now();
    return data.codex;
  }

  async getLeaderboard({ difficulty = 0, hero = -1, ending = 0, page = 1, pageSize = 10, mine = false } = {}) {
    const params = new URLSearchParams({
      difficulty: String(difficulty),
      hero: String(hero),
      ending: String(ending),
      page: String(page),
      pageSize: String(pageSize)
    });
    if (mine) params.set("mine", "1");
    return this.request(`/api/leaderboard?${params.toString()}`, {
      method: "GET",
      auth: mine
    });
  }

  async submitScore({ difficulty, score, hero, ending, seed, durationSeconds }) {
    return this.request("/api/leaderboard", {
      method: "POST",
      auth: true,
      body: { difficulty, score, hero, ending, seed, durationSeconds }
    });
  }
}
