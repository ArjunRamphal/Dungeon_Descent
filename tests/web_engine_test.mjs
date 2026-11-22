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
import { WebStore, getCodexMilestones, isEndingUnlocked } from "../web/js/storage.js";

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const repoRoot = path.resolve(__dirname, "..");
const assetsDir = path.join(repoRoot, "assets");

async function main() {
  const assets = new AssetStore("./assets", async (rel) => {
    return fs.readFile(path.join(assetsDir, rel), "utf8");
  });
  await assets.preloadAll();

  // 1. Verify all 102 original riddles loaded cleanly
  assert.equal(assets.sourceRiddles[0].length, 30, "30 Easy riddles");
  assert.equal(assets.sourceRiddles[1].length, 37, "37 Medium riddles");
  assert.equal(assets.sourceRiddles[2].length, 35, "35 Hard riddles");

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

  // Verify Codex milestones (19 total)
  const codex = store.loadCodex();
  const ms = getCodexMilestones(codex);
  assert.equal(ms.length, 19, "Exactly 19 Codex milestones");
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
  assert.ok(isEndingUnlocked(codex, Ending.Good) && isEndingUnlocked(codex, Ending.True) &&
            isEndingUnlocked(codex, Ending.Bad) && isEndingUnlocked(codex, Ending.Perfect));

  // Verify Leaderboard submission & sorting
  const lb = store.submitLeaderboard(Difficulty.Easy, "ACE", perfectRun.score(), Hero.Mage, Ending.Perfect, perfectRun.seed, 195);
  assert.equal(lb.tables[Difficulty.Easy].length, 1);
  assert.equal(lb.tables[Difficulty.Easy][0].initials, "ACE");

  // Verify all 4 ending music files exist on disk
  for (const e of [Ending.Good, Ending.True, Ending.Bad, Ending.Perfect]) {
    const file = path.join(assetsDir, "music", endingMusicFile(e));
    const stat = await fs.stat(file);
    assert.ok(stat.size > 100000, `Ending audio exists: ${file}`);
  }

  console.log("Web Edition Engine & Asset Tests PASSED: 102 riddles, 4 endings, 19 milestones, Library/Shop shuffling, and Suspend Save verified.");
}

main().catch((err) => {
  console.error(err);
  process.exit(1);
});
