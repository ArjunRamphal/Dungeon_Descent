import assert from "node:assert/strict";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { createApiServer } from "../server/server.mjs";
import { mergeCodex } from "../server/codex_merge.mjs";

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const { server } = createApiServer({
  dbPath: ":memory:",
  staticDir: path.resolve(__dirname, "..", "web"),
  allowedOrigins: ["https://dungeondescent.duckdns.org", "http://127.0.0.1:8000"],
  enableRateLimit: false
});

await new Promise((resolve) => server.listen(0, "127.0.0.1", resolve));
const { port } = server.address();
const baseUrl = `http://127.0.0.1:${port}`;

async function api(path, { method = "GET", body = null, token = null, origin = "https://dungeondescent.duckdns.org" } = {}) {
  const headers = { Accept: "application/json" };
  if (origin) headers.Origin = origin;
  if (body !== null) headers["Content-Type"] = "application/json";
  if (token) headers.Authorization = `Bearer ${token}`;
  const res = await fetch(`${baseUrl}${path}`, {
    method,
    headers,
    body: body !== null ? JSON.stringify(body) : undefined
  });
  const data = res.status === 204 ? null : await res.json();
  return { status: res.status, headers: res.headers, data };
}

try {
  // 1. Health & CORS check
  const health = await api("/api/health");
  assert.equal(health.status, 200);
  assert.equal(health.data.ok, true);
  assert.equal(health.headers.get("access-control-allow-origin"), "https://dungeondescent.duckdns.org");

  // 2. Validation failures on register
  const badUser = await api("/api/auth/register", { method: "POST", body: { username: "ab", password: "password123" } });
  assert.equal(badUser.status, 400);
  const badPass = await api("/api/auth/register", { method: "POST", body: { username: "Arjun", password: "123" } });
  assert.equal(badPass.status, 400);

  // 3. Register Account #1 (Arjun) with seeded localCodex
  const reg1 = await api("/api/auth/register", {
    method: "POST",
    body: {
      username: "Arjun",
      password: "secret-password-1",
      localCodex: {
        unlockedEndingsMask: 0b0001, // Good ending
        totalRuns: 3,
        totalVictories: 2,
        noAbilityDifficultyMask: 0b0001, // Pure Steel: Easy
        heroVictories: [2, 0, 0, 0],
        heroBestScore: [4200, 0, 0, 0],
        fastestVictorySeconds: 420,
        bestGauntletScore: 14
      }
    }
  });
  assert.equal(reg1.status, 201);
  assert.equal(reg1.data.user.username, "Arjun");
  assert.ok(reg1.data.token.length >= 32);
  assert.equal(reg1.data.codex.unlockedEndingsMask, 0b0001);
  assert.equal(reg1.data.codex.fastestVictorySeconds, 420);

  // Duplicate case-insensitive username rejected
  const dupUser = await api("/api/auth/register", {
    method: "POST",
    body: { username: "arjun", password: "another-password" }
  });
  assert.equal(dupUser.status, 409);

  // 4. Register Account #2 (ShadowBlade)
  const reg2 = await api("/api/auth/register", {
    method: "POST",
    body: { username: "ShadowBlade", password: "secret-password-2" }
  });
  assert.equal(reg2.status, 201);
  assert.equal(reg2.data.user.username, "ShadowBlade");

  // 5. Login case-insensitively with offline localCodex -> verifies monotonic merge
  const badLogin = await api("/api/auth/login", {
    method: "POST",
    body: { username: "ARJUN", password: "wrong-password" }
  });
  assert.equal(badLogin.status, 401);

  const login1 = await api("/api/auth/login", {
    method: "POST",
    body: {
      username: "arjun",
      password: "secret-password-1",
      localCodex: {
        unlockedEndingsMask: 0b1000, // Perfect ending unlocked offline on another browser!
        totalRuns: 1, // Lower than cloud's 3 — must NOT overwrite cloud's 3
        totalVictories: 1,
        noAbilityDifficultyMask: 0b1000, // Pure Steel: Impossible
        heroVictories: [0, 1, 0, 0],
        heroBestScore: [3100, 5800, 0, 0],
        fastestVictorySeconds: 315, // Faster than cloud's 420 — must win!
        bestGauntletScore: 8 // Lower than cloud's 14 — must keep 14!
      }
    }
  });
  assert.equal(login1.status, 200);
  assert.equal(login1.data.user.username, "Arjun"); // preserves original casing
  assert.equal(login1.data.codex.unlockedEndingsMask, 0b1001);
  assert.equal(login1.data.codex.noAbilityDifficultyMask, 0b1001);
  assert.equal(login1.data.codex.totalRuns, 3);
  assert.deepEqual(login1.data.codex.heroVictories, [2, 1, 0, 0]);
  assert.deepEqual(login1.data.codex.heroBestScore, [4200, 5800, 0, 0]);
  assert.equal(login1.data.codex.fastestVictorySeconds, 315);
  assert.equal(login1.data.codex.bestGauntletScore, 14);

  // 6. PUT /api/codex monotonic merge & GET /api/auth/me
  const syncRes = await api("/api/codex", {
    method: "PUT",
    token: login1.data.token,
    body: { codex: { unlockedEndingsMask: 0b0010, evilRouteCompleted: true } }
  });
  assert.equal(syncRes.status, 200);
  assert.equal(syncRes.data.codex.unlockedEndingsMask, 0b1011);
  assert.equal(syncRes.data.codex.evilRouteCompleted, true);

  const meRes = await api("/api/auth/me", { token: login1.data.token });
  assert.equal(meRes.status, 200);
  assert.equal(meRes.data.user.username, "Arjun");
  assert.equal(meRes.data.codex.unlockedEndingsMask, 0b1011);

  // 7. Global Leaderboard submissions (auth required, ranking & filtering)
  const guestPost = await api("/api/leaderboard", {
    method: "POST",
    body: { difficulty: 0, score: 5000, hero: 0, ending: 1, seed: 42, durationSeconds: 200 }
  });
  assert.equal(guestPost.status, 401);

  const s1 = await api("/api/leaderboard", {
    method: "POST",
    token: login1.data.token,
    body: { difficulty: 0, score: 4200, hero: 0, ending: 1, seed: 101, durationSeconds: 300 }
  });
  assert.equal(s1.status, 201);
  assert.equal(s1.data.rank, 1);
  assert.equal(s1.data.entry.username, "Arjun");

  // Rapid duplicate submit returns same entry instead of creating a duplicate
  const s1Dup = await api("/api/leaderboard", {
    method: "POST",
    token: login1.data.token,
    body: { difficulty: 0, score: 4200, hero: 0, ending: 1, seed: 101, durationSeconds: 300 }
  });
  assert.equal(s1Dup.status, 201);
  assert.equal(s1Dup.data.entry.id, s1.data.entry.id);

  const s2 = await api("/api/leaderboard", {
    method: "POST",
    token: reg2.data.token,
    body: { difficulty: 0, score: 5950, hero: 3, ending: 4, seed: 777, durationSeconds: 260 }
  });
  assert.equal(s2.status, 201);
  assert.equal(s2.data.rank, 1);
  assert.equal(s2.data.entry.username, "ShadowBlade");

  // Query full Easy global board
  const lbAll = await api("/api/leaderboard?difficulty=0");
  assert.equal(lbAll.status, 200);
  assert.equal(lbAll.data.totalEntries, 2);
  assert.equal(lbAll.data.entries[0].username, "ShadowBlade");
  assert.equal(lbAll.data.entries[0].rank, 1);
  assert.equal(lbAll.data.entries[1].username, "Arjun");
  assert.equal(lbAll.data.entries[1].rank, 2);

  // Filter by hero=0 (Warrior) preserves true global rank (#2)!
  const lbHero = await api("/api/leaderboard?difficulty=0&hero=0");
  assert.equal(lbHero.status, 200);
  assert.equal(lbHero.data.totalEntries, 1);
  assert.equal(lbHero.data.entries[0].username, "Arjun");
  assert.equal(lbHero.data.entries[0].rank, 2);

  // Filter mine=1 returns only signed-in user's runs
  const lbMine = await api("/api/leaderboard?difficulty=0&mine=1", { token: login1.data.token });
  assert.equal(lbMine.status, 200);
  assert.equal(lbMine.data.totalEntries, 1);
  assert.equal(lbMine.data.entries[0].username, "Arjun");

  // 8. Logout invalidates token
  const logoutRes = await api("/api/auth/logout", { method: "POST", token: login1.data.token });
  assert.equal(logoutRes.status, 200);
  const afterLogout = await api("/api/auth/me", { token: login1.data.token });
  assert.equal(afterLogout.status, 401);

  // 9. Verify pure mergeCodex helper sanity (including prequel fields)
  const merged = mergeCodex(
    { fastestVictorySeconds: 0, unlockedEndingsMask: 1, perfectImpossibleUnlocked: true, prequelCodasMask: 0b001 },
    { fastestVictorySeconds: 180, unlockedEndingsMask: 4, prequelCompleted: true, prequelCodasMask: 0b110 }
  );
  assert.equal(merged.fastestVictorySeconds, 180);
  assert.equal(merged.unlockedEndingsMask, 5);
  assert.equal(merged.perfectImpossibleUnlocked, true);
  assert.equal(merged.prequelCompleted, true);
  assert.equal(merged.prequelCodasMask, 0b111);

  // 10. Verify static Web Edition serving, Range requests, and path traversal protection
  const indexRes = await fetch(`${baseUrl}/`);
  assert.equal(indexRes.status, 200);
  assert.match(indexRes.headers.get("content-type") || "", /text\/html/);
  const html = await indexRes.text();
  assert.match(html, /Dungeon Descent/i);

  const rangeRes = await fetch(`${baseUrl}/index.html`, { headers: { Range: "bytes=0-14" } });
  assert.equal(rangeRes.status, 206);
  assert.equal((await rangeRes.text()).length, 15);

  const traversalRes = await fetch(`${baseUrl}/..%2Fserver%2Fserver.mjs`);
  assert.equal(traversalRes.status, 404);

  console.log("Global API, Accounts, Leaderboard, Static Serving & Cloud Codex Tests PASSED!");
} finally {
  await new Promise((resolve) => server.close(resolve));
}
