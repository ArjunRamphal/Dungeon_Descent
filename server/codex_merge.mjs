// Shared Codex sanitization and monotonic merge rules for Cloud Sync.
// Guarantees that signing in on any device never overwrites milestones, endings, or bests.

export function defaultCodex() {
  return {
    unlockedEndingsMask: 0,
    totalRuns: 0,
    totalVictories: 0,
    totalDefeats: 0,
    dungeonDeaths: 0,
    impossibleVictories: 0,
    evilRouteCompleted: false,
    noAbilityDifficultyMask: 0,
    malvelNoAbilityDifficultyMask: 0,
    heroVictories: [0, 0, 0, 0],
    heroBestScore: [0, 0, 0, 0],
    totalRiddlesAnswered: 0,
    totalRiddlesCorrect: 0,
    bestRiddleStreak: 0,
    fastestVictorySeconds: 0,
    bestGauntletScore: 0,
    bestGauntletStreak: 0,
    perfectImpossibleUnlocked: false,
    prequelCompleted: false,
    prequelCodasMask: 0
  };
}

function clampInt(val, min, max) {
  const n = Math.trunc(Number(val));
  if (!Number.isFinite(n)) return min;
  return Math.max(min, Math.min(max, n));
}

function sanitizeHeroArray(arr, maxVal) {
  const out = [0, 0, 0, 0];
  if (!Array.isArray(arr)) return out;
  for (let i = 0; i < 4; i++) {
    out[i] = clampInt(arr[i], 0, maxVal);
  }
  return out;
}

export function sanitizeCodex(raw) {
  const def = defaultCodex();
  if (!raw || typeof raw !== "object") return def;

  const totalRiddlesAnswered = clampInt(raw.totalRiddlesAnswered, 0, 10000000);
  const totalRiddlesCorrect = clampInt(raw.totalRiddlesCorrect, 0, totalRiddlesAnswered);

  return {
    unlockedEndingsMask: clampInt(raw.unlockedEndingsMask, 0, 15) & 15,
    totalRuns: clampInt(raw.totalRuns, 0, 1000000),
    totalVictories: clampInt(raw.totalVictories, 0, 1000000),
    totalDefeats: clampInt(raw.totalDefeats, 0, 1000000),
    dungeonDeaths: clampInt(raw.dungeonDeaths, 0, 1000000),
    impossibleVictories: clampInt(raw.impossibleVictories, 0, 1000000),
    evilRouteCompleted: Boolean(raw.evilRouteCompleted),
    noAbilityDifficultyMask: clampInt(raw.noAbilityDifficultyMask, 0, 15) & 15,
    malvelNoAbilityDifficultyMask: clampInt(raw.malvelNoAbilityDifficultyMask, 0, 15) & 15,
    heroVictories: sanitizeHeroArray(raw.heroVictories, 1000000),
    heroBestScore: sanitizeHeroArray(raw.heroBestScore, 50000),
    totalRiddlesAnswered,
    totalRiddlesCorrect,
    bestRiddleStreak: clampInt(raw.bestRiddleStreak, 0, 10000),
    fastestVictorySeconds: clampInt(raw.fastestVictorySeconds, 0, 86400),
    bestGauntletScore: clampInt(raw.bestGauntletScore, 0, 102),
    bestGauntletStreak: clampInt(raw.bestGauntletStreak, 0, 102),
    perfectImpossibleUnlocked: Boolean(raw.perfectImpossibleUnlocked),
    prequelCompleted: Boolean(raw.prequelCompleted),
    prequelCodasMask: clampInt(raw.prequelCodasMask, 0, 7) & 7
  };
}

export function mergeCodex(aRaw, bRaw) {
  const a = sanitizeCodex(aRaw);
  const b = sanitizeCodex(bRaw);

  const fastest = (a.fastestVictorySeconds > 0 && b.fastestVictorySeconds > 0)
    ? Math.min(a.fastestVictorySeconds, b.fastestVictorySeconds)
    : Math.max(a.fastestVictorySeconds, b.fastestVictorySeconds);

  const totalRiddlesAnswered = Math.max(a.totalRiddlesAnswered, b.totalRiddlesAnswered);
  const totalRiddlesCorrect = Math.min(
    totalRiddlesAnswered,
    Math.max(a.totalRiddlesCorrect, b.totalRiddlesCorrect)
  );

  return {
    unlockedEndingsMask: (a.unlockedEndingsMask | b.unlockedEndingsMask) & 15,
    totalRuns: Math.max(a.totalRuns, b.totalRuns),
    totalVictories: Math.max(a.totalVictories, b.totalVictories),
    totalDefeats: Math.max(a.totalDefeats, b.totalDefeats),
    dungeonDeaths: Math.max(a.dungeonDeaths, b.dungeonDeaths),
    impossibleVictories: Math.max(a.impossibleVictories, b.impossibleVictories),
    evilRouteCompleted: Boolean(a.evilRouteCompleted || b.evilRouteCompleted),
    noAbilityDifficultyMask: (a.noAbilityDifficultyMask | b.noAbilityDifficultyMask) & 15,
    malvelNoAbilityDifficultyMask: (a.malvelNoAbilityDifficultyMask | b.malvelNoAbilityDifficultyMask) & 15,
    heroVictories: [0, 1, 2, 3].map((i) => Math.max(a.heroVictories[i], b.heroVictories[i])),
    heroBestScore: [0, 1, 2, 3].map((i) => Math.max(a.heroBestScore[i], b.heroBestScore[i])),
    totalRiddlesAnswered,
    totalRiddlesCorrect,
    bestRiddleStreak: Math.max(a.bestRiddleStreak, b.bestRiddleStreak),
    fastestVictorySeconds: fastest,
    bestGauntletScore: Math.max(a.bestGauntletScore, b.bestGauntletScore),
    bestGauntletStreak: Math.max(a.bestGauntletStreak, b.bestGauntletStreak),
    perfectImpossibleUnlocked: Boolean(a.perfectImpossibleUnlocked || b.perfectImpossibleUnlocked),
    prequelCompleted: Boolean(a.prequelCompleted || b.prequelCompleted),
    prequelCodasMask: (a.prequelCodasMask | b.prequelCodasMask) & 7
  };
}
