// Browser localStorage persistence for Settings, Codex (19 Milestones), Leaderboards, and Suspend Save.

import { Ending, Hero, Difficulty, Phase } from "./engine.js";

const KEYS = {
  settings: "dungeon_descent_settings_v1",
  codex: "dungeon_descent_codex_v1",
  leaderboards: "dungeon_descent_leaderboards_v1",
  suspend: "dungeon_descent_suspend_v1"
};

function getStorage(customStorage) {
  if (customStorage) return customStorage;
  if (typeof window !== "undefined" && window.localStorage) return window.localStorage;
  const mem = new Map();
  return {
    getItem: (k) => (mem.has(k) ? mem.get(k) : null),
    setItem: (k, v) => mem.set(k, String(v)),
    removeItem: (k) => mem.delete(k)
  };
}

function clamp(v, lo, hi) {
  return Math.max(lo, Math.min(hi, v));
}

export function endingBit(ending) {
  switch (ending) {
    case Ending.Good: return 1 << 0;
    case Ending.True: return 1 << 1;
    case Ending.Bad: return 1 << 2;
    case Ending.Perfect: return 1 << 3;
    default: return 0;
  }
}

export function defaultSettings() {
  return {
    musicVolume: 25,
    sfxVolume: 80,
    narrationVolume: 60,
    muted: false,
    fullscreen: false
  };
}

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
    bestGauntletStreak: 0
  };
}

export function isEndingUnlocked(codex, ending) {
  const bit = endingBit(ending);
  return bit !== 0 && ((codex.unlockedEndingsMask & bit) !== 0);
}

export function unlockedEndingCount(codex) {
  let count = 0;
  for (let i = 0; i < 4; i++) {
    if ((codex.unlockedEndingsMask & (1 << i)) !== 0) count++;
  }
  return count;
}

export function getCodexMilestones(codex) {
  const allFourHeroes = codex.heroVictories.every((v) => v > 0);
  return [
    { id: "first_victory", title: "Slayer of Malvel", description: "Defeat Malvel and reach any victorious ending.", unlocked: codex.totalVictories > 0 },
    { id: "true_ending", title: "Justice for Falkreath", description: "Unlock the True Ending.", unlocked: isEndingUnlocked(codex, Ending.True) },
    { id: "crown_in_ashes", title: "A Crown in Ashes", description: "Earn the Perfect Ending (9/9 battles, 24/24 riddles, 9/9 rep).", unlocked: isEndingUnlocked(codex, Ending.Perfect) },
    { id: "all_endings", title: "Chronicler of Aethoria", description: "Unlock all four branching endings.", unlocked: unlockedEndingCount(codex) === 4 },
    { id: "all_heroes", title: "Guild Champion", description: "Defeat Malvel with Warrior, Ranger, Mage and Rogue.", unlocked: allFourHeroes },
    { id: "impossible_clear", title: "Defying the Impossible", description: "Defeat Malvel on Impossible difficulty.", unlocked: codex.impossibleVictories > 0 },
    { id: "riddle_savant", title: "Riddle Savant", description: "Reach a riddle streak of 8+ correct answers in a row.", unlocked: codex.bestRiddleStreak >= 8 },
    { id: "gauntlet_master", title: "Gauntlet Scholar", description: "Score 10+ correct answers in a Riddle Gauntlet.", unlocked: codex.bestGauntletScore >= 10 },
    { id: "fallen_to_malvel", title: "Fallen to the Sorcerer", description: "Meet your end at Malvel's hands in the final chamber (Bad Ending).", unlocked: isEndingUnlocked(codex, Ending.Bad) },
    { id: "claimed_by_depths", title: "Claimed by the Depths", description: "Perish in combat or to the Riddle Keeper before reaching Malvel.", unlocked: codex.dungeonDeaths > 0 },
    { id: "evil_route", title: "Ruthless Descent", description: "Show no mercy: choose the kill option in all 3 NPC encounters in one run.", unlocked: Boolean(codex.evilRouteCompleted) },
    { id: "no_ability_easy", title: "Pure Steel: Easy", description: "Complete an entire Easy run from Floor 1 to Malvel without using your class ability once.", unlocked: (codex.noAbilityDifficultyMask & (1 << 0)) !== 0 },
    { id: "no_ability_medium", title: "Pure Steel: Medium", description: "Complete an entire Medium run from Floor 1 to Malvel without using your class ability once.", unlocked: (codex.noAbilityDifficultyMask & (1 << 1)) !== 0 },
    { id: "no_ability_hard", title: "Pure Steel: Hard", description: "Complete an entire Hard run from Floor 1 to Malvel without using your class ability once.", unlocked: (codex.noAbilityDifficultyMask & (1 << 2)) !== 0 },
    { id: "no_ability_impossible", title: "Pure Steel: Impossible", description: "Complete an entire Impossible run from Floor 1 to Malvel without using your class ability once.", unlocked: (codex.noAbilityDifficultyMask & (1 << 3)) !== 0 },
    { id: "malvel_no_ability_easy", title: "Duelist vs Malvel: Easy", description: "Defeat Malvel on Easy without using your class ability in the final battle.", unlocked: (codex.malvelNoAbilityDifficultyMask & (1 << 0)) !== 0 },
    { id: "malvel_no_ability_medium", title: "Duelist vs Malvel: Medium", description: "Defeat Malvel on Medium without using your class ability in the final battle.", unlocked: (codex.malvelNoAbilityDifficultyMask & (1 << 1)) !== 0 },
    { id: "malvel_no_ability_hard", title: "Duelist vs Malvel: Hard", description: "Defeat Malvel on Hard without using your class ability in the final battle.", unlocked: (codex.malvelNoAbilityDifficultyMask & (1 << 2)) !== 0 },
    { id: "malvel_no_ability_impossible", title: "Duelist vs Malvel: Impossible", description: "Defeat Malvel on Impossible without using your class ability in the final battle.", unlocked: (codex.malvelNoAbilityDifficultyMask & (1 << 3)) !== 0 }
  ];
}

export function mergeCodex(aRaw, bRaw) {
  const def = defaultCodex();
  const norm = (r) => ({
    ...def,
    ...(r && typeof r === "object" ? r : {}),
    heroVictories: Array.isArray(r?.heroVictories) && r.heroVictories.length === 4 ? r.heroVictories.map((n) => Math.max(0, Math.trunc(Number(n) || 0))) : [0, 0, 0, 0],
    heroBestScore: Array.isArray(r?.heroBestScore) && r.heroBestScore.length === 4 ? r.heroBestScore.map((n) => Math.max(0, Math.trunc(Number(n) || 0))) : [0, 0, 0, 0]
  });
  const a = norm(aRaw);
  const b = norm(bRaw);
  const fastest = (a.fastestVictorySeconds > 0 && b.fastestVictorySeconds > 0)
    ? Math.min(a.fastestVictorySeconds, b.fastestVictorySeconds)
    : Math.max(a.fastestVictorySeconds, b.fastestVictorySeconds);
  const totalRiddlesAnswered = Math.max(a.totalRiddlesAnswered, b.totalRiddlesAnswered);
  const totalRiddlesCorrect = Math.min(totalRiddlesAnswered, Math.max(a.totalRiddlesCorrect, b.totalRiddlesCorrect));
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
    bestGauntletStreak: Math.max(a.bestGauntletStreak, b.bestGauntletStreak)
  };
}

export class WebStore {
  constructor(customStorage = null) {
    this.storage = getStorage(customStorage);
  }

  // Settings
  loadSettings() {
    const def = defaultSettings();
    try {
      const raw = this.storage.getItem(KEYS.settings);
      if (!raw) return def;
      const parsed = JSON.parse(raw);
      return {
        musicVolume: clamp(Number(parsed.musicVolume ?? def.musicVolume), 0, 100),
        sfxVolume: clamp(Number(parsed.sfxVolume ?? def.sfxVolume), 0, 100),
        narrationVolume: clamp(Number(parsed.narrationVolume ?? def.narrationVolume), 0, 100),
        muted: Boolean(parsed.muted),
        fullscreen: Boolean(parsed.fullscreen)
      };
    } catch {
      return def;
    }
  }

  saveSettings(settings) {
    const clean = {
      musicVolume: clamp(Math.round(Number(settings.musicVolume) || 0), 0, 100),
      sfxVolume: clamp(Math.round(Number(settings.sfxVolume) || 0), 0, 100),
      narrationVolume: clamp(Math.round(Number(settings.narrationVolume) || 0), 0, 100),
      muted: Boolean(settings.muted),
      fullscreen: Boolean(settings.fullscreen)
    };
    this.storage.setItem(KEYS.settings, JSON.stringify(clean));
    return clean;
  }

  // Codex
  loadCodex() {
    const def = defaultCodex();
    try {
      const raw = this.storage.getItem(KEYS.codex);
      if (!raw) return def;
      const parsed = JSON.parse(raw);
      return {
        ...def,
        ...parsed,
        heroVictories: Array.isArray(parsed.heroVictories) && parsed.heroVictories.length === 4 ? parsed.heroVictories.map(Number) : [0, 0, 0, 0],
        heroBestScore: Array.isArray(parsed.heroBestScore) && parsed.heroBestScore.length === 4 ? parsed.heroBestScore.map(Number) : [0, 0, 0, 0]
      };
    } catch {
      return def;
    }
  }

  saveCodex(codex) {
    this.storage.setItem(KEYS.codex, JSON.stringify(codex));
  }

  recordCompletedRun(run, playTimeSeconds) {
    const data = this.loadCodex();
    data.totalRuns++;
    const p = run.player;
    const heroIdx = p.hero;
    data.totalRiddlesAnswered += Math.max(0, p.riddlesAnswered);
    data.totalRiddlesCorrect += Math.max(0, p.riddlesCorrect);
    data.bestRiddleStreak = Math.max(data.bestRiddleStreak, p.bestRiddleStreak);
    if (p.npcKills >= 3) data.evilRouteCompleted = true;
    if (heroIdx >= 0 && heroIdx < 4) {
      data.heroBestScore[heroIdx] = Math.max(data.heroBestScore[heroIdx], run.score());
    }
    if (run.phase === Phase.Ending && run.ending !== Ending.None) {
      data.unlockedEndingsMask |= endingBit(run.ending);
      if (run.ending !== Ending.Bad) {
        data.totalVictories++;
        if (heroIdx >= 0 && heroIdx < 4) data.heroVictories[heroIdx]++;
        if (run.difficulty === Difficulty.Impossible) data.impossibleVictories++;
        const diffBit = run.difficulty;
        if (diffBit >= 0 && diffBit < 4) {
          if (p.classAbilitiesUsed === 0) data.noAbilityDifficultyMask |= (1 << diffBit);
          if (!run.classAbilityUsedInBattle()) data.malvelNoAbilityDifficultyMask |= (1 << diffBit);
        }
        if (playTimeSeconds > 0 && (data.fastestVictorySeconds === 0 || playTimeSeconds < data.fastestVictorySeconds)) {
          data.fastestVictorySeconds = Math.round(playTimeSeconds);
        }
      } else {
        data.totalDefeats++;
      }
    } else if (run.phase === Phase.Defeat) {
      data.totalDefeats++;
      data.dungeonDeaths++;
    }
    this.saveCodex(data);
    return data;
  }

  recordGauntletResult(score, bestStreak) {
    const data = this.loadCodex();
    data.bestGauntletScore = Math.max(data.bestGauntletScore, Math.max(0, score));
    data.bestGauntletStreak = Math.max(data.bestGauntletStreak, Math.max(0, bestStreak));
    data.bestRiddleStreak = Math.max(data.bestRiddleStreak, Math.max(0, bestStreak));
    this.saveCodex(data);
    return data;
  }

  // Leaderboards
  loadLeaderboards() {
    const empty = [[], [], [], []];
    try {
      const raw = this.storage.getItem(KEYS.leaderboards);
      if (!raw) return empty;
      const parsed = JSON.parse(raw);
      if (!Array.isArray(parsed) || parsed.length !== 4) return empty;
      return parsed.map((board) => (Array.isArray(board) ? board : []));
    } catch {
      return empty;
    }
  }

  submitLeaderboard(difficulty, playerLabel, score, hero, ending, seed, durationSeconds) {
    const cleanName = String(playerLabel || "").trim().replace(/[^A-Za-z0-9_-]/g, "").slice(0, 20);
    if (!cleanName) throw new Error("Player name must be 1 to 20 letters, digits, _ or -.");
    const tables = this.loadLeaderboards();
    const id = `${Date.now()}-${Math.random().toString(36).slice(2, 8)}`;
    const entry = {
      id,
      username: cleanName,
      initials: cleanName,
      score: Math.round(score),
      hero,
      ending,
      seed: seed >>> 0,
      durationSeconds: Math.max(0, Math.round(durationSeconds)),
      timestamp: Date.now()
    };
    const board = tables[difficulty];
    board.push(entry);
    board.sort((a, b) => (b.score - a.score) || (a.durationSeconds - b.durationSeconds) || (a.timestamp - b.timestamp));
    this.storage.setItem(KEYS.leaderboards, JSON.stringify(tables));
    return { tables, entryId: id };
  }

  // Suspend Save
  saveSuspend(run, playTimeSeconds) {
    if (!run.inProgress() || run.phase === Phase.Riddle) {
      throw new Error("Cannot suspend save outside a safe room state.");
    }
    const payload = {
      version: 1,
      hero: run.player.hero,
      difficulty: run.difficulty,
      biome: run.biome,
      floorNumber: run.floorNumber,
      roomNumber: run.roomNumber(),
      playTimeSeconds: Math.max(0, Math.round(playTimeSeconds)),
      state: run.serializeState()
    };
    this.storage.setItem(KEYS.suspend, JSON.stringify(payload));
  }

  loadSuspendPreview() {
    try {
      const raw = this.storage.getItem(KEYS.suspend);
      if (!raw) return null;
      const parsed = JSON.parse(raw);
      if (!parsed || !parsed.state) return null;
      return parsed;
    } catch {
      return null;
    }
  }

  clearSuspend() {
    this.storage.removeItem(KEYS.suspend);
  }
}
