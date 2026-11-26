// Faithful 1:1 port of DungeonRun.cpp game rules, balance tables, boss intents,
// tactical combat, 102 riddles, seeded Library/Shop shuffling, and 4 branching endings.

import { MT19937, roomChoiceShuffleOrder } from "./rng.js";

export const Hero = Object.freeze({ Warrior: 0, Ranger: 1, Mage: 2, Rogue: 3 });
export const Difficulty = Object.freeze({ Easy: 0, Medium: 1, Hard: 2, Impossible: 3 });
export const Biome = Object.freeze({ Ice: 0, Jungle: 1, Desert: 2, Ghost: 3, Lava: 4 });
export const Room = Object.freeze({ Library: 0, Chest: 1, Battle: 2, Encounter: 3, Respite: 4, Shop: 5, Boss: 6 });
export const Phase = Object.freeze({ Room: 0, Outcome: 1, Combat: 2, Riddle: 3, RiddleOutcome: 4, Loot: 5, Complete: 6, Defeat: 7, Ending: 8 });
export const Ending = Object.freeze({ None: 0, Good: 1, True: 2, Bad: 3, Perfect: 4 });
export const Stat = Object.freeze({ Strength: 0, Wisdom: 1, Observation: 2, Agility: 3, Health: 4, Accuracy: 5 });
export const CombatAction = Object.freeze({ StandardStrike: 0, HeavyStrike: 1, GuardStrike: 2, ClassAbility: 3 });

// Difficulty [0..3], Floor [0..2], Encounter [opening, later, guardian]
const ENEMY_BALANCE = [
  [[{ health: 32, damage: 3 }, { health: 32, damage: 4 }, { health: 144, damage: 5 }],
   [{ health: 68, damage: 6 }, { health: 68, damage: 8 }, { health: 306, damage: 10 }],
   [{ health: 126, damage: 9 }, { health: 126, damage: 12 }, { health: 567, damage: 15 }]],
  [[{ health: 36, damage: 3 }, { health: 48, damage: 4.25 }, { health: 190, damage: 5.25 }],
   [{ health: 100, damage: 6.25 }, { health: 145, damage: 8.25 }, { health: 480, damage: 10.5 }],
   [{ health: 220, damage: 9.25 }, { health: 300, damage: 12.25 }, { health: 1000, damage: 15.5 }]],
  [[{ health: 48, damage: 3 }, { health: 60, damage: 4.5 }, { health: 230, damage: 5.25 }],
   [{ health: 125, damage: 6.5 }, { health: 175, damage: 8.5 }, { health: 550, damage: 10.5 }],
   [{ health: 280, damage: 9.5 }, { health: 350, damage: 12.5 }, { health: 1200, damage: 15.75 }]],
  [[{ health: 56, damage: 3 }, { health: 70, damage: 4.75 }, { health: 450, damage: 5.5 }],
   [{ health: 145, damage: 6.75 }, { health: 210, damage: 8.75 }, { health: 620, damage: 10.75 }],
   [{ health: 330, damage: 9.75 }, { health: 420, damage: 12.75 }, { health: 1450, damage: 16 }]]
];

export function formatNum(value) {
  let s = Number(value).toFixed(2);
  while (s.includes(".") && s.endsWith("0")) s = s.slice(0, -1);
  if (s.endsWith(".")) s = s.slice(0, -1);
  return s;
}

export function formatDuration(seconds) {
  const s = Math.max(0, Math.floor(seconds || 0));
  const h = Math.floor(s / 3600);
  const m = Math.floor((s % 3600) / 60);
  const sec = s % 60;
  return `${h}:${String(m).padStart(2, "0")}:${String(sec).padStart(2, "0")}`;
}

export function makePlayer(hero) {
  const base = {
    hero,
    stats: [0, 0, 0, 0, 0, 0],
    reputation: 0,
    battlesFought: 0,
    battlesWon: 0,
    strikesLeftover: 0,
    riddlesAnswered: 0,
    riddlesCorrect: 0,
    riddleStreak: 0,
    bestRiddleStreak: 0,
    npcKills: 0,
    classAbilitiesUsed: 0
  };
  switch (hero) {
    case Hero.Warrior: base.stats = [7, 1, 0, 3, 35, 2]; break;
    case Hero.Ranger:  base.stats = [2, 2, 4, 2, 30, 8]; break;
    case Hero.Mage:    base.stats = [1, 7, 3, 1, 40, 1]; break;
    case Hero.Rogue:   base.stats = [2, 0, 4, 8, 30, 5]; break;
  }
  return base;
}

export function heroName(hero) {
  return ["Warrior", "Ranger", "Mage", "Rogue"][hero] || "Warrior";
}

export function heroWeapon(hero) {
  return ["Sword", "Longbow", "Wand", "Throwing knife"][hero] || "Sword";
}

export function difficultyName(diff) {
  return ["Easy", "Medium", "Hard", "Impossible"][diff] || "Easy";
}

export function biomeName(biome) {
  return ["The Frozen Depths", "The Overgrown Halls", "The Sunken Sands", "The Haunted Halls", "Malvel's Domain"][biome] || "";
}

export function biomeKey(biome) {
  return ["ice", "jungle", "desert", "ghost", "lava"][biome] || "ice";
}

export function biomeColor(biome) {
  return ["#7bc6e0", "#77c693", "#e0b571", "#a9c0d3", "#ee8559"][biome] || "#dfbb77";
}

export function portraitImage(hero) {
  const name = heroName(hero);
  return `images/pfps/${name}${(hero === Hero.Mage || hero === Hero.Ranger) ? ".jpeg" : ".jpg"}`;
}

export function bossImage(biome) {
  const key = biomeKey(biome);
  return `images/biomes/${key}/battle/${key}boss${(biome === Biome.Desert || biome === Biome.Ghost) ? ".jpeg" : ".jpg"}`;
}

export function heroAbilitySummary(hero) {
  switch (hero) {
    case Hero.Warrior: return "Runic might / +3 Strength\n1.5x Strength & Health | Shield Bash";
    case Hero.Ranger:  return "Steady aim / +3 Accuracy\n1.5x Accuracy & Health | Precision Shot";
    case Hero.Mage:    return "Arcane insight / +3 Wisdom\n1.5x Wisdom & Health | Arcane Surge";
    case Hero.Rogue:   return "Shadow step / +3 Agility\n1.5x Agility & Health | Shadow Step";
    default: return "";
  }
}

export function combatAbilityName(hero) {
  return ["Shield Bash", "Precision Shot", "Arcane Surge", "Shadow Step"][hero] || "";
}

export function combatAbilityDescription(hero) {
  switch (hero) {
    case Hero.Warrior: return "1.35x Strength strike and staggers the enemy to block its counterattack (once per battle).";
    case Hero.Ranger:  return "Guaranteed critical hit for 2.5x Accuracy damage (once per battle).";
    case Hero.Mage:    return "1.5x Wisdom strike and restores 3 + floor Health (once per battle).";
    case Hero.Rogue:   return "1.15x Agility strike, evades counterattack, and refunds its strike (once per battle).";
    default: return "";
  }
}

export function statName(stat) {
  return ["Strength", "Wisdom", "Observation", "Agility", "Health", "Accuracy"][stat] || "";
}

export function statTooltip(stat) {
  switch (stat) {
    case Stat.Strength:    return "Strength: Primary combat damage for the Warrior (and scales Sword strikes).";
    case Stat.Wisdom:      return "Wisdom: Primary combat damage for the Mage and adds floor(Wisdom / 2)s to riddle timers.";
    case Stat.Observation: return "Observation: Above 30 eliminates one false answer on every riddle.";
    case Stat.Agility:     return "Agility: Primary combat damage for the Rogue and adds floor(Agility / 3) extra strikes per battle.";
    case Stat.Health:      return "Health: Your growing life pool. Reaching 0 in combat or riddles ends the descent.";
    case Stat.Accuracy:    return "Accuracy: Primary combat damage for the Ranger and grants Accuracy / 35 chance for 2x critical hits.";
    default: return "";
  }
}

export function roomName(room) {
  return ["Library", "Sealed chest", "Battle", "The prisoner", "Respite", "Surtees' shop", "The guardian"][room] || "";
}

export function endingKey(ending) {
  switch (ending) {
    case Ending.Good: return "goodending";
    case Ending.True: return "trueending";
    case Ending.Bad: return "badending";
    case Ending.Perfect: return "perfectending";
    default: return "";
  }
}

export function endingName(ending) {
  switch (ending) {
    case Ending.Good: return "Hero of Aethoria?";
    case Ending.True: return "Justice for Falkreath";
    case Ending.Bad: return "Dungeon Demise";
    case Ending.Perfect: return "A Crown in Ashes";
    default: return "Game over";
  }
}

export function endingHint(ending) {
  switch (ending) {
    case Ending.Good: return "Defeat Malvel on Floor 3 without uncovering the full truth of Falkreath.";
    case Ending.True: return "Defeat Malvel with at least 5 Reputation and strong battle or riddle mastery.";
    case Ending.Bad: return "Fall to Malvel in the final chamber of Floor 3.";
    case Ending.Perfect: return "Achieve a flawless descent: win all 9 battles, solve all 24 riddles, and earn 9/9 Reputation.";
    default: return "";
  }
}

export function endingImage(ending) {
  const k = endingKey(ending);
  return k ? `images/ending/${k}.jpg` : "";
}

export function endingStoryFile(ending) {
  const k = endingKey(ending);
  return k ? `textfiles/ending/${k}.txt` : "";
}

export function endingMusicFile(ending) {
  switch (ending) {
    case Ending.Good: return "ending_good.ogg";
    case Ending.True: return "ending_true.ogg";
    case Ending.Bad: return "ending_bad.ogg";
    case Ending.Perfect: return "ending_perfect.ogg";
    default: return "menu.mp3";
  }
}

export function resolveEnding(malvelDefeated, player) {
  if (!malvelDefeated) return Ending.Bad;
  if (player.battlesFought === 9 && player.battlesWon === 9 &&
      player.riddlesAnswered === 24 && player.riddlesCorrect === 24 &&
      player.reputation === 9) {
    return Ending.Perfect;
  }
  return (player.reputation >= 5 && (player.riddlesCorrect >= 14 || player.battlesWon >= 5))
    ? Ending.True : Ending.Good;
}

export class DungeonRun {
  constructor(assetStore) {
    this.assets = assetStore;
    this.player = makePlayer(Hero.Warrior);
    this.difficulty = Difficulty.Easy;
    this.biome = Biome.Ice;
    this.firstBiome = Biome.Ice;
    this.secondBiome = null;
    this.floorNumber = 1;
    this.phase = Phase.Room;
    this.ending = Ending.None;
    this.started = false;
    this.seed = 0;
    this.runSerial = 0;
    this.rooms = [];
    this.roomIndex = 0;
    this.riddles = [];
    this.riddleOrder = [];
    this.hiddenAnswer = null;
    this.rng = new MT19937(1);
    this.chestUnlocked = false;
    this.classAbilityUsed = false;
    this.timeLeft = 0;
    this.timeLimit = 0;
    this.enemyHealth = 0;
    this.strikesLeft = 0;
    this.bossTurn = 1;
    this.enemyShellBroken = false;
    this.enemyUndyingUsed = false;
    this.playerVenomStacks = 0;
    this.message = "";
    this.lastCombatExchange = null;
    this.lastStatDeltas = [0, 0, 0, 0, 0, 0];
    this.lastReputationDelta = 0;
  }

  start(hero, biome, seed, difficulty = Difficulty.Easy) {
    if (biome !== Biome.Ice && biome !== Biome.Jungle) {
      throw new Error("A new descent begins with the Ice or Jungle biome.");
    }
    this.seed = (seed >>> 0);
    this.rng = new MT19937(this.seed);
    this.player = makePlayer(hero);
    this.difficulty = difficulty;
    this.firstBiome = biome;
    this.secondBiome = null;
    this.biome = biome;
    this.floorNumber = 1;
    this.ending = Ending.None;
    this.started = true;
    this.runSerial++;
    this.clearStatDeltas();
    this.lastCombatExchange = null;
    this.prepareFloor();
  }

  canDescend() {
    return this.started && this.phase === Phase.Complete && this.floorNumber < 3;
  }

  inProgress() {
    return this.started && this.phase !== Phase.Defeat && this.phase !== Phase.Ending;
  }

  descend(biome) {
    if (!this.canDescend()) return false;
    if (this.floorNumber === 1 && biome !== Biome.Desert && biome !== Biome.Ghost) return false;
    if (this.floorNumber === 2 && biome !== Biome.Lava) return false;
    if (this.floorNumber === 1) this.secondBiome = biome;
    this.floorNumber++;
    this.biome = biome;
    this.clearStatDeltas();
    this.lastCombatExchange = null;
    this.prepareFloor();
    return true;
  }

  prepareFloor() {
    const tier = this.assets.sourceRiddles[this.floorNumber - 1];
    this.riddles = tier.map((r) => ({
      question: r.question,
      answers: [...r.answers],
      correct: r.correct,
      requiredBiome: r.requiredBiome ?? null
    }));
    this.roomIndex = 0;
    if (this.floorNumber === 1) {
      this.rooms = [Room.Library, Room.Chest, Room.Battle, Room.Encounter, Room.Battle, Room.Respite, Room.Library, Room.Shop, Room.Boss];
    } else if (this.floorNumber === 2) {
      this.rooms = [Room.Library, Room.Chest, Room.Battle, Room.Shop, Room.Chest, Room.Encounter, Room.Respite, Room.Battle, Room.Boss];
    } else {
      this.rooms = [Room.Library, Room.Chest, Room.Battle, Room.Chest, Room.Battle, Room.Shop, Room.Encounter, Room.Respite, Room.Boss];
    }
    this.rng.shuffle(this.rooms, 0, 3);
    this.riddleOrder = [];
    for (let idx = 0; idx < this.riddles.length; idx++) {
      const req = this.riddles[idx].requiredBiome;
      const eligible = req === null || req === this.firstBiome || req === this.biome ||
        (this.secondBiome !== null && req === this.secondBiome);
      if (eligible) this.riddleOrder.push(idx);
    }
    this.rng.shuffle(this.riddleOrder);
    this.enterRoom();
  }

  enterRoom() {
    this.phase = Phase.Room;
    this.chestUnlocked = false;
    this.classAbilityUsed = false;
    this.enemyHealth = 0;
    this.strikesLeft = 0;
    this.bossTurn = 1;
    this.enemyShellBroken = false;
    this.enemyUndyingUsed = false;
    this.playerVenomStacks = 0;
    this.timeLeft = 0;
    this.timeLimit = 0;
    this.hiddenAnswer = null;
    this.message = "";
    this.clearStatDeltas();
    this.lastCombatExchange = null;
  }

  room() { return this.rooms[this.roomIndex]; }
  roomNumber() { return this.roomIndex + 1; }
  riddle() { return this.riddles[this.riddleOrder[this.roomIndex]]; }
  hasObservationHint() { return this.phase === Phase.Riddle && this.hiddenAnswer !== null; }
  answerAvailable(choice) { return choice < 3 && (this.hiddenAnswer === null || this.hiddenAnswer !== choice); }
  classAbilityAvailable() { return this.phase === Phase.Combat && !this.classAbilityUsed && this.strikesLeft >= 1; }
  classAbilityUsedInBattle() { return this.classAbilityUsed; }

  clearStatDeltas() {
    this.lastStatDeltas = [0, 0, 0, 0, 0, 0];
    this.lastReputationDelta = 0;
  }

  recordStatDeltas(beforeStats, beforeRep) {
    for (let i = 0; i < 6; i++) {
      this.lastStatDeltas[i] = this.player.stats[i] - beforeStats[i];
    }
    this.lastReputationDelta = this.player.reputation - beforeRep;
  }

  addStat(stat, amount) {
    const h = this.player.hero;
    const bonus =
      (h === Hero.Warrior && stat === Stat.Strength) ||
      (h === Hero.Ranger && stat === Stat.Accuracy) ||
      (h === Hero.Mage && stat === Stat.Wisdom) ||
      (h === Hero.Rogue && stat === Stat.Agility);
    this.player.stats[stat] += amount * (bonus ? 1.5 : 1);
  }

  addAll(amount) {
    const h = this.player.hero;
    for (let s = 0; s < 6; s++) {
      const bonus = s === Stat.Health ||
        (h === Hero.Warrior && s === Stat.Strength) ||
        (h === Hero.Ranger && s === Stat.Accuracy) ||
        (h === Hero.Mage && s === Stat.Wisdom) ||
        (h === Hero.Rogue && s === Stat.Agility);
      this.player.stats[s] += amount * this.floorNumber * (bonus ? 1.5 : 1);
    }
  }

  subtractAll(amount) {
    for (let s = 0; s < 6; s++) {
      this.player.stats[s] = Math.max(0, this.player.stats[s] - amount * this.floorNumber);
    }
  }

  outcome(text) {
    this.message = text;
    this.phase = this.player.stats[Stat.Health] <= 0 ? Phase.Defeat : Phase.Outcome;
  }

  primaryCombatStat() {
    if (this.player.hero === Hero.Ranger) return Stat.Accuracy;
    if (this.player.hero === Hero.Mage) return Stat.Wisdom;
    if (this.player.hero === Hero.Rogue) return Stat.Agility;
    return Stat.Strength;
  }

  enemyMaxHealth() {
    const encounter = this.room() === Room.Boss ? 2 : (this.roomNumber() <= 3 ? 0 : 1);
    return ENEMY_BALANCE[this.difficulty][this.floorNumber - 1][encounter].health;
  }

  enemyDamage() {
    const encounter = this.room() === Room.Boss ? 2 : (this.roomNumber() <= 3 ? 0 : 1);
    return ENEMY_BALANCE[this.difficulty][this.floorNumber - 1][encounter].damage;
  }

  bossIntent() {
    const intent = {
      active: false,
      isBoss: false,
      telegraphed: false,
      name: "",
      detail: "",
      counterMultiplier: 1,
      damageTakenMultiplier: 1,
      lifestealRatio: 0,
      drainsStrike: false,
      heavyPierces: false,
      guardBypasses: false,
      critPierces: false,
      shattersOnPierce: false,
      heavyBacklash: false,
      heavyCounterBonus: 1,
      regenRatio: 0,
      appliesVenom: false,
      undyingActive: false
    };
    if (this.phase !== Phase.Combat || (this.room() !== Room.Boss && this.room() !== Room.Battle)) return intent;
    intent.active = true;
    if (this.room() === Room.Boss) {
      intent.isBoss = true;
      const beat = this.bossTurn % 3;
      switch (this.biome) {
        case Biome.Ice:
          if (beat === 0) {
            intent.telegraphed = true;
            intent.name = "Glacial Slam";
            intent.detail = "2x counterattack! Guard or negate to survive.";
            intent.counterMultiplier = 2;
          } else {
            intent.name = "Gathering Frost";
            intent.detail = beat === 2 ? "Next turn: Glacial Slam (2x counter)." : "Standard retaliation.";
          }
          break;
        case Biome.Jungle:
          if (beat === 0) {
            intent.telegraphed = true;
            intent.name = "Thorned Carapace";
            intent.detail = "1.6x counter on open strikes; Guard or Ability bypasses thorns.";
            intent.counterMultiplier = 1.6;
            intent.guardBypasses = true;
          } else {
            intent.name = "Coiling Vines";
            intent.detail = beat === 2 ? "Next turn: Thorned Carapace (1.6x thorns)." : "Standard retaliation.";
          }
          break;
        case Biome.Desert:
          if (beat === 0) {
            intent.telegraphed = true;
            intent.name = "Curse of the Sands";
            intent.detail = "1.75x counter + drains 1 strike unless Guarded/negated.";
            intent.counterMultiplier = 1.75;
            intent.drainsStrike = true;
          } else {
            intent.name = "Shifting Dunes";
            intent.detail = beat === 2 ? "Next turn: Curse of the Sands (drains 1 strike)." : "Standard retaliation.";
          }
          break;
        case Biome.Ghost:
          if (beat === 0) {
            intent.telegraphed = true;
            intent.name = "Blood Siphon";
            intent.detail = "1.5x counter + heals 2x damage dealt! Guard to starve heal.";
            intent.counterMultiplier = 1.5;
            intent.lifestealRatio = 2;
          } else {
            intent.name = "Crimson Gaze";
            intent.detail = beat === 2 ? "Next turn: Blood Siphon (heals on counter)." : "Standard retaliation.";
          }
          break;
        case Biome.Lava:
          if (beat === 0) {
            intent.telegraphed = true;
            intent.name = "Obsidian Cataclysm";
            intent.detail = "2.25x counterattack! Guard or negate this turn.";
            intent.counterMultiplier = 2.25;
          } else if (beat === 2) {
            intent.telegraphed = true;
            intent.name = "Arcane Ward";
            intent.detail = "50% damage reduction vs Standard/Guard; Heavy & Ability pierce!";
            intent.damageTakenMultiplier = 0.5;
            intent.heavyPierces = true;
          } else {
            intent.name = "Molten Focus";
            intent.detail = "Next turn: Arcane Ward (Heavy/Ability pierces).";
          }
          break;
      }
      return intent;
    }

    // Ordinary monster battle traits (10 unique encounters across the 5 biomes)
    switch (this.biome) {
      case Biome.Ice:
        if (this.roomNumber() === 5) {
          if (!this.enemyShellBroken) {
            intent.telegraphed = true;
            intent.name = "Glacial Shell";
            intent.detail = "30% dmg reduction vs Standard/Guard. Heavy, Crit, or Ability shatters shell!";
            intent.damageTakenMultiplier = 0.70;
            intent.heavyPierces = true;
            intent.critPierces = true;
            intent.shattersOnPierce = true;
          } else {
            intent.name = "Glacial Shell (Shattered)";
            intent.detail = "Ice armor cracked — all attacks deal full damage!";
          }
        } else {
          if (this.bossTurn % 2 === 0) {
            intent.telegraphed = true;
            intent.name = "Runic Frost Club";
            intent.detail = "1.5x frozen club counterattack! Guard or negate this turn.";
            intent.counterMultiplier = 1.5;
          } else {
            intent.name = "Runic Frost Club";
            intent.detail = "Standard counter now. Next turn: 1.5x frozen club swing!";
          }
        }
        break;
      case Biome.Jungle:
        if (this.roomNumber() === 5) {
          if (this.bossTurn % 3 === 2) {
            intent.telegraphed = true;
            intent.name = "Tangled Vines";
            intent.detail = "Whipping vines drain 1 strike on unguarded counter! Guard or negate.";
            intent.drainsStrike = true;
          } else {
            intent.name = "Tangled Vines";
            intent.detail = this.bossTurn % 3 === 1
              ? "Next turn: Tangled Vines (drains 1 strike unless Guarded)."
              : "Vines coiling — standard retaliation this turn.";
          }
        } else {
          intent.regenRatio = 0.10;
          intent.telegraphed = this.enemyHealth < this.enemyMaxHealth();
          intent.name = "Moss Regeneration";
          intent.detail = "Regenerates 10% Max HP after each exchange it survives. Burst it down!";
        }
        break;
      case Biome.Desert:
        if (this.roomNumber() === 8) {
          intent.appliesVenom = true;
          intent.telegraphed = this.playerVenomStacks > 0;
          intent.name = this.playerVenomStacks > 0
            ? `Venomous Stinger (Venom x${this.playerVenomStacks})`
            : "Venomous Stinger";
          intent.detail = this.playerVenomStacks > 0
            ? "Active Venom deals +20% counter dmg/stack! Guard to prevent more stacks."
            : "Unguarded counters inject Venom (+20% dmg/stack, max 3). Guard to avoid!";
        } else {
          intent.damageTakenMultiplier = 0.75;
          intent.heavyPierces = true;
          intent.critPierces = true;
          if (this.bossTurn === 1) {
            intent.telegraphed = true;
            intent.counterMultiplier = 1.35;
            intent.name = "Sandstone Ambush";
            intent.detail = "Turn 1 eruption: 1.35x counter! Plates cut open hits 25% (Heavy/Crit/Ability pierce).";
          } else {
            intent.name = "Armored Scales";
            intent.detail = "25% dmg reduction vs non-crit Standard/Guard. Heavy, Crit & Ability pierce!";
          }
        }
        break;
      case Biome.Ghost:
        if (this.roomNumber() === 8) {
          if (this.enemyHealth <= this.enemyMaxHealth() * 0.5) {
            intent.telegraphed = true;
            intent.counterMultiplier = 1.45;
            intent.lifestealRatio = 1.0;
            intent.name = "Blood Frenzy";
            intent.detail = "Frenzied (<50% HP): 1.45x counter + heals 100% of counter dmg! Guard or finish!";
          } else {
            intent.name = "Blood Frenzy (Dormant)";
            intent.detail = "Below 50% HP gains 1.45x counter + lifesteal. Burst through 50%!";
          }
        } else {
          if (!this.enemyUndyingUsed) {
            intent.undyingActive = true;
            intent.heavyPierces = true;
            intent.critPierces = true;
            intent.telegraphed = this.enemyHealth <= this.enemyMaxHealth() * 0.5;
            intent.name = "Undying Resilience";
            intent.detail = "Survives one killing blow at 1 HP — Heavy Strike, Crit, or Ability destroys it!";
          } else {
            intent.name = "Undying Resilience (Spent)";
            intent.detail = "Cannot cling to unlife again — any lethal blow finishes it!";
          }
        }
        break;
      case Biome.Lava:
        if (this.roomNumber() === 5) {
          if (this.bossTurn % 2 === 1) {
            intent.telegraphed = true;
            intent.damageTakenMultiplier = 0.65;
            intent.heavyPierces = true;
            intent.name = "Volcanic Plate";
            intent.detail = "35% dmg reduction vs Standard/Guard; Heavy & Ability pierce! (Next: Molten Cleave)";
          } else {
            intent.telegraphed = true;
            intent.counterMultiplier = 1.65;
            intent.name = "Molten Cleave";
            intent.detail = "1.65x molten counterattack! Guard or negate this turn. (Next: Volcanic Plate)";
          }
        } else {
          intent.telegraphed = true;
          intent.heavyBacklash = true;
          intent.heavyCounterBonus = 1.30;
          intent.name = "Searing Scales";
          intent.detail = "Heavy Strikes trigger +30% extra counter backlash! Favor Standard, Guard, or Ability.";
        }
        break;
    }
    return intent;
  }

  enemyTraitName() {
    if (this.room() === Room.Boss) {
      return ["Glacial Slam", "Thorned Carapace", "Curse of the Sands", "Blood Siphon", "Arcane Ward & Cataclysm"][this.biome] || "";
    }
    switch (this.biome) {
      case Biome.Ice:    return this.roomNumber() === 5 ? "Glacial Shell" : "Runic Frost Club";
      case Biome.Jungle: return this.roomNumber() === 5 ? "Tangled Vines" : "Moss Regeneration";
      case Biome.Desert: return this.roomNumber() === 8 ? "Venomous Stinger" : "Sandstone Ambush";
      case Biome.Ghost:  return this.roomNumber() === 8 ? "Blood Frenzy" : "Undying Resilience";
      case Biome.Lava:   return this.roomNumber() === 5 ? "Volcanic Stance" : "Searing Scales";
      default: return "";
    }
  }

  enemyTraitSummary() {
    if (this.room() === Room.Boss) {
      return [
        "2x counterattack every 3rd turn — Guard or negate.",
        "1.6x thorns every 3rd turn — Guard or Ability bypasses.",
        "1.75x counter + strike drain every 3rd turn — Guard to block.",
        "1.5x counter + 2x lifesteal every 3rd turn — Guard to starve heal.",
        "Turn 2 Arcane Ward (Heavy pierces); Turn 3 2.25x Cataclysm."
      ][this.biome] || "";
    }
    switch (this.biome) {
      case Biome.Ice:
        return this.roomNumber() === 5
          ? "30% dmg reduction vs open hits; Heavy/Crit/Ability shatters shell."
          : "1.5x counterattack on every 2nd turn — Guard on even turns.";
      case Biome.Jungle:
        return this.roomNumber() === 5
          ? "Drains 1 strike on turn 2 (and every 3rd turn after) unless Guarded."
          : "Regenerates 10% Max HP after every exchange it survives.";
      case Biome.Desert:
        return this.roomNumber() === 8
          ? "Unguarded counters stack Venom (+20% dmg/stack, max 3)."
          : "Turn 1 1.35x ambush + 25% plate armor (Heavy/Crit/Ability pierce).";
      case Biome.Ghost:
        return this.roomNumber() === 8
          ? "Below 50% HP: 1.45x counterattack + 100% counter lifesteal."
          : "Clings to 1 HP once unless slain by Heavy Strike, Crit, or Ability.";
      case Biome.Lava:
        return this.roomNumber() === 5
          ? "Odd turns: 35% Volcanic Plate (Heavy pierces). Even: 1.65x Cleave."
          : "Heavy Strikes provoke +30% extra counter backlash — avoid Heavy.";
      default: return "";
    }
  }

  combatChoiceAvailable(choice) {
    if (this.phase !== Phase.Combat || this.strikesLeft <= 0) return false;
    switch (choice) {
      case 0: return this.strikesLeft >= 1;
      case 1: return this.strikesLeft >= 2;
      case 2: return this.strikesLeft >= 1;
      case 3: return !this.classAbilityUsed && this.strikesLeft >= 1;
      default: return false;
    }
  }

  previewCombatAction(action) {
    const base = Math.max(1, this.player.stats[this.primaryCombatStat()]);
    const intent = this.bossIntent();
    let title = "";
    let strikeCost = 1;
    let available = false;
    let mult = 1;
    let counterMult = 1;
    let guaranteedCrit = false;
    let piercesWard = false;
    let bypassesThorns = false;
    let guardedAction = false;
    let effect = "";

    switch (action) {
      case CombatAction.StandardStrike:
        title = "Standard Strike";
        strikeCost = 1;
        available = this.combatChoiceAvailable(0);
        effect = intent.undyingActive
          ? "Balanced 1-strike attack (non-crit won't bypass Undying)."
          : "Balanced 1-strike attack.";
        break;
      case CombatAction.HeavyStrike:
        title = "Heavy Strike";
        strikeCost = 2;
        available = this.combatChoiceAvailable(1);
        mult = 1.85;
        counterMult = 1.25 * (intent.heavyBacklash ? intent.heavyCounterBonus : 1);
        piercesWard = true;
        if (intent.heavyBacklash) {
          effect = "Costs 2 strikes, 1.85x dmg — WARNING: Searing Scales backlash!";
        } else if (intent.shattersOnPierce) {
          effect = "Costs 2 strikes, 1.85x dmg — Shatters Glacial Shell!";
        } else if (intent.undyingActive) {
          effect = "Costs 2 strikes, 1.85x dmg — Destroys Undying cleanly!";
        } else {
          effect = `Costs 2 strikes, 1.85x dmg, 1.25x counter${intent.heavyPierces && intent.damageTakenMultiplier < 1 ? " (Pierces armor!)" : "."}`;
        }
        break;
      case CombatAction.GuardStrike:
        title = "Guard Strike";
        strikeCost = 1;
        available = this.combatChoiceAvailable(2);
        mult = 0.75;
        counterMult = 0.25;
        bypassesThorns = true;
        guardedAction = true;
        if (intent.appliesVenom) {
          effect = "0.75x dmg, cuts counter to 25% & blocks new Venom stacks.";
        } else {
          effect = `0.75x dmg, cuts counter to 25%${intent.drainsStrike ? " & blocks strike drain." : "."}`;
        }
        break;
      case CombatAction.ClassAbility:
        title = combatAbilityName(this.player.hero);
        strikeCost = 1;
        available = this.combatChoiceAvailable(3);
        piercesWard = true;
        bypassesThorns = true;
        if (this.player.hero === Hero.Warrior) {
          mult = 1.35; counterMult = 0; guardedAction = true;
          effect = "1.35x dmg + blocks counterattack (once/battle).";
        } else if (this.player.hero === Hero.Ranger) {
          mult = 2.5; counterMult = 1; guaranteedCrit = true;
          effect = "Guaranteed 2.5x crit (once/battle).";
        } else if (this.player.hero === Hero.Mage) {
          mult = 1.5; counterMult = 1;
          effect = `1.5x dmg + heals ${3 + this.floorNumber} HP (once/battle).`;
        } else {
          mult = 1.15; counterMult = 0; guardedAction = true;
          effect = "1.15x dmg, evades counter & refunds strike.";
        }
        break;
    }

    const nonCritWardMult = (intent.damageTakenMultiplier < 1 && !(intent.heavyPierces && piercesWard) && !(intent.critPierces && guaranteedCrit))
      ? intent.damageTakenMultiplier : 1;
    const critWardMult = (intent.damageTakenMultiplier < 1 && !(intent.heavyPierces && piercesWard) && !intent.critPierces)
      ? intent.damageTakenMultiplier : 1;
    const minDamage = Math.max(1, base * mult * nonCritWardMult);
    const critDamage = guaranteedCrit ? minDamage : Math.max(1, base * mult * 2 * critWardMult);
    const guaranteedKill = (this.phase === Phase.Combat) && (minDamage >= this.enemyHealth) &&
                           !(intent.undyingActive && !piercesWard && !guaranteedCrit);
    let expectedCounter = 0;
    if (!guaranteedKill) {
      const bossCounterMult = (intent.guardBypasses && bypassesThorns) ? 1 : intent.counterMultiplier;
      const retaliation = this.enemyDamage() * counterMult * bossCounterMult;
      let projectedVenom = this.playerVenomStacks;
      if (intent.appliesVenom && retaliation > 0 && !guardedAction) {
        projectedVenom = Math.min(3, projectedVenom + 1);
      }
      const venomTick = projectedVenom > 0 ? (this.enemyDamage() * 0.20 * projectedVenom) : 0;
      expectedCounter = retaliation + venomTick;
    }

    return { title, effect, strikeCost, minDamage, critDamage, expectedCounter, available };
  }

  combatSummary() {
    const base = Math.max(1, this.player.stats[this.primaryCombatStat()]);
    const critChance = Math.max(0, Math.min(1, this.player.stats[Stat.Accuracy] / 35));
    const agiBonus = Math.floor(Math.max(0, this.player.stats[Stat.Agility]) / 3);
    const abilityMult = [1.35, 2.5, 1.5, 1.15][this.player.hero];
    return {
      baseDamage: base,
      heavyDamage: base * 1.85,
      guardDamage: Math.max(1, base * 0.75),
      abilityDamage: base * abilityMult,
      critChance,
      critDamage: base * 2,
      minStrikes: 3 + agiBonus,
      maxStrikes: 5 + agiBonus,
      enemyMaxHealth: this.enemyMaxHealth(),
      enemyHealth: this.phase === Phase.Combat ? this.enemyHealth : this.enemyMaxHealth(),
      enemyCounterDamage: this.enemyDamage(),
      guardCounterDamage: Math.max(0, this.enemyDamage() * 0.25)
    };
  }

  roomChoiceOrder() {
    if (this.phase !== Phase.Room) return [];
    const r = this.room();
    if (r !== Room.Library && r !== Room.Shop) return [];
    return roomChoiceShuffleOrder(this.seed, this.floorNumber, this.roomIndex, r === Room.Library);
  }

  displayChoiceFor(canonicalChoice) {
    const order = this.roomChoiceOrder();
    if (!order.length) return canonicalChoice;
    const idx = order.indexOf(canonicalChoice);
    return idx >= 0 ? idx : canonicalChoice;
  }

  choices() {
    if (this.phase === Phase.Combat) {
      return [
        "Standard Strike",
        "Heavy Strike (2 strikes)",
        "Guard Strike",
        combatAbilityName(this.player.hero) + (this.classAbilityUsed ? " (Used)" : "")
      ];
    }
    if (this.phase !== Phase.Room) return [];
    const reorder = (canonical) => this.roomChoiceOrder().map((idx) => canonical[idx]);
    switch (this.room()) {
      case Room.Library:
        if (this.biome === Biome.Lava) return reorder(["The Princess Diary", "The Smoldering Whisper", "The Magma Script", "The Ashen Eye"]);
        if (this.biome === Biome.Desert) return reorder(["The Sands of Strategy", "The Silent Zephyrs", "The Hidden Glyphs", "The Oasis Eye"]);
        if (this.biome === Biome.Ghost) return reorder(["The Cyclops Might", "The Weeping Wolfman", "The Immortal Alucard", "The Eyes of the Raven"]);
        return reorder(["The Art of Warfare", "The Whispering Winds", "The Ancient Lore", "The Heart of the Forest"]);
      case Room.Chest: return ["Read the seal's riddle"];
      case Room.Battle: return ["Confront the creature", "Sneak past"];
      case Room.Encounter:
        if (this.floorNumber === 3) return ["Threaten minions", "Kill minions"];
        return this.floorNumber === 1 ? ["Free the warrior", "Take his vitality"] : ["Save the butler", "Take the vial"];
      case Room.Respite:
        return [this.floorNumber === 3 ? "Rest in the sauna" : "Drink from the spring", "Search the room"];
      case Room.Shop:
        if (this.biome === Biome.Lava) return reorder(["Emberheart Elixir", "Corebound Tincture", "Inferno Draught"]);
        if (this.biome === Biome.Desert) return reorder(["Draught of Revitalization", "Essence of Fortitude", "Scroll of Ancient Wisdom"]);
        if (this.biome === Biome.Ghost) return reorder(["Elixir of Restless Slumber", "Phantom's Breath", "Scroll of Lost Echoes"]);
        return reorder(["Health Potion", "Stat Increase Potion", "Book of Knowledge"]);
      case Room.Boss:
        return [this.floorNumber === 3 ? "Face Malvel" : "Face the guardian"];
      default:
        return [];
    }
  }

  choose(choice) {
    if (this.phase === Phase.Combat) {
      if (choice >= 0 && choice < 4) this.attack(choice);
      return;
    }
    const list = this.choices();
    if (this.phase !== Phase.Room || choice < 0 || choice >= list.length) return;
    const beforeStats = [...this.player.stats];
    const beforeRep = this.player.reputation;

    switch (this.room()) {
      case Room.Chest:
        this.beginRiddle();
        return;
      case Room.Library: {
        const canonical = this.roomChoiceOrder()[choice];
        const stats = [Stat.Strength, Stat.Agility, Stat.Wisdom, Stat.Observation];
        const stat = stats[canonical];
        const before = this.player.stats[stat];
        this.addStat(stat, 2);
        this.outcome(`You study the book. ${statName(stat)} increased by ${formatNum(this.player.stats[stat] - before)}.`);
        if (this.floorNumber === 3 && canonical === 0) {
          this.player.reputation++;
          this.message += `\nReputation +1.\n\n${this.assets.getText("textfiles/biomes/lava/library/lavalibrarydiary.txt")}`;
        }
        this.recordStatDeltas(beforeStats, beforeRep);
        return;
      }
      case Room.Encounter: {
        if (choice === 0) {
          this.player.reputation++;
          const suffix = this.floorNumber === 1 ? "prisonfree.txt" : this.floorNumber === 2 ? "butlerfree.txt" : "minionfree.txt";
          const bk = biomeKey(this.biome);
          this.outcome(`${this.assets.getText(`textfiles/biomes/${bk}/encounter/${bk}${suffix}`)}\n\nReputation +1.`);
        } else {
          this.player.npcKills++;
          const amount = this.floorNumber === 1 ? 1 : this.floorNumber === 2 ? 1.5 : 2;
          this.addAll(amount);
          const desc = this.floorNumber === 1 ? "You take the warrior's vitality." :
            this.floorNumber === 2 ? "You take the vial and leave the butler to die." : "You kill the minions.";
          this.outcome(`${desc} All stats increase by ${formatNum(amount * this.floorNumber)}, with your class bonuses.`);
        }
        this.recordStatDeltas(beforeStats, beforeRep);
        return;
      }
      case Room.Respite: {
        if (choice === 0) {
          this.addStat(Stat.Health, 10);
          if (this.floorNumber === 1) {
            this.player.reputation++;
            this.outcome(`The spring restores your health. Reputation +1.\n\n${this.assets.getText("textfiles/respitedrink.txt")}`);
          } else {
            this.outcome(`${this.floorNumber === 3 ? "You rest in the sauna." : "You drink from the respite."} Health increases by 10, with your class bonus.`);
          }
        } else {
          this.addAll(2);
          this.outcome(`You uncover supplies among the ruins. All stats increase by ${2 * this.floorNumber}, with your class bonuses.`);
        }
        this.recordStatDeltas(beforeStats, beforeRep);
        return;
      }
      case Room.Shop: {
        const canonical = this.roomChoiceOrder()[choice];
        if (canonical === 0) {
          this.addStat(Stat.Health, 5);
          this.outcome("The potion increases Health by 5, with your class bonus.");
        } else if (canonical === 1 && this.biome === Biome.Desert) {
          this.addStat(Stat.Strength, 1.5);
          this.outcome("Essence of Fortitude increases Strength by 1.5, with your class bonus.");
        } else if (canonical === 1 && this.biome === Biome.Lava) {
          this.addStat(Stat.Strength, 2);
          this.outcome("Corebound Tincture increases Strength by 2, with your class bonus.");
        } else if (canonical === 1) {
          const amount = this.floorNumber === 1 ? 1 : 2;
          this.addAll(amount);
          this.outcome(`The potion strengthens you. All stats increase by ${formatNum(amount * this.floorNumber)}, with your class bonuses.`);
        } else if (this.floorNumber === 3) {
          this.addAll(2);
          this.outcome("Inferno Draught strengthens you. All stats increase by 6, with your class bonuses.");
        } else if (this.floorNumber === 1) {
          this.player.reputation++;
          this.outcome(`${this.assets.getText("textfiles/shopkeeperbook.txt")}\n\nReputation +1.`);
        } else {
          this.addStat(Stat.Wisdom, 2);
          this.outcome("The scroll increases Wisdom by 2, with your class bonus.");
        }
        this.recordStatDeltas(beforeStats, beforeRep);
        return;
      }
      case Room.Battle:
      case Room.Boss: {
        if (choice === 1) {
          this.outcome("You slip past unnoticed. You gain no battle reward, but the riddle keeper still awaits.");
          this.recordStatDeltas(beforeStats, beforeRep);
          return;
        }
        this.player.battlesFought++;
        this.enemyHealth = this.enemyMaxHealth();
        this.strikesLeft = this.rng.nextInt(3, 5) + Math.floor(this.player.stats[Stat.Agility] / 3);
        this.bossTurn = 1;
        this.enemyShellBroken = false;
        this.enemyUndyingUsed = false;
        this.playerVenomStacks = 0;
        this.classAbilityUsed = false;
        this.lastCombatExchange = null;
        this.clearStatDeltas();
        this.phase = Phase.Combat;
        this.message = "Choose your combat tactic. Heavy Strike trades two strikes for burst power; Guard Strike dampens retaliation; your class ability can turn a tough fight once per battle.";
        return;
      }
    }
  }

  beginRiddle() {
    const r = this.riddle();
    const correctText = r.answers[r.correct];
    this.rng.shuffle(r.answers);
    r.correct = r.answers.indexOf(correctText);
    this.hiddenAnswer = null;
    if (this.player.stats[Stat.Observation] > 30) {
      const wrong = [];
      for (let i = 0; i < 3; i++) if (i !== r.correct) wrong.push(i);
      this.hiddenAnswer = wrong[this.rng.nextInt(0, 1)];
    }
    this.timeLimit = (25 - 5 * this.floorNumber) + Math.floor(this.player.stats[Stat.Wisdom] / 2);
    this.timeLeft = this.timeLimit;
    this.clearStatDeltas();
    this.phase = Phase.Riddle;
    this.message = "";
  }

  resolveRiddle(correct, timeout) {
    if (this.phase !== Phase.Riddle) return;
    const beforeStats = [...this.player.stats];
    const beforeRep = this.player.reputation;
    this.player.riddlesAnswered++;
    if (correct) {
      this.player.riddlesCorrect++;
      this.player.riddleStreak++;
      this.player.bestRiddleStreak = Math.max(this.player.bestRiddleStreak, this.player.riddleStreak);
      const streakMult = this.player.riddleStreak >= 5 ? 1.30 : (this.player.riddleStreak >= 3 ? 1.15 : 1);
      this.addAll(0.75 * streakMult);
      this.chestUnlocked = this.room() === Room.Chest;
      this.message = `Correct. All stats increase by ${formatNum(0.75 * streakMult * this.floorNumber)}, with your class bonuses.`;
      if (this.player.riddleStreak >= 3) {
        const pct = this.player.riddleStreak >= 5 ? 30 : 15;
        this.message += `\nRiddle streak x${this.player.riddleStreak} (+${pct}% bonus)!`;
      }
      if (this.chestUnlocked) this.message += "\nThe chest is unlocked. Claim it on the next screen.";
    } else {
      this.player.riddleStreak = 0;
      this.subtractAll(timeout ? 0.5 : 1.5);
      this.message = timeout
        ? `Time has run out. All stats decrease by ${formatNum(0.5 * this.floorNumber)}.`
        : `Incorrect. All stats decrease by ${formatNum(1.5 * this.floorNumber)}.`;
      this.message += `\nThe answer was: ${this.riddle().answers[this.riddle().correct]}.`;
      if (this.room() === Room.Chest) this.message += "\nThe chest remains locked.";
    }
    this.timeLeft = 0;
    this.recordStatDeltas(beforeStats, beforeRep);
    this.phase = this.player.stats[Stat.Health] <= 0 ? Phase.Defeat : Phase.RiddleOutcome;
  }

  answer(choice) {
    if (this.phase === Phase.Riddle && this.answerAvailable(choice)) {
      this.resolveRiddle(choice === this.riddle().correct, false);
    }
  }

  tick(seconds) {
    if (this.phase !== Phase.Riddle || !Number.isFinite(seconds) || seconds <= 0) return;
    this.timeLeft = Math.max(0, this.timeLeft - seconds);
    if (this.timeLeft <= 0) this.resolveRiddle(false, true);
  }

  attack(action = CombatAction.StandardStrike) {
    if (this.phase !== Phase.Combat || this.strikesLeft <= 0) return;
    if (action === CombatAction.HeavyStrike && this.strikesLeft < 2) return;
    if (action === CombatAction.ClassAbility && this.classAbilityUsed) return;

    const beforeStats = [...this.player.stats];
    const beforeRep = this.player.reputation;
    const intent = this.bossIntent();
    const strikeCost = action === CombatAction.HeavyStrike ? 2 : 1;
    this.strikesLeft -= strikeCost;

    if (action === CombatAction.ClassAbility) {
      this.classAbilityUsed = true;
      this.player.classAbilitiesUsed++;
    }

    const base = Math.max(1, this.player.stats[this.primaryCombatStat()]);
    let critical = false;
    let actionMult = 1;
    let counterMult = 1;
    let piercesWard = false;
    let bypassesThorns = false;
    let prefix = "You strike. ";

    const rollCrit = () => this.rng.nextInt(0, 34) < this.player.stats[Stat.Accuracy];

    switch (action) {
      case CombatAction.StandardStrike:
        critical = rollCrit();
        actionMult = critical ? 2 : 1;
        prefix = critical ? "Critical hit! " : "You strike. ";
        break;
      case CombatAction.HeavyStrike:
        critical = rollCrit();
        actionMult = 1.85 * (critical ? 2 : 1);
        counterMult = 1.25 * (intent.heavyBacklash ? intent.heavyCounterBonus : 1);
        piercesWard = true;
        prefix = critical ? "Devastating critical Heavy Strike! " : "Heavy Strike! ";
        break;
      case CombatAction.GuardStrike:
        critical = rollCrit();
        actionMult = 0.75 * (critical ? 2 : 1);
        counterMult = 0.25;
        bypassesThorns = true;
        prefix = critical ? "Guarded critical strike! " : "Guard Strike! ";
        break;
      case CombatAction.ClassAbility:
        piercesWard = true;
        bypassesThorns = true;
        if (this.player.hero === Hero.Warrior) {
          critical = rollCrit();
          actionMult = 1.35 * (critical ? 2 : 1);
          counterMult = 0;
          prefix = critical ? "Critical Shield Bash! Enemy staggered. " : "Shield Bash! Enemy staggered. ";
        } else if (this.player.hero === Hero.Ranger) {
          critical = true;
          actionMult = 2.5;
          counterMult = 1;
          prefix = "Precision Shot! Guaranteed critical strike! ";
        } else if (this.player.hero === Hero.Mage) {
          critical = rollCrit();
          actionMult = 1.5 * (critical ? 2 : 1);
          counterMult = 1;
          this.player.stats[Stat.Health] += 3 + this.floorNumber;
          prefix = critical ? "Critical Arcane Surge! " : "Arcane Surge! ";
        } else {
          critical = rollCrit();
          actionMult = 1.15 * (critical ? 2 : 1);
          counterMult = 0;
          this.strikesLeft++;
          prefix = critical ? "Critical Shadow Step! Evaded counter and kept strike momentum. "
                            : "Shadow Step! Evaded counter and kept strike momentum. ";
        }
        break;
    }

    const piercedByAction = (intent.heavyPierces && piercesWard) || (intent.critPierces && critical);
    const wardReduced = intent.damageTakenMultiplier < 1 && !piercedByAction;
    const wardMult = wardReduced ? intent.damageTakenMultiplier : 1;
    const damage = Math.max(1, base * actionMult * wardMult);
    this.enemyHealth = Math.max(0, this.enemyHealth - damage);
    this.message = `${prefix}${formatNum(damage)} damage.`;

    const result = {
      valid: true,
      action,
      critical,
      guarded: action === CombatAction.GuardStrike || (action === CombatAction.ClassAbility && (this.player.hero === Hero.Warrior || this.player.hero === Hero.Rogue)),
      strikesSpent: strikeCost,
      damageDealt: damage,
      counterattacked: false,
      counterDamage: 0,
      enemyHealed: 0,
      strikesDrained: 0,
      shellShattered: false,
      undyingTriggered: false,
      venomDamage: 0,
      venomStacksApplied: 0,
      enemyDefeated: false,
      playerDefeated: false,
      retreated: false
    };

    if (wardReduced) {
      this.message += ` (${intent.name} dampens the blow!)`;
    } else if (intent.shattersOnPierce && piercedByAction && !this.enemyShellBroken) {
      this.enemyShellBroken = true;
      result.shellShattered = true;
      this.message += " (Glacial Shell shattered!)";
    } else if (intent.damageTakenMultiplier < 1 && piercedByAction) {
      this.message += ` (Pierced ${intent.name}!)`;
    }

    if (this.enemyHealth <= 0 && intent.undyingActive && !this.enemyUndyingUsed && !piercesWard && !critical) {
      this.enemyHealth = 1;
      this.enemyUndyingUsed = true;
      result.undyingTriggered = true;
      this.message += "\nRestless Zombie clings to unlife at 1 HP!";
    }

    if (this.enemyHealth <= 0) {
      this.player.battlesWon++;
      this.player.strikesLeftover += Math.max(0, this.strikesLeft);
      result.enemyDefeated = true;
      this.lastCombatExchange = result;
      if (this.room() === Room.Boss && this.floorNumber === 3) {
        this.recordStatDeltas(beforeStats, beforeRep);
        this.finish(true);
        return;
      }
      this.addAll(1);
      this.message += `\n${this.enemyName()} defeated. All stats increase by ${this.floorNumber}, with your class bonuses.`;
      if (this.room() === Room.Boss && this.floorNumber === 2) {
        this.player.reputation++;
        const bk = biomeKey(this.biome);
        this.message += `\nReputation +1.\n\n${this.assets.getText(`textfiles/biomes/${bk}/battle/${bk}bosswin.txt`)}`;
      }
      this.recordStatDeltas(beforeStats, beforeRep);
      this.phase = this.room() === Room.Boss ? Phase.Complete : Phase.Outcome;
      return;
    }

    const bossCounterMult = (intent.guardBypasses && bypassesThorns) ? 1 : intent.counterMultiplier;
    const retaliation = this.enemyDamage() * counterMult * bossCounterMult;
    if (retaliation > 0) {
      this.player.stats[Stat.Health] = Math.max(0, this.player.stats[Stat.Health] - retaliation);
      result.counterattacked = true;
      result.counterDamage = retaliation;
      if (action === CombatAction.GuardStrike) {
        this.message += `\nGuard reduces the counterattack to ${formatNum(retaliation)} health.`;
      } else if (action === CombatAction.HeavyStrike && intent.heavyBacklash) {
        this.message += `\nSearing Scales erupt for ${formatNum(retaliation)} backlash damage!`;
      } else if (intent.telegraphed && bossCounterMult > 1) {
        this.message += `\n${intent.name} strikes back for ${formatNum(retaliation)} health!`;
      } else {
        this.message += `\nThe enemy counters for ${formatNum(retaliation)} health.`;
      }
      if (intent.lifestealRatio > 0) {
        const heal = Math.min(this.enemyMaxHealth() - this.enemyHealth, retaliation * intent.lifestealRatio);
        if (heal > 0) {
          this.enemyHealth += heal;
          result.enemyHealed += heal;
          this.message += `\n${intent.name} restores ${formatNum(heal)} enemy HP!`;
        }
      }
      if (intent.drainsStrike && !result.guarded && this.strikesLeft > 0) {
        this.strikesLeft--;
        result.strikesDrained = 1;
        this.message += `\n${intent.name} drains 1 strike!`;
      }
      if (intent.appliesVenom && !result.guarded && this.playerVenomStacks < 3) {
        this.playerVenomStacks++;
        result.venomStacksApplied = 1;
        this.message += `\nVenomous Stinger injects Venom (x${this.playerVenomStacks})!`;
      }
    } else {
      this.message += "\nThe enemy's counterattack is negated.";
    }

    if (this.playerVenomStacks > 0) {
      const venomDmg = this.enemyDamage() * 0.20 * this.playerVenomStacks;
      this.player.stats[Stat.Health] = Math.max(0, this.player.stats[Stat.Health] - venomDmg);
      result.venomDamage = venomDmg;
      this.message += `\nVenom (x${this.playerVenomStacks}) deals ${formatNum(venomDmg)} damage!`;
    }

    if (intent.regenRatio > 0) {
      const heal = Math.min(this.enemyMaxHealth() - this.enemyHealth, this.enemyMaxHealth() * intent.regenRatio);
      if (heal > 0) {
        this.enemyHealth += heal;
        result.enemyHealed += heal;
        this.message += `\n${intent.name} restores ${formatNum(heal)} enemy HP!`;
      }
    }

    this.bossTurn++;

    if (this.player.stats[Stat.Health] <= 0) {
      result.playerDefeated = true;
      this.lastCombatExchange = result;
      this.recordStatDeltas(beforeStats, beforeRep);
      if (this.room() === Room.Boss && this.floorNumber === 3) {
        this.finish(false);
        return;
      }
      this.phase = Phase.Defeat;
      this.message += "\nYour strength gives out. Your descent has ended.";
    } else if (this.strikesLeft === 0) {
      if (this.room() === Room.Boss) {
        result.playerDefeated = true;
        this.lastCombatExchange = result;
        this.recordStatDeltas(beforeStats, beforeRep);
        if (this.floorNumber === 3) {
          this.finish(false);
          return;
        }
        this.phase = Phase.Defeat;
        this.message += "\nYour strikes are exhausted. The guardian still blocks the stairway.";
      } else {
        result.retreated = true;
        this.lastCombatExchange = result;
        this.subtractAll(0.75);
        this.recordStatDeltas(beforeStats, beforeRep);
        this.outcome(`${this.message}\nYou retreat. All stats decrease by ${formatNum(0.75 * this.floorNumber)}. The riddle keeper awaits.`);
      }
    } else {
      this.lastCombatExchange = result;
    }
  }

  advance() {
    if (this.phase === Phase.Outcome) {
      this.beginRiddle();
      return;
    }
    if (this.phase === Phase.RiddleOutcome) {
      if (this.chestUnlocked) {
        this.clearStatDeltas();
        this.phase = Phase.Loot;
        return;
      }
    } else if (this.phase === Phase.Loot) {
      const beforeStats = [...this.player.stats];
      const beforeRep = this.player.reputation;
      this.addAll(2);
      this.chestUnlocked = false;
      this.message = `You claim the chest's treasure. All stats increase by ${2 * this.floorNumber}, with your class bonuses.`;
      if (this.floorNumber === 2 && this.roomNumber() === 5) {
        this.player.reputation++;
        this.message += `\nReputation +1.\n\n${this.assets.getText("textfiles/floor2chestopen.txt")}`;
      }
      if (this.floorNumber === 3 && this.roomNumber() <= 3) {
        this.player.reputation++;
        this.message += `\nReputation +1.\n\n${this.assets.getText("textfiles/biomes/lava/chest/lavachestopen.txt")}`;
      }
      this.recordStatDeltas(beforeStats, beforeRep);
      this.phase = Phase.RiddleOutcome;
      return;
    } else {
      return;
    }
    if (this.roomIndex + 1 < this.rooms.length) {
      this.roomIndex++;
      this.enterRoom();
    }
  }

  finish(malvelDefeated) {
    this.ending = resolveEnding(malvelDefeated, this.player);
    this.phase = Phase.Ending;
    this.message = this.assets.getText(this.storyFile());
  }

  scoreBreakdown(playTimeSeconds = -1) {
    let nonHealthStats = 0;
    for (let i = 0; i < 6; i++) {
      if (i === Stat.Health) continue;
      nonHealthStats += Math.max(0, Math.trunc(this.player.stats[i] || 0));
    }
    const statsPoints = nonHealthStats * 5;
    const healthPoints = Math.max(0, Math.trunc(this.player.stats[Stat.Health] || 0)) * 10;
    const strikesPoints = Math.max(0, Math.trunc(this.player.strikesLeftover || 0)) * 10;
    const victorious = this.ending !== Ending.None && this.ending !== Ending.Bad;
    const timePoints = (victorious && Number.isFinite(playTimeSeconds) && playTimeSeconds >= 0)
      ? Math.max(0, 1800 - Math.trunc(playTimeSeconds)) * 2
      : 0;
    return {
      statsPoints,
      healthPoints,
      strikesPoints,
      timePoints,
      total: statsPoints + healthPoints + strikesPoints + timePoints
    };
  }

  score(playTimeSeconds = -1) {
    return this.scoreBreakdown(playTimeSeconds).total;
  }

  musicFile() {
    if (this.phase === Phase.Ending && this.ending !== Ending.None) return endingMusicFile(this.ending);
    return `${biomeKey(this.biome)}music.ogg`;
  }

  mapImage() {
    return `images/map/${this.floorNumber}/floor${this.floorNumber}_map.jpg`;
  }

  enemyName() {
    if (this.room() === Room.Boss) {
      return ["The Yeti", "The Jungle Warden", "Ahmose the Pharaoh", "Count Dracula", "Malvel"][this.biome];
    }
    if (this.biome === Biome.Lava) return this.roomNumber() === 5 ? "The Lava Knight" : "Lava Mini Dragon";
    if (this.biome === Biome.Desert) return this.roomNumber() === 8 ? "Giant Scorpion" : "The Sandwyrm";
    if (this.biome === Biome.Ghost) return this.roomNumber() === 8 ? "The Werewolf" : "Restless Zombie";
    if (this.biome === Biome.Ice) return this.roomNumber() === 5 ? "Ice Golem" : "Glacier Troll";
    return this.roomNumber() === 5 ? "Jungle Golem" : "Jungle Troll";
  }

  roomTitle(type = this.room()) {
    if (this.floorNumber === 3 && type === Room.Encounter) return "Malvel's minions";
    if (this.floorNumber === 3 && type === Room.Shop) return "The merchant";
    if (this.floorNumber === 2 && type === Room.Encounter) return "The wounded butler";
    if (this.floorNumber === 2 && type === Room.Shop) return "The merchant";
    return roomName(type);
  }

  title() {
    return this.room() === Room.Boss ? this.enemyName() : this.roomTitle();
  }

  image() {
    if (this.phase === Phase.Ending) return endingImage(this.ending);
    const key = biomeKey(this.biome);
    if (this.room() === Room.Boss) return bossImage(this.biome);
    if (this.floorNumber === 3) {
      const map = {
        [Room.Library]: "library/lavalibrary.jpg",
        [Room.Chest]: "chest/lavachest.jpg",
        [Room.Battle]: this.roomNumber() === 5 ? "battle/lavabattle2.jpg" : "battle/lavabattle1.jpg",
        [Room.Encounter]: "encounter/lavaminions.jpg",
        [Room.Respite]: "respite/lavarespite.jpg",
        [Room.Shop]: "shop/lavashop.jpg"
      };
      return `images/biomes/lava/${map[this.room()]}`;
    }
    if (this.floorNumber === 2) {
      const map = {
        [Room.Library]: `library/${key}library.jpeg`,
        [Room.Chest]: `chest/${key}chest.jpeg`,
        [Room.Battle]: `battle/${key}${this.biome === Biome.Ghost ? (this.roomNumber() === 8 ? "battle2.jpeg" : "battle1.jpeg") : (this.roomNumber() === 8 ? "battle2.jpg" : "battle.jpg")}`,
        [Room.Encounter]: `encounter/${key}butler.jpeg`,
        [Room.Respite]: `respite/${key}respite.jpeg`,
        [Room.Shop]: `shop/${key}shop.jpeg`
      };
      return `images/biomes/${key}/${map[this.room()]}`;
    }
    if (this.room() === Room.Encounter) {
      return `images/${this.biome === Biome.Ice ? "biomes/ice/encounter/iceencounter.jpg" : "prison.jpeg"}`;
    }
    const map = {
      [Room.Library]: `library/${key}library.jpg`,
      [Room.Chest]: `chest/${key}chest.jpg`,
      [Room.Battle]: `battle/${key}${this.roomNumber() === 5 ? "battle2.jpg" : "battle.jpg"}`,
      [Room.Boss]: `battle/${key}boss.jpg`,
      [Room.Respite]: `respite/${key}respite.jpg`,
      [Room.Shop]: `shop/${key}shop.jpg`
    };
    return `images/biomes/${key}/${map[this.room()]}`;
  }

  storyFile() {
    if (this.phase === Phase.Ending) return endingStoryFile(this.ending);
    const key = biomeKey(this.biome);
    if (this.room() === Room.Shop) {
      return this.floorNumber === 1 ? "textfiles/shopkeeper.txt" : `textfiles/biomes/${key}/shop/${key}shop.txt`;
    }
    let suffix = "";
    switch (this.room()) {
      case Room.Library: suffix = `library/${key}library.txt`; break;
      case Room.Chest: suffix = `chest/${key}chest.txt`; break;
      case Room.Battle:
        if (this.biome === Biome.Desert) suffix = `battle/${key}${this.roomNumber() === 8 ? "battle.txt" : "battle2.txt"}`;
        else suffix = `battle/${key}${this.roomNumber() === (this.floorNumber === 2 ? 8 : 5) ? "battle2.txt" : "battle.txt"}`;
        break;
      case Room.Boss: suffix = `battle/${key}boss.txt`; break;
      case Room.Encounter:
        suffix = `encounter/${key}${this.floorNumber === 1 ? "prison.txt" : this.floorNumber === 2 ? "butler.txt" : "minion.txt"}`;
        break;
      case Room.Respite: suffix = `respite/${key}respite.txt`; break;
    }
    return `textfiles/biomes/${key}/${suffix}`;
  }

  serializeState() {
    return {
      player: { ...this.player, stats: [...this.player.stats] },
      difficulty: this.difficulty,
      biome: this.biome,
      firstBiome: this.firstBiome,
      secondBiome: this.secondBiome,
      floorNumber: this.floorNumber,
      phase: this.phase,
      ending: this.ending,
      started: this.started,
      seed: this.seed,
      rooms: [...this.rooms],
      roomIndex: this.roomIndex,
      riddleOrder: [...this.riddleOrder],
      activeRiddle: this.riddles[this.riddleOrder[this.roomIndex]] ? {
        answers: [...this.riddles[this.riddleOrder[this.roomIndex]].answers],
        correct: this.riddles[this.riddleOrder[this.roomIndex]].correct
      } : null,
      hiddenAnswer: this.hiddenAnswer,
      rng: this.rng.serialize(),
      chestUnlocked: this.chestUnlocked,
      classAbilityUsed: this.classAbilityUsed,
      timeLeft: this.timeLeft,
      timeLimit: this.timeLimit,
      enemyHealth: this.enemyHealth,
      strikesLeft: this.strikesLeft,
      bossTurn: this.bossTurn,
      enemyShellBroken: this.enemyShellBroken,
      enemyUndyingUsed: this.enemyUndyingUsed,
      playerVenomStacks: this.playerVenomStacks,
      message: this.message
    };
  }

  deserializeState(state) {
    this.player = { ...makePlayer(state.player.hero), ...state.player, stats: [...state.player.stats] };
    this.difficulty = state.difficulty;
    this.biome = state.biome;
    this.firstBiome = state.firstBiome;
    this.secondBiome = state.secondBiome ?? (state.floorNumber === 2 ? state.biome : null);
    this.floorNumber = state.floorNumber;
    this.phase = state.phase;
    this.ending = state.ending;
    this.started = Boolean(state.started);
    this.seed = state.seed >>> 0;
    this.runSerial++;
    this.rooms = [...state.rooms];
    this.roomIndex = state.roomIndex;
    const tier = this.assets.sourceRiddles[this.floorNumber - 1];
    this.riddles = tier.map((r) => ({
      question: r.question,
      answers: [...r.answers],
      correct: r.correct,
      requiredBiome: r.requiredBiome ?? null
    }));
    this.riddleOrder = [...state.riddleOrder];
    if (state.activeRiddle && this.riddles[this.riddleOrder[this.roomIndex]]) {
      this.riddles[this.riddleOrder[this.roomIndex]].answers = [...state.activeRiddle.answers];
      this.riddles[this.riddleOrder[this.roomIndex]].correct = state.activeRiddle.correct;
    }
    this.hiddenAnswer = state.hiddenAnswer ?? null;
    this.rng = MT19937.deserialize(state.rng);
    this.chestUnlocked = Boolean(state.chestUnlocked);
    this.classAbilityUsed = Boolean(state.classAbilityUsed);
    this.timeLeft = Number(state.timeLeft) || 0;
    this.timeLimit = Number(state.timeLimit) || 0;
    this.enemyHealth = Number(state.enemyHealth) || 0;
    this.strikesLeft = Number(state.strikesLeft) || 0;
    this.bossTurn = Number(state.bossTurn) || 1;
    this.enemyShellBroken = Boolean(state.enemyShellBroken);
    this.enemyUndyingUsed = Boolean(state.enemyUndyingUsed);
    this.playerVenomStacks = Math.max(0, Math.min(3, Math.trunc(Number(state.playerVenomStacks) || 0)));
    this.message = String(state.message || "");
    this.clearStatDeltas();
    this.lastCombatExchange = null;
  }
}
