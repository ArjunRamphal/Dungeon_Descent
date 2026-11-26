// Automated Node.js test suite for Dungeon Descent Web Edition engine, assets, RNG, and storage.
import assert from "node:assert/strict";
import fs from "node:fs/promises";
import path from "node:path";
import { fileURLToPath } from "node:url";

import { AssetStore } from "../web/js/assets.js";
import {
  DungeonRun, Hero, Difficulty, Biome, Room, Phase, Ending, Stat, CombatAction,
  endingMusicFile
} from "../web/js/engine.js";
import { parseSeed, dailyRunSeed } from "../web/js/rng.js";
import { WebStore, getCodexMilestones, isEndingUnlocked, isPrequelUnlocked } from "../web/js/storage.js";
import {
  PrequelRun, PrequelAct, PrequelPhase, PrequelCoda,
  PREQUEL_ART_FILES, PREQUEL_MUSIC_FILES
} from "../web/js/prequel.js";

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const repoRoot = path.resolve(__dirname, "..");
const assetsDir = path.join(repoRoot, "assets");

async function main() {
  const assets = new AssetStore("./assets", async (rel) => {
    return fs.readFile(path.join(assetsDir, rel), "utf8");
  });
  await assets.preloadAll();

  // 1. Verify all 220 dark-fantasy riddles loaded cleanly with stripped route tags
  assert.equal(assets.sourceRiddles[0].length, 70, "70 Easy riddles");
  assert.equal(assets.sourceRiddles[1].length, 75, "75 Medium riddles");
  assert.equal(assets.sourceRiddles[2].length, 75, "75 Hard riddles");
  for (const bank of assets.sourceRiddles) {
    for (const r of bank) {
      assert.ok(!r.question.startsWith("["), `Route tag must be stripped from displayed question: ${r.question}`);
    }
  }

  // 2. Seed parsing & Daily Run determinism
  assert.equal(parseSeed("42"), 42);
  assert.equal(parseSeed("  007 "), 7);
  assert.equal(parseSeed("Aethoria"), parseSeed("Aethoria"));
  assert.equal(dailyRunSeed(2026, 11, 22), dailyRunSeed(2026, 11, 22));

  // 3. Seeded Library & Shop shuffling + exact label-to-reward mapping + Suspend Save preservation
  const run = new DungeonRun(assets);
  const libOrders = new Set();
  const shopOrders = new Set();

  const playToRoom = (r, targetRoom) => {
    for (let step = 0; step < 200 && r.inProgress() && r.room() !== targetRoom; step++) {
      if (r.phase === Phase.Room) {
        let c = 0;
        if (r.room() === Room.Respite) c = 1;
        else if (r.room() === Room.Shop) c = r.floorNumber === 3 ? 2 : 1;
        r.choose(r.displayChoiceFor(c));
      } else if (r.phase === Phase.Combat) r.attack();
      else if (r.phase === Phase.Riddle) r.answer(r.riddle().correct);
      else r.advance();
    }
  };

  for (let seed = 1; seed <= 30; seed++) {
    run.start(Hero.Warrior, Biome.Ice, seed, Difficulty.Easy);
    playToRoom(run, Room.Library);
    const libChoices = run.choices();
    libOrders.add(libChoices.join("|"));

    // Verify Suspend Save preserves shuffled Library order
    const restored = new DungeonRun(assets);
    restored.deserializeState(run.serializeState());
    assert.deepEqual(restored.choices(), libChoices, "Suspend save preserves shuffled Library choices");

    // Verify every book grants its exact matching stat
    for (let i = 0; i < libChoices.length; i++) {
      const probe = new DungeonRun(assets);
      probe.deserializeState(run.serializeState());
      const before = [...probe.player.stats];
      probe.choose(i);
      const lbl = libChoices[i];
      if (lbl === "The Art of Warfare") assert.ok(probe.player.stats[Stat.Strength] > before[Stat.Strength]);
      else if (lbl === "The Whispering Winds") assert.ok(probe.player.stats[Stat.Agility] > before[Stat.Agility]);
      else if (lbl === "The Ancient Lore") assert.ok(probe.player.stats[Stat.Wisdom] > before[Stat.Wisdom]);
      else if (lbl === "The Heart of the Forest") assert.ok(probe.player.stats[Stat.Observation] > before[Stat.Observation]);
      else assert.fail(`Unexpected book: ${lbl}`);
    }

    playToRoom(run, Room.Shop);
    const shopChoices = run.choices();
    shopOrders.add(shopChoices.join("|"));
    for (let i = 0; i < shopChoices.length; i++) {
      const probe = new DungeonRun(assets);
      probe.deserializeState(run.serializeState());
      const before = { ...probe.player, stats: [...probe.player.stats] };
      probe.choose(i);
      const lbl = shopChoices[i];
      if (lbl === "Health Potion") assert.ok(probe.player.stats[Stat.Health] > before.stats[Stat.Health]);
      else if (lbl === "Stat Increase Potion") assert.ok(probe.player.stats[Stat.Wisdom] > before.stats[Stat.Wisdom]);
      else if (lbl === "Book of Knowledge") assert.equal(probe.player.reputation, before.reputation + 1);
      else assert.fail(`Unexpected shop item: ${lbl}`);
    }
  }
  assert.ok(libOrders.size >= 8, "Library book order shuffles broadly");
  assert.equal(shopOrders.size, 6, "Shop item order covers all 6 permutations");

  // 4. Full 3-floor runs reaching all 4 endings + Codex 19 milestones + Leaderboard + Ending Music
  const store = new WebStore();
  const simulate = (hero, diff, { evil = false, perfect = false, loseMalvel = false, useAbilityEarly = false, useAbilityMalvel = false, seed = 12 } = {}) => {
    const r = new DungeonRun(assets);
    r.start(hero, Biome.Ice, seed, diff);
    for (let step = 0; step < 1500 && r.inProgress(); step++) {
      if (r.phase === Phase.Complete) {
        r.descend(r.floorNumber === 1 ? Biome.Ghost : Biome.Lava);
      } else if (r.phase === Phase.Room) {
        let c = 0;
        if (r.room() === Room.Library) {
          if (perfect && r.floorNumber === 3) c = 0;
          else if (loseMalvel && r.floorNumber === 3) c = 3;
          else c = hero === Hero.Mage ? 2 : hero === Hero.Rogue ? 1 : 0;
        } else if (r.room() === Room.Battle && loseMalvel) {
          c = 1;
        } else if (r.room() === Room.Encounter) {
          c = evil ? 1 : 0;
        } else if (r.room() === Room.Respite) {
          c = perfect && r.floorNumber === 1 ? 0 : 1;
        } else if (r.room() === Room.Shop) {
          c = perfect ? (r.floorNumber === 2 ? 1 : 2) : (r.floorNumber === 3 ? 2 : 1);
        }
        r.choose(r.displayChoiceFor(c));
      } else if (r.phase === Phase.Combat) {
        const isMalvel = r.floorNumber === 3 && r.room() === Room.Boss;
        const allowAbility = isMalvel ? useAbilityMalvel : useAbilityEarly;
        if (allowAbility && r.classAbilityAvailable()) r.attack(CombatAction.ClassAbility);
        else if (!loseMalvel && r.bossIntent().telegraphed && r.bossIntent().counterMultiplier > 1) r.attack(CombatAction.GuardStrike);
        else r.attack(CombatAction.StandardStrike);
      } else if (r.phase === Phase.Riddle) {
        if (loseMalvel && r.floorNumber === 3) {
          for (let i = 0; i < 3; i++) {
            if (i !== r.riddle().correct && r.answerAvailable(i)) { r.answer(i); break; }
          }
        } else {
          r.answer(r.riddle().correct);
        }
      } else {
        r.advance();
      }
    }
    return r;
  };

  // Perfect ending
  let perfectRun = null;
  for (let s = 1; s < 50; s++) {
    const candidate = simulate(Hero.Mage, Difficulty.Easy, { perfect: true, useAbilityEarly: true, useAbilityMalvel: true, seed: s });
    if (candidate.phase === Phase.Ending && candidate.ending === Ending.Perfect) {
      perfectRun = candidate;
      break;
    }
  }
  assert.ok(perfectRun, "Earned Perfect Ending in web engine");
  assert.equal(perfectRun.musicFile(), "ending_perfect.ogg");
  store.recordCompletedRun(perfectRun, 195);

  // Evil + full-run 0-ability Easy (Good ending because rep < 5)
  const evilGoodRun = simulate(Hero.Mage, Difficulty.Easy, { evil: true, useAbilityEarly: false, useAbilityMalvel: false, seed: 15 });
  assert.equal(evilGoodRun.phase, Phase.Ending);
  assert.equal(evilGoodRun.ending, Ending.Good);
  assert.equal(evilGoodRun.musicFile(), "ending_good.ogg");
  assert.equal(evilGoodRun.player.classAbilitiesUsed, 0);
  assert.equal(evilGoodRun.player.npcKills, 3);
  store.recordCompletedRun(evilGoodRun, 210);

  // Medium Duelist vs Malvel (used ability earlier, NOT vs Malvel) -> True Ending
  const duelistRun = simulate(Hero.Mage, Difficulty.Medium, { evil: false, useAbilityEarly: true, useAbilityMalvel: false, seed: 12 });
  assert.equal(duelistRun.phase, Phase.Ending);
  assert.equal(duelistRun.ending, Ending.True);
  assert.equal(duelistRun.musicFile(), "ending_true.ogg");
  assert.ok(duelistRun.player.classAbilitiesUsed > 0 && !duelistRun.classAbilityUsedInBattle());
  store.recordCompletedRun(duelistRun, 220);

  // Bad ending (fall to Malvel)
  const badRun = simulate(Hero.Mage, Difficulty.Easy, { loseMalvel: true, seed: 9 });
  assert.equal(badRun.phase, Phase.Ending);
  assert.equal(badRun.ending, Ending.Bad);
  assert.equal(badRun.musicFile(), "ending_bad.ogg");
  store.recordCompletedRun(badRun, 160);

  // Verify Codex milestones (22 total including 3 Ashes of Falkreath prequel milestones)
  const codex = store.loadCodex();
  const ms = getCodexMilestones(codex);
  assert.equal(ms.length, 22, "Exactly 22 Codex milestones");
  const isUnlocked = (id) => ms.find((m) => m.id === id)?.unlocked === true;
  assert.ok(isUnlocked("first_victory"));
  assert.ok(isUnlocked("true_ending"));
  assert.ok(isUnlocked("crown_in_ashes"));
  assert.ok(isUnlocked("all_endings"));
  assert.ok(isUnlocked("fallen_to_malvel"));
  assert.ok(isUnlocked("evil_route"));
  assert.ok(isUnlocked("no_ability_easy"));
  assert.ok(isUnlocked("malvel_no_ability_easy"));
  assert.ok(isUnlocked("malvel_no_ability_medium"));
  assert.ok(!isUnlocked("no_ability_medium"), "Using ability on earlier floor must not unlock full-run Pure Steel: Medium");
  assert.ok(!isUnlocked("seal_breaker"), "Easy Perfect ending must not unlock Seal Breaker");
  assert.ok(!isPrequelUnlocked(codex), "Prequel stays sealed until Perfect Ending on Impossible");
  assert.ok(isEndingUnlocked(codex, Ending.Good) && isEndingUnlocked(codex, Ending.True) &&
            isEndingUnlocked(codex, Ending.Bad) && isEndingUnlocked(codex, Ending.Perfect));

  // Verify multi-factor score breakdown (stats, ending health, leftover strikes, run time)
  const perfectBdFast = perfectRun.scoreBreakdown(195);
  const perfectBdSlow = perfectRun.scoreBreakdown(650);
  const badBd = badRun.scoreBreakdown(160);
  assert.ok(perfectBdFast.statsPoints > 0, "Stats contribute to final score");
  assert.ok(perfectBdFast.healthPoints > 0, "Remaining health contributes to final score");
  assert.ok(perfectRun.player.strikesLeftover > 0, "Leftover strikes are tracked across battles");
  assert.equal(perfectBdFast.strikesPoints, perfectRun.player.strikesLeftover * 10);
  assert.equal(perfectBdFast.timePoints, (1800 - 195) * 2);
  assert.equal(
    perfectBdFast.total,
    perfectBdFast.statsPoints + perfectBdFast.healthPoints + perfectBdFast.strikesPoints + perfectBdFast.timePoints
  );
  assert.ok(perfectBdFast.total > perfectBdSlow.total, "Faster victory scores higher than slower victory");
  assert.equal(perfectRun.scoreBreakdown(2000).timePoints, 0, "Time bonus floors at 0 beyond 30 minutes");
  assert.equal(badBd.timePoints, 0, "Bad ending receives 0 time bonus (no die-fast exploit)");
  assert.ok(perfectBdFast.total <= 50000, "Final score stays safely within server cap");
  // Verify strikesLeftover survives Suspend Save round-trip
  const clonedRun = new DungeonRun(assets);
  clonedRun.deserializeState(perfectRun.serializeState());
  assert.equal(clonedRun.player.strikesLeftover, perfectRun.player.strikesLeftover);
  assert.equal(clonedRun.score(195), perfectRun.score(195));

  // Verify Leaderboard submission & sorting
  const lb = store.submitLeaderboard(Difficulty.Easy, "ACE", perfectRun.score(195), Hero.Mage, Ending.Perfect, perfectRun.seed, 195);
  assert.equal(lb.tables[Difficulty.Easy].length, 1);
  assert.equal(lb.tables[Difficulty.Easy][0].initials, "ACE");
  assert.equal(lb.tables[Difficulty.Easy][0].score, perfectBdFast.total);

  // Verify all 11 candle intro animation frames exist on disk
  for (let i = 1; i <= 11; i++) {
    const file = path.join(assetsDir, "candle", `candle${i}.jpg`);
    const stat = await fs.stat(file);
    assert.ok(stat.size > 10000, `Candle intro frame exists: ${file}`);
  }
  // Verify all 91 animated main menu background frames exist on disk
  for (let i = 1; i <= 91; i++) {
    const file = path.join(assetsDir, "images", "startscreen", `startscreen${i}.jpg`);
    const stat = await fs.stat(file);
    assert.ok(stat.size > 10000, `Startscreen animation frame exists: ${file}`);
  }

  // Verify all 4 ending music files exist on disk
  for (const e of [Ending.Good, Ending.True, Ending.Bad, Ending.Perfect]) {
    const file = path.join(assetsDir, "music", endingMusicFile(e));
    const stat = await fs.stat(file);
    assert.ok(stat.size > 100000, `Ending audio exists: ${file}`);
  }

  // 5. Ashes of Falkreath (Web Prequel Campaign): assets, audio, both Codas, Parley timeout & Codex
  for (const rel of PREQUEL_ART_FILES) {
    const stat = await fs.stat(path.join(assetsDir, rel));
    assert.ok(stat.size > 5000, `Prequel artwork exists: ${rel}`);
  }
  for (const name of PREQUEL_MUSIC_FILES) {
    const stat = await fs.stat(path.join(assetsDir, "music", name));
    assert.ok(stat.size > 100000, `Prequel audio exists: ${name}`);
  }

  const playPrequel = async (pq, { ruthless, useAbilities }) => {
    pq.start();
    let safety = 0;
    while (pq.phase !== PrequelPhase.Epilogue && pq.phase !== PrequelPhase.Defeat && ++safety < 600) {
      await fs.stat(path.join(assetsDir, "music", pq.musicFile()));
      if (pq.phase === PrequelPhase.ActIntro || pq.phase === PrequelPhase.Outcome || pq.phase === PrequelPhase.ActComplete) {
        pq.advance();
      } else if (pq.phase === PrequelPhase.Scene) {
        assert.ok(pq.choices.length >= 1, "Scene must have choices");
        pq.choose(ruthless ? pq.choices.length - 1 : 0);
      } else if (pq.phase === PrequelPhase.Parley) {
        pq.answerParley(pq.parley.bestIndex);
      } else if (pq.phase === PrequelPhase.Combat) {
        if (useAbilities && pq.combatChoiceAvailable(CombatAction.ClassAbility)) {
          pq.attack(CombatAction.ClassAbility);
        } else if (pq.enemy.intent.telegraphed && pq.enemy.intent.counterMultiplier > 1.2) {
          pq.attack(CombatAction.GuardStrike);
        } else if (pq.combatChoiceAvailable(CombatAction.HeavyStrike) && !pq.enemy.intent.telegraphed) {
          pq.attack(CombatAction.HeavyStrike);
        } else {
          pq.attack(CombatAction.StandardStrike);
        }
      }
    }
    assert.equal(pq.phase, PrequelPhase.Epilogue, `Expected Epilogue, ended in ${pq.phase} at chapter ${pq.chapterNumber()}`);
  };

  // Remorseful + Iron Chronicle (0 abilities) -> Last Doubt Coda
  const ironPrequel = new PrequelRun();
  await playPrequel(ironPrequel, { ruthless: false, useAbilities: false });
  assert.equal(ironPrequel.coda, PrequelCoda.LastDoubt);
  assert.equal(ironPrequel.musicFile(), "prequel_coda_last_doubt.ogg");
  assert.ok(ironPrequel.ironChronicleEligible() && ironPrequel.abilitiesUsed === 0);
  assert.equal(ironPrequel.battlesWon, 6);
  assert.equal(ironPrequel.parleysWon, 3);
  assert.equal(ironPrequel.sceneImage, "images/prequel/pedestal_chamber.jpg");

  // Ruthless + Signature Abilities -> Hollow Crown Coda
  const ruthlessPrequel = new PrequelRun();
  await playPrequel(ruthlessPrequel, { ruthless: true, useAbilities: true });
  assert.equal(ruthlessPrequel.coda, PrequelCoda.HollowCrown);
  assert.equal(ruthlessPrequel.musicFile(), "prequel_coda_hollow_crown.ogg");
  assert.ok(!ruthlessPrequel.ironChronicleEligible() && ruthlessPrequel.abilitiesUsed >= 3);

  // Parley timeout + restartAct() rollback
  const parleyProbe = new PrequelRun();
  parleyProbe.start();
  parleyProbe.advance(); // ActIntro -> Chapter 1 Scene
  parleyProbe.choose(0); // Outcome -> queued Parley
  parleyProbe.advance(); // Parley
  assert.equal(parleyProbe.phase, PrequelPhase.Parley);
  const hpBefore = parleyProbe.character.hp;
  parleyProbe.tickParley(120);
  assert.equal(parleyProbe.phase, PrequelPhase.Outcome);
  assert.ok(parleyProbe.character.hp < hpBefore);
  parleyProbe.restartAct();
  assert.equal(parleyProbe.phase, PrequelPhase.ActIntro);
  assert.equal(parleyProbe.act, PrequelAct.Act1_Eldred);
  assert.equal(parleyProbe.character.hp, parleyProbe.character.maxHp);

  // Record Prequel completion in WebStore Codex
  const afterPrequel = store.recordPrequelCompletion(1, true);
  assert.equal(afterPrequel.prequelCompleted, true);
  assert.equal(afterPrequel.prequelCodasMask, (1 << 1) | (1 << 2));
  const msAfter = getCodexMilestones(afterPrequel);
  assert.ok(msAfter.find((m) => m.id === "ashes_of_falkreath")?.unlocked);
  assert.ok(msAfter.find((m) => m.id === "iron_chronicle")?.unlocked);

  // 6. Route-aware riddle filtering (Ice->Ghost vs Jungle->Desert) & Gauntlet save/load/clear
  const checkRouteRiddles = (firstBio, secondBio) => {
    const r = new DungeonRun(assets);
    r.start(Hero.Mage, firstBio, 17, Difficulty.Easy);
    for (let step = 0; step < 300 && r.floorNumber === 1 && r.inProgress(); step++) {
      if (r.phase === Phase.Complete) { r.descend(secondBio); break; }
      else if (r.phase === Phase.Room) r.choose(r.displayChoiceFor(r.room() === Room.Respite ? 1 : 0));
      else if (r.phase === Phase.Combat) r.attack();
      else if (r.phase === Phase.Riddle) r.answer(r.riddle().correct);
      else r.advance();
    }
    assert.equal(r.floorNumber, 2);
    for (const idx of r.riddleOrder) {
      const req = r.riddles[idx].requiredBiome;
      assert.ok(req === null || req === firstBio || req === secondBio,
        `Floor 2 riddle must match chosen route (${firstBio}/${secondBio}), got ${req}`);
    }
    for (let step = 0; step < 400 && r.floorNumber === 2 && r.inProgress(); step++) {
      if (r.phase === Phase.Complete) { r.descend(Biome.Lava); break; }
      else if (r.phase === Phase.Room) r.choose(r.displayChoiceFor(r.room() === Room.Respite ? 1 : 0));
      else if (r.phase === Phase.Combat) r.attack();
      else if (r.phase === Phase.Riddle) r.answer(r.riddle().correct);
      else r.advance();
    }
    assert.equal(r.floorNumber, 3);
    for (const idx of r.riddleOrder) {
      const req = r.riddles[idx].requiredBiome;
      assert.ok(req === null || req === firstBio || req === secondBio || req === Biome.Lava,
        `Floor 3 riddle must match chosen route (${firstBio}/${secondBio}/Lava), got ${req}`);
    }
  };
  checkRouteRiddles(Biome.Ice, Biome.Ghost);
  checkRouteRiddles(Biome.Jungle, Biome.Desert);

  // Gauntlet Save / Load / Clear
  assert.equal(store.loadGauntlet(), null);
  store.saveGauntlet({
    active: true,
    finished: false,
    showingFeedback: false,
    feedbackText: "",
    feedbackOk: false,
    lives: 2,
    score: 14,
    streak: 5,
    bestStreak: 8,
    index: 16,
    timeLimit: 16,
    timeLeft: 11.4,
    deck: Array.from({ length: 220 }, (_, i) => ({
      tier: i < 70 ? 0 : (i < 145 ? 1 : 2),
      riddle: { question: `Q${i}`, answers: ["A", "B", "C"], correct: 0 }
    }))
  });
  const loadedG = store.loadGauntlet();
  assert.ok(loadedG && loadedG.score === 14 && loadedG.lives === 2 && loadedG.index === 16 && loadedG.deck.length === 220);
  store.clearGauntlet();
  assert.equal(store.loadGauntlet(), null);

  // 7. All 10 Unique Ordinary Monster Battle Traits, Canonical Names & Mid-Combat Save
  const reachBattle = (firstBio, secondBio, targetFloor, targetIsSecondBattle, difficulty = Difficulty.Hard, hero = Hero.Warrior, seed = 42) => {
    const r = new DungeonRun(assets);
    r.start(hero, firstBio, seed, difficulty);
    for (let step = 0; step < 900 && r.inProgress(); step++) {
      if (r.floorNumber === targetFloor && r.phase === Phase.Room && r.room() === Room.Battle) {
        const isSecond = (targetFloor === 2) ? (r.roomNumber() === 8) : (r.roomNumber() === 5);
        if (isSecond === targetIsSecondBattle) return r;
      }
      if (r.phase === Phase.Complete) {
        r.descend(r.floorNumber === 1 ? secondBio : Biome.Lava);
      } else if (r.phase === Phase.Room) {
        if (r.room() === Room.Battle) r.choose(1); // sneak past earlier battles quickly
        else if (r.room() === Room.Respite) r.choose(1);
        else if (r.room() === Room.Shop) r.choose(r.displayChoiceFor(r.floorNumber === 3 ? 2 : 1));
        else if (r.room() === Room.Library) r.choose(r.displayChoiceFor(hero === Hero.Mage ? 2 : 0));
        else r.choose(0);
      } else if (r.phase === Phase.Combat) {
        const intent = r.bossIntent();
        if (r.classAbilityAvailable()) r.attack(CombatAction.ClassAbility);
        else if (intent.telegraphed && intent.counterMultiplier > 1) r.attack(CombatAction.GuardStrike);
        else if (intent.heavyPierces && intent.damageTakenMultiplier < 1 && r.strikesLeft >= 2) r.attack(CombatAction.HeavyStrike);
        else r.attack(CombatAction.StandardStrike);
      } else if (r.phase === Phase.Riddle) {
        r.answer(r.riddle().correct);
      } else {
        r.advance();
      }
    }
    throw new Error(`Failed to reach battle floor=${targetFloor} second=${targetIsSecondBattle}`);
  };

  // 1) F1 Ice Battle 1: Glacier Troll — Runic Frost Club
  {
    const r = reachBattle(Biome.Ice, Biome.Desert, 1, false, Difficulty.Impossible);
    assert.equal(r.enemyName(), "Glacier Troll");
    assert.equal(r.enemyTraitName(), "Runic Frost Club");
    r.choose(0);
    assert.equal(r.bossIntent().isBoss, false);
    assert.equal(r.bossIntent().counterMultiplier, 1);
    r.attack(CombatAction.GuardStrike);
    assert.equal(r.bossTurn, 2);
    assert.equal(r.bossIntent().telegraphed, true);
    assert.equal(r.bossIntent().counterMultiplier, 1.5);
  }

  // 2) F1 Ice Battle 2: Ice Golem — Glacial Shell
  {
    const r = reachBattle(Biome.Ice, Biome.Desert, 1, true, Difficulty.Impossible);
    assert.equal(r.enemyName(), "Ice Golem");
    assert.equal(r.enemyTraitName(), "Glacial Shell");
    r.choose(0);
    assert.equal(r.bossIntent().damageTakenMultiplier, 0.70);
    assert.equal(r.bossIntent().shattersOnPierce, true);
    r.attack(CombatAction.HeavyStrike);
    assert.equal(r.enemyShellBroken, true);
    assert.equal(r.lastCombatExchange.shellShattered, true);
    assert.equal(r.bossIntent().damageTakenMultiplier, 1);
    // Verify Suspend Save preserves shattered shell
    const clone = new DungeonRun(assets);
    clone.deserializeState(r.serializeState());
    assert.equal(clone.enemyShellBroken, true);
    assert.equal(clone.bossTurn, r.bossTurn);
  }

  // 3) F1 Jungle Battle 1: Jungle Troll — Moss Regeneration
  {
    const r = reachBattle(Biome.Jungle, Biome.Desert, 1, false, Difficulty.Impossible);
    assert.equal(r.enemyName(), "Jungle Troll");
    assert.equal(r.enemyTraitName(), "Moss Regeneration");
    r.choose(0);
    assert.equal(r.bossIntent().regenRatio, 0.10);
    r.attack(CombatAction.GuardStrike);
    assert.ok(r.lastCombatExchange.enemyHealed > 0, "Jungle Troll regenerates HP on surviving exchange");
  }

  // 4) F1 Jungle Battle 2: Jungle Golem — Tangled Vines
  {
    const r = reachBattle(Biome.Jungle, Biome.Desert, 1, true, Difficulty.Impossible);
    assert.equal(r.enemyName(), "Jungle Golem");
    assert.equal(r.enemyTraitName(), "Tangled Vines");
    r.choose(0);
    assert.equal(r.bossIntent().drainsStrike, false);
    r.attack(CombatAction.GuardStrike); // turn 1 -> turn 2
    assert.equal(r.bossIntent().drainsStrike, true);
    const strikesBefore = r.strikesLeft;
    r.attack(CombatAction.StandardStrike);
    assert.equal(r.lastCombatExchange.strikesDrained, 1);
    assert.equal(r.strikesLeft, strikesBefore - 2);
  }

  // 5) F2 Desert Battle 1: The Sandwyrm — Sandstone Ambush & Armored Scales
  {
    const r = reachBattle(Biome.Ice, Biome.Desert, 2, false, Difficulty.Hard);
    assert.equal(r.enemyName(), "The Sandwyrm");
    assert.equal(r.enemyTraitName(), "Sandstone Ambush");
    r.choose(0);
    assert.equal(r.bossIntent().counterMultiplier, 1.35);
    assert.equal(r.bossIntent().damageTakenMultiplier, 0.75);
    r.attack(CombatAction.GuardStrike);
    assert.equal(r.bossIntent().counterMultiplier, 1);
    assert.equal(r.bossIntent().damageTakenMultiplier, 0.75);
  }

  // 6) F2 Desert Battle 2: Giant Scorpion — Venomous Stinger + Suspend Save
  {
    const r = reachBattle(Biome.Ice, Biome.Desert, 2, true, Difficulty.Hard);
    assert.equal(r.enemyName(), "Giant Scorpion");
    assert.equal(r.enemyTraitName(), "Venomous Stinger");
    r.choose(0);
    r.enemyHealth = 500;
    r.strikesLeft = 8;
    const hpBefore = r.player.stats[Stat.Health];
    const previewStd = r.previewCombatAction(CombatAction.StandardStrike);
    r.attack(CombatAction.StandardStrike);
    assert.equal(r.playerVenomStacks, 1);
    assert.ok(r.lastCombatExchange.venomDamage > 0);
    assert.ok(Math.abs((hpBefore - r.player.stats[Stat.Health]) - previewStd.expectedCounter) < 0.001,
      "Giant Scorpion preview matches counter + venom tick");
    // Guard blocks additional venom stack, existing stack still ticks
    const hpBeforeGuard = r.player.stats[Stat.Health];
    const previewGuard = r.previewCombatAction(CombatAction.GuardStrike);
    r.attack(CombatAction.GuardStrike);
    assert.equal(r.playerVenomStacks, 1, "Guard prevents new Venom stacks");
    assert.ok(Math.abs((hpBeforeGuard - r.player.stats[Stat.Health]) - previewGuard.expectedCounter) < 0.001);
    // Suspend Save preserves venom stacks
    const clone = new DungeonRun(assets);
    clone.deserializeState(r.serializeState());
    assert.equal(clone.playerVenomStacks, 1);
  }

  // 7) F2 Ghost Battle 1: Restless Zombie — Undying Resilience
  {
    const r = reachBattle(Biome.Ice, Biome.Ghost, 2, false, Difficulty.Easy);
    assert.equal(r.enemyName(), "Restless Zombie");
    assert.equal(r.enemyTraitName(), "Undying Resilience");
    r.choose(0);
    assert.equal(r.bossIntent().undyingActive, true);
    // Force low enemy HP and zero Accuracy so Standard Strike cannot crit -> triggers Undying at 1 HP
    r.enemyHealth = 5;
    r.player.stats[Stat.Accuracy] = 0;
    const prevStd = r.previewCombatAction(CombatAction.StandardStrike);
    assert.ok(prevStd.expectedCounter > 0, "Preview knows Standard Strike won't bypass Undying");
    r.attack(CombatAction.StandardStrike);
    assert.equal(r.enemyHealth, 1);
    assert.equal(r.enemyUndyingUsed, true);
    assert.equal(r.lastCombatExchange.undyingTriggered, true);
    // Now Heavy Strike kills cleanly (or any hit kills once spent)
    const r2 = reachBattle(Biome.Ice, Biome.Ghost, 2, false, Difficulty.Easy);
    r2.choose(0);
    r2.enemyHealth = 5;
    r2.player.stats[Stat.Accuracy] = 0;
    const prevHeavy = r2.previewCombatAction(CombatAction.HeavyStrike);
    assert.equal(prevHeavy.expectedCounter, 0, "Preview knows Heavy Strike bypasses Undying");
    r2.attack(CombatAction.HeavyStrike);
    assert.equal(r2.enemyHealth, 0);
    assert.equal(r2.lastCombatExchange.undyingTriggered, false);
  }

  // 8) F2 Ghost Battle 2: The Werewolf — Blood Frenzy
  {
    const r = reachBattle(Biome.Ice, Biome.Ghost, 2, true, Difficulty.Hard);
    assert.equal(r.enemyName(), "The Werewolf");
    assert.equal(r.enemyTraitName(), "Blood Frenzy");
    r.choose(0);
    assert.equal(r.bossIntent().counterMultiplier, 1);
    assert.equal(r.bossIntent().lifestealRatio, 0);
    r.enemyHealth = r.enemyMaxHealth() * 0.4;
    r.player.stats[Stat.Strength] = 4;
    r.player.stats[Stat.Accuracy] = 0;
    assert.equal(r.bossIntent().telegraphed, true);
    assert.equal(r.bossIntent().counterMultiplier, 1.45);
    assert.equal(r.bossIntent().lifestealRatio, 1.0);
    r.attack(CombatAction.GuardStrike);
    assert.ok(r.lastCombatExchange.enemyHealed > 0, "Frenzied Werewolf heals on counterattack");
  }

  // 9) F3 Lava Battle 1: Lava Mini Dragon — Searing Scales
  {
    const r = reachBattle(Biome.Ice, Biome.Ghost, 3, false, Difficulty.Hard);
    assert.equal(r.enemyName(), "Lava Mini Dragon");
    assert.equal(r.enemyTraitName(), "Searing Scales");
    r.choose(0);
    assert.equal(r.bossIntent().heavyBacklash, true);
    const prevHeavy = r.previewCombatAction(CombatAction.HeavyStrike);
    assert.ok(Math.abs(prevHeavy.expectedCounter - r.enemyDamage() * 1.25 * 1.30) < 0.001,
      "Lava Mini Dragon Heavy backlash preview is exact");
    const hpBefore = r.player.stats[Stat.Health];
    r.attack(CombatAction.HeavyStrike);
    if (r.enemyHealth > 0) {
      assert.ok(Math.abs((hpBefore - r.player.stats[Stat.Health]) - prevHeavy.expectedCounter) < 0.001);
    }
  }

  // 10) F3 Lava Battle 2: The Lava Knight — Volcanic Stance
  {
    const r = reachBattle(Biome.Ice, Biome.Ghost, 3, true, Difficulty.Hard);
    assert.equal(r.enemyName(), "The Lava Knight");
    assert.equal(r.enemyTraitName(), "Volcanic Stance");
    r.choose(0);
    assert.equal(r.bossIntent().name, "Volcanic Plate");
    assert.equal(r.bossIntent().damageTakenMultiplier, 0.65);
    assert.equal(r.bossIntent().heavyPierces, true);
    r.attack(CombatAction.GuardStrike);
    assert.equal(r.bossIntent().name, "Molten Cleave");
    assert.equal(r.bossIntent().counterMultiplier, 1.65);
    assert.equal(r.bossIntent().damageTakenMultiplier, 1);
  }

  console.log("Web Edition Engine, Prequel & Asset Tests PASSED: 220 route-filtered riddles, 10 unique monster battle traits, 4 endings, 22 milestones, Gauntlet Save/Resume, 3-Act Ashes of Falkreath, and Suspend Save verified.");
}

main().catch((err) => {
  console.error(err);
  process.exit(1);
});
