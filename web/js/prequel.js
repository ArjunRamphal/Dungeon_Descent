// Dungeon Descent: Ashes of Falkreath — 3-Act Unlockable Prequel Campaign Engine
// Exact Web Edition port of src/DungeonDescent/sfml/PrequelRun.h & PrequelRun.cpp

import { CombatAction, formatNum } from "./engine.js";

export const PrequelAct = Object.freeze({
  Act1_Eldred: 0,
  Act2_Malvel: 1,
  Act3_Sophie: 2
});

export const PrequelPhase = Object.freeze({
  ActIntro: "act_intro",
  Scene: "scene",
  Parley: "parley",
  Combat: "combat",
  Outcome: "outcome",
  ActComplete: "act_complete",
  Defeat: "defeat",
  Epilogue: "epilogue"
});

export const PrequelCoda = Object.freeze({
  None: 0,
  HollowCrown: 1,
  LastDoubt: 2
});

export const PREQUEL_ART_FILES = Object.freeze([
  "images/prequel/eldred_pfp.jpg",
  "images/prequel/malvel_pfp.jpg",
  "images/prequel/sophie_pfp.jpg",
  "images/prequel/falkreath_river.jpg",
  "images/prequel/falkreath_field.jpg",
  "images/prequel/iron_captain.jpg",
  "images/prequel/library_of_wan.jpg",
  "images/prequel/archmage_solon.jpg",
  "images/prequel/sir_henry.jpg",
  "images/prequel/obsidian_forge.jpg",
  "images/prequel/candlelit_throne.jpg",
  "images/prequel/pedestal_chamber.jpg",
  "images/prequel/royal_captain.jpg"
]);

export const PREQUEL_MUSIC_FILES = Object.freeze([
  "prequel_act1_falkreath.ogg",
  "prequel_act1_battle.ogg",
  "prequel_act2_ashes.ogg",
  "prequel_act2_battle.ogg",
  "prequel_act3_court.ogg",
  "prequel_act3_battle.ogg",
  "prequel_coda_hollow_crown.ogg",
  "prequel_coda_last_doubt.ogg",
  "prequel_defeat.ogg"
]);

export function prequelActTitle(act) {
  switch (act) {
    case PrequelAct.Act1_Eldred: return "ACT I: THE RIVER & THE LEVY";
    case PrequelAct.Act2_Malvel: return "ACT II: THE HIDDEN HEIR";
    case PrequelAct.Act3_Sophie: return "ACT III: THE SILK SMILE";
    default: return "ASHES OF FALKREATH";
  }
}

export function prequelCodaTitle(coda) {
  switch (coda) {
    case PrequelCoda.HollowCrown: return "PRELUDE CODA: THE HOLLOW CROWN";
    case PrequelCoda.LastDoubt: return "PRELUDE CODA: THE LAST DOUBT";
    default: return "ASHES OF FALKREATH";
  }
}

export function prequelActAccent(act) {
  switch (act) {
    case PrequelAct.Act1_Eldred: return "#d9a252";
    case PrequelAct.Act2_Malvel: return "#e06c4d";
    case PrequelAct.Act3_Sophie: return "#c99ee8";
    default: return "#d9a252";
  }
}

function defaultCharacter() {
  return {
    act: PrequelAct.Act1_Eldred,
    name: "",
    title: "",
    weapon: "",
    portrait: "",
    abilityName: "",
    abilityDesc: "",
    power: 8,
    resolve: 6,
    cunning: 4,
    agility: 4,
    hp: 42,
    maxHp: 42
  };
}

function defaultEnemy() {
  return {
    name: "",
    title: "",
    image: "",
    hp: 0,
    maxHp: 0,
    counterDamage: 0,
    strikesLeft: 0,
    maxStrikes: 0,
    turn: 0,
    boss: false,
    abilityUsedThisBattle: false,
    rallyGuardActive: false,
    intent: {
      active: false,
      telegraphed: false,
      name: "",
      detail: "",
      counterMultiplier: 1,
      damageTakenMultiplier: 1
    }
  };
}

function defaultParley() {
  return {
    speaker: "",
    prompt: "",
    options: ["", "", ""],
    bestIndex: 0,
    hiddenIndex: 3,
    timeLimit: 18,
    timeLeft: 18,
    successText: "",
    failText: ""
  };
}

function defaultExchange() {
  return {
    valid: false,
    action: CombatAction.StandardStrike,
    strikesSpent: 1,
    damageDealt: 0,
    counterDamage: 0,
    critical: false,
    guarded: false,
    counterattacked: false,
    enemyDefeated: false,
    playerDefeated: false
  };
}

export class PrequelRun {
  constructor() {
    this.act = PrequelAct.Act1_Eldred;
    this.chapterIndex = 0; // 0..14
    this.phase = PrequelPhase.ActIntro;
    this.coda = PrequelCoda.None;

    this.character = defaultCharacter();
    this.enemy = defaultEnemy();
    this.parley = defaultParley();
    this.choices = [];
    this.lastExchange = defaultExchange();

    this.heading = "";
    this.subheading = "";
    this.body = "";
    this.sceneImage = "images/prequel/falkreath_river.jpg";

    this.pendingVictoryText = "";
    this.pendingNextChapter = 0;
    this.pendingAfterOutcomeCombat = false;
    this.pendingAfterOutcomeParley = false;
    this.queuedParley = null;
    this.queuedEnemy = null;
    this.queuedVictoryText = "";
    this.queuedNextChapter = 0;

    // Cross-Act Chronicle Threads
    this.legacy = 0;
    this.fury = 0;
    this.remorse = 0;

    // Snapshot at Act start for clean Act retry
    this.actStartLegacy = 0;
    this.actStartFury = 0;
    this.actStartRemorse = 0;
    this.actStartAbilities = 0;
    this.actStartBattles = 0;
    this.actStartParleys = 0;

    this.abilitiesUsed = 0;
    this.battlesWon = 0;
    this.parleysWon = 0;
    this.serial = 0;
    this.rngState = 0x5EED1337 >>> 0;
  }

  nextUnitFloat() {
    let x = this.rngState >>> 0;
    x = (x ^ ((x << 13) >>> 0)) >>> 0;
    x = (x ^ (x >>> 17)) >>> 0;
    x = (x ^ ((x << 5) >>> 0)) >>> 0;
    this.rngState = x >>> 0;
    return (x & 0xFFFF) / 65536;
  }

  actNumber() {
    return this.act + 1;
  }

  chapterNumber() {
    return this.chapterIndex + 1; // 1..15
  }

  actChapterNumber() {
    return (this.chapterIndex % 5) + 1; // 1..5
  }

  ironChronicleEligible() {
    return this.abilitiesUsed === 0;
  }

  musicFile() {
    if (this.phase === PrequelPhase.Epilogue) {
      return this.coda === PrequelCoda.HollowCrown
        ? "prequel_coda_hollow_crown.ogg"
        : "prequel_coda_last_doubt.ogg";
    }
    if (this.phase === PrequelPhase.Defeat) return "prequel_defeat.ogg";
    switch (this.act) {
      case PrequelAct.Act1_Eldred:
        return this.phase === PrequelPhase.Combat ? "prequel_act1_battle.ogg" : "prequel_act1_falkreath.ogg";
      case PrequelAct.Act2_Malvel:
        return this.phase === PrequelPhase.Combat ? "prequel_act2_battle.ogg" : "prequel_act2_ashes.ogg";
      case PrequelAct.Act3_Sophie:
        return this.phase === PrequelPhase.Combat ? "prequel_act3_battle.ogg" : "prequel_act3_court.ogg";
      default:
        return "prequel_act1_falkreath.ogg";
    }
  }

  start() {
    this.serial++;
    this.rngState = (0x5EED1337 ^ Math.imul(this.serial, 2654435761)) >>> 0;
    if (this.rngState === 0) this.rngState = 0x5EED1337;
    this.legacy = 0;
    this.fury = 0;
    this.remorse = 0;
    this.abilitiesUsed = 0;
    this.battlesWon = 0;
    this.parleysWon = 0;
    this.coda = PrequelCoda.None;
    this.pendingAfterOutcomeCombat = false;
    this.pendingAfterOutcomeParley = false;
    this.setupAct(PrequelAct.Act1_Eldred);
  }

  restartAct() {
    this.legacy = this.actStartLegacy;
    this.fury = this.actStartFury;
    this.remorse = this.actStartRemorse;
    this.abilitiesUsed = this.actStartAbilities;
    this.battlesWon = this.actStartBattles;
    this.parleysWon = this.actStartParleys;
    this.pendingAfterOutcomeCombat = false;
    this.pendingAfterOutcomeParley = false;
    this.setupAct(this.act);
  }

  setupAct(act) {
    this.act = act;
    this.actStartLegacy = this.legacy;
    this.actStartFury = this.fury;
    this.actStartRemorse = this.remorse;
    this.actStartAbilities = this.abilitiesUsed;
    this.actStartBattles = this.battlesWon;
    this.actStartParleys = this.parleysWon;
    this.chapterIndex = act * 5;
    this.phase = PrequelPhase.ActIntro;
    this.choices = [];
    this.lastExchange = defaultExchange();

    if (act === PrequelAct.Act1_Eldred) {
      this.character = {
        act,
        name: "Lord Eldred",
        title: "Lord of House Falkreath",
        weapon: "Falkreath Greatsword",
        portrait: "images/prequel/eldred_pfp.jpg",
        abilityName: "Rally the Banner",
        abilityDesc: "Once per battle: 1.4x blow, halves counter damage, and restores 4 HP.",
        power: 10,
        resolve: 6,
        cunning: 5,
        agility: 6,
        maxHp: 48,
        hp: 48
      };
      this.heading = prequelActTitle(act);
      this.subheading = "Years Before the Descent — The Riverlands of House Falkreath";
      this.sceneImage = "images/prequel/falkreath_river.jpg";
      this.body =
        "Before Malvel sat upon his obsidian throne, before a gilded cage hung over the magma " +
        "of Mount Aethor, House Falkreath was the green breadbasket of the realm.\n\n" +
        "Its fields fed the capital. Its looms clothed the King's court. And its lord, Eldred, " +
        "bore two burdens no chronicle in the palace dared print: a starving province pushed to " +
        "the brink by Aethoria's crown — and a secret love for the King's own daughter, Sophie.\n\n" +
        "You take up Falkreath's banner as the King's final levy rides across the river bridge.";
    } else if (act === PrequelAct.Act2_Malvel) {
      this.character = {
        act,
        name: "Malvel",
        title: "Hidden Heir of Falkreath",
        weapon: "Obsidian Flame",
        portrait: "images/prequel/malvel_pfp.jpg",
        abilityName: "Molten Ward",
        abilityDesc: "Once per battle: 1.55x ward-piercing surge that negates counterattack.",
        power: 11.5 + Math.max(0, this.legacy) * 0.5,
        resolve: 7,
        cunning: 7,
        agility: 6,
        maxHp: 52,
        hp: 52
      };
      this.heading = prequelActTitle(act);
      this.subheading = "After the Fall — Ruins of Falkreath & The Ancient Library of Wan";
      this.sceneImage = "images/prequel/falkreath_field.jpg";
      this.body =
        "Aethoria celebrated the end of House Falkreath as a clean victory over traitors. " +
        "The King never knew Eldred had a hidden son.\n\n" +
        "While the smoke still clung to the wheat fields, Malvel walked through the ashes and " +
        "picked up the broken crest of his house. The Luminari mages had called his molten arts " +
        "forbidden; the King's knights had called his father's defense treason.\n\n" +
        "The war ended for Aethoria. For Falkreath's last heir, it is only beginning.";
      if (this.legacy >= 2) {
        this.body +=
          "\n\n[Falkreath's Legacy]: Because Eldred safeguarded his people's forge and loyalty, " +
          "Falkreath's survivors smuggled tempered obsidian into your hands (+Power).";
      }
    } else {
      this.character = {
        act,
        name: "Princess Sophie",
        title: "Architect of the Trap",
        weapon: "Royal Stiletto & Seal",
        portrait: "images/prequel/sophie_pfp.jpg",
        abilityName: "Sovereign Command",
        abilityDesc: "Once per battle: 1.45x precision strike that staggers enemy and heals 5 HP.",
        power: 11 + Math.max(0, this.fury) * 0.35,
        resolve: 8,
        cunning: 9,
        agility: 9,
        maxHp: 50,
        hp: 50
      };
      this.heading = prequelActTitle(act);
      this.subheading = "The Eve of Dungeon Descent — Palace of Aethoria & The Obsidian Sanctum";
      this.sceneImage = "images/prequel/candlelit_throne.jpg";
      this.body =
        "The banners of victory hung across the capital for seven days. Princess Sophie smiled " +
        "beside her father on the balcony, applauded the knights who returned from Falkreath, " +
        "and wore silk while her heart turned to ash.\n\n" +
        "In the quiet hours after midnight, she stopped weeping and began planning. Malvel " +
        "had the fire beneath the mountain. She had the keys to the kingdom — and the face " +
        "every hero in Aethoria would march into hell to save.";
    }
  }

  loadChapter(index) {
    this.chapterIndex = index;
    this.phase = PrequelPhase.Scene;
    this.choices = [];
    this.pendingAfterOutcomeCombat = false;
    this.pendingAfterOutcomeParley = false;

    switch (index) {
      // ACT I — ELDRED (Chapters 0..4)
      case 0:
        this.heading = "Chapter 1: The King's Levy";
        this.subheading = "Falkreath Keep — Tax Collectors of Aethoria";
        this.sceneImage = "images/prequel/falkreath_river.jpg";
        this.body =
          "Royal riders bearing Aethoria's golden seal halt inside Falkreath's courtyard. " +
          "Their parchment demands half the autumn harvest, every bolt of wool, and two hundred " +
          "young farmers to march in a border war Falkreath never voted for.\n\n" +
          "The shopkeeper's chronicle will one day record this afternoon as the final straw. " +
          "Your villagers watch from the stone steps, waiting for your answer.";
        this.choices = [
          { label: "Hide half the grain for your villages and parley to stall the royal envoy" },
          { label: "Pay the silver from your own vault so no Falkreath farmer is conscripted" },
          { label: "Tear up the conscription writ before the envoy's eyes — Falkreath bends no further" }
        ];
        break;

      case 1:
        this.heading = "Chapter 2: Moonlight on the River";
        this.subheading = "The Old Stone Well — Forbidden Meeting";
        this.sceneImage = "images/prequel/falkreath_river.jpg";
        this.body =
          "After midnight, willow branches brush the silver water beside Falkreath's stone well. " +
          "Princess Sophie slips off her dark riding hood, her hands cold from the river mist.\n\n" +
          "\"My father's council already speaks of marching on your walls, Eldred,\" she whispers. " +
          "Then a twig snaps in the reeds. Stepping into the lantern light with wide, terrified eyes " +
          "is the royal palace butler — he followed Sophie from the capital and heard every word.";
        this.choices = [
          { label: "Appeal to the butler's conscience and swear him to silence for Sophie's sake" },
          { label: "Warn Sophie that her father's spies already know, and press Falkreath's signet into her palm" },
          { label: "Confine the butler quietly until dawn so no word reaches the King tonight" }
        ];
        break;

      case 2:
        this.heading = "Chapter 3: The Hidden Son";
        this.subheading = "Beneath the Mountain Cavern — Young Malvel";
        this.sceneImage = "images/prequel/obsidian_forge.jpg";
        this.body =
          "In a sulfur-scented cavern beneath the northern ridge, your secret son Malvel stands " +
          "before veins of glowing magma, obsidian runes humming around his palms. No lord in " +
          "Aethoria knows he exists — you kept his name off every royal ledger to protect him.\n\n" +
          "\"The Luminari call this heresy,\" Malvel says, his eyes reflecting orange fire. " +
          "\"Yet when Aethoria's spears reach our river, honor won't stop them. Fire will.\"";
        this.choices = [
          { label: "Urge restraint: \"Protect Falkreath's living people, Malvel — never become what we hate.\"" },
          { label: "Entrust him with Falkreath's hidden subterranean maps in case the keep falls" },
          { label: "Embrace his strength: \"If the King brings ruin to our gate, answer him in kind.\"" }
        ];
        break;

      case 3:
        this.heading = "Chapter 4: The Northern Patrol";
        this.subheading = "Border Woods — Encounter with Halt the Ranger";
        this.sceneImage = "images/prequel/falkreath_field.jpg";
        this.body =
          "Scouting the northern pines before dawn, you stumble upon a lone Aethorian ranger " +
          "pinned behind fallen timber by masked royal assassins. His longbow never wavers.\n\n" +
          "\"Name's Halt,\" the ranger mutters, tossing you a blood-stained dispatch bearing the " +
          "seals of corrupt palace nobles. \"They forged treason letters to justify burning Falkreath " +
          "— and now they're framing me to bury the evidence. Watch the ridge. More riders incoming!\"";
        this.choices = [
          { label: "Stand shoulder-to-shoulder with Halt and break the royal vanguard scouts in battle" },
          { label: "Cover Halt's escape north with the forged dispatch before charging the scouts alone" }
        ];
        break;

      case 4:
        this.heading = "Chapter 5: The Field of Falkreath";
        this.subheading = "Act I Finale — The King's Vanguard";
        this.sceneImage = "images/prequel/iron_captain.jpg";
        this.body =
          "By dusk, the wheat fields of Falkreath burn crimson. The King's army has broken the outer " +
          "bridge — and through the smoke marches the Iron Captain, Aethoria's deadliest war-leader, " +
          "with royal executioners at his back.\n\n" +
          "Behind you, wagons carrying Falkreath's families and young Malvel race for the mountain pass. " +
          "You plant your boots in the scorched mud and raise Falkreath's greatsword. " +
          "You only need to hold the line long enough for them to disappear into the dark.";
        this.choices = [
          { label: "Face the Iron Captain in single combat to hold the pass" }
        ];
        break;

      // ACT II — MALVEL (Chapters 5..9)
      case 5:
        this.heading = "Chapter 6: Oath in the Ashes";
        this.subheading = "The Ruined Keep of House Falkreath";
        this.sceneImage = "images/prequel/falkreath_field.jpg";
        this.body =
          "Rain hisses across blackened timber where Falkreath's banners once stood. " +
          "You kneel in the mud where your father fell and pry Eldred's cracked signet from the ash.\n\n" +
          "Aethoria's heralds already sing of a glorious conquest. None of them know that " +
          "Falkreath's last heir survived — or that the Ancient Library of Wan holds the " +
          "sealed manuscript of the Obsidian Focusing Seal.";
        this.choices = [
          { label: "Channel pure obsidian wrath — swear to reduce the King's throne to cinders" },
          { label: "Swear a calculated oath: patience, stealth, and precision will bring down Aethoria" },
          { label: "Gather Falkreath's surviving smiths to forge your dungeon sanctum first" }
        ];
        break;

      case 6:
        this.heading = "Chapter 7: Raid on the Library of Wan";
        this.subheading = "Sanctuary of the Luminari";
        this.sceneImage = "images/prequel/library_of_wan.jpg";
        this.body =
          "Golden rune circles hum beneath the soaring vaults of the Ancient Library of Wan. " +
          "Upon the central lectern lies the black-bound Codex of the Obsidian Seal. " +
          "In the shadows of the gallery, a young Luminari scholar named Elian freezes in horror as " +
          "your purple cloak steps through the shattered ward.\n\n" +
          "From the inner sanctum rises Arch-Mage Solon, staff blazing with ivory light: " +
          "\"You dare bring forbidden magma into the halls of the Luminari, son of Falkreath?\"";
        this.choices = [
          { label: "Disrupt the Luminari ward circle with a swift arcane counter-argument before dueling Solon" },
          { label: "Shatter the outer ward by raw obsidian fire and confront Arch-Mage Solon head-on" }
        ];
        break;

      case 7:
        this.heading = "Chapter 8: Sir Henry's Stand";
        this.subheading = "The Snow-Dusted Mountain Pass";
        this.sceneImage = "images/prequel/sir_henry.jpg";
        this.body =
          "With the stolen codex tucked inside your robes, snow lashes the narrow stone bridge " +
          "leading back toward Mount Aethor. Blocking the archway stands a silver-haired veteran " +
          "of the King's Guard, blue lion shield raised steady against the storm.\n\n" +
          "\"I am Sir Henry,\" the knight calls out over the wind. \"My squire Cassio rides an hour " +
          "behind me. Turn back now, sorcerer — no dark tome passes this bridge while I draw breath.\"";
        this.choices = [
          { label: "Acknowledge the knight's honor, then summon your obsidian shade to break his defense" },
          { label: "Mock the King's Guard for serving a tyrant and strike without mercy" }
        ];
        break;

      case 8:
        this.heading = "Chapter 9: The Hall of Chains";
        this.subheading = "Hunting Eldred's Killer";
        this.sceneImage = "images/prequel/iron_captain.jpg";
        this.body =
          "Three weeks later, in a rain-swept border garrison, you corner the man who drove his " +
          "steel through Eldred's throat on Falkreath Field: the Iron Captain.\n\n" +
          "He recognizes your father's eyes at once and draws his broadsword with a grim bark of " +
          "laughter. \"So Falkreath left a pup behind after all! Finish it then, boy — give me a " +
          "soldier's death!\" You smile coldly. Death is far too merciful. You need him alive in chains.";
        this.choices = [
          { label: "Subdue the Iron Captain in battle and drag him alive to the Hall of Chains" }
        ];
        break;

      case 9:
        this.heading = "Chapter 10: The Obsidian Forge";
        this.subheading = "Act II Finale — The Visitor in the Volcano";
        this.sceneImage = "images/prequel/obsidian_forge.jpg";
        this.body =
          "Deep beneath the mountain, rivers of magma illuminate the towering obsidian pillar " +
          "etched with runes from the stolen Luminari codex. The Focusing Seal pulses like a living " +
          "heart — one command, and Aethoria's capital will burn.\n\n" +
          "Footsteps echo along the basalt bridge. A hooded woman steps into the orange glow and " +
          "lowers her veil. Golden hair. Tear-hardened eyes. Princess Sophie of Aethoria.\n\n" +
          "\"My father took Eldred from both of us, Malvel,\" she says softly. \"Give me the seal. " +
          "Together, we will make them watch their own kingdom crumble.\"";
        this.choices = [
          { label: "Kneel before Sophie's resolve: \"You will lead. I will build the trap.\"" },
          { label: "Warn her of the cost: \"Once we light this seal, neither of us walks back into the sun.\"" }
        ];
        break;

      // ACT III — PRINCESS SOPHIE (Chapters 10..14)
      case 10:
        this.heading = "Chapter 11: The King's Triumph & The Well";
        this.subheading = "The Royal Banquet & The Riverbank";
        this.sceneImage = "images/prequel/falkreath_river.jpg";
        this.body =
          "Across the candlelit banquet hall, your father raises his goblet to the defeat of " +
          "'the traitor Eldred.' Every noble cheers. Every eye turns to see whether the princess " +
          "will smile.\n\n" +
          "In your sleeve rests the damp, hurried letter you wrote by the river — the very parchment " +
          "an adventurer will one day pull from the respite well. First, you must survive your " +
          "father's scrutiny at the high table without betraying a single tremor.";
        this.choices = [
          { label: "Hold your silk smile and enter the Throne Room Parley with the King" }
        ];
        break;

      case 11:
        this.heading = "Chapter 12: A Shadow in the Palace";
        this.subheading = "Midnight in the Royal Gallery";
        this.sceneImage = "images/prequel/candlelit_throne.jpg";
        this.body =
          "Slipping down the palace gallery to meet Malvel's courier, you freeze as a lithe shadow " +
          "drops silently from the rafters beside the royal gemstone display. A young thief from the " +
          "capital slums — Asher — meets your gaze, stunned motionless by the moonlight on your face.\n\n" +
          "You could call the guards in a heartbeat. Yet looking at the desperate devotion sudden in " +
          "his eyes, a colder thought forms: heroes come in many shapes, even from the slums.";
        this.choices = [
          { label: "Let Asher slip away into the night unharmed — leave the kingdom's future heroes untouched" },
          { label: "Drop a royal sapphire ribbon at his feet so rumor of the 'captive princess' haunts him" }
        ];
        break;

      case 12:
        this.heading = "Chapter 13: Silencing the Witness";
        this.subheading = "The Sands & Crypts of Floor 2";
        this.sceneImage = "images/prequel/candlelit_throne.jpg";
        this.body =
          "Descending into the outer dungeon vaults to inspect Malvel's defenses, you hear a trembling " +
          "voice behind a stone archway: the royal butler. He clutches a bundle of your intercepted " +
          "letters to Malvel.\n\n" +
          "\"Your Highness, please!\" he weeps. \"I kept your secret about Lord Eldred — I swear it! " +
          "Don't let the sorcerer burn innocent people in the capital!\"";
        this.choices = [
          { label: "Leave him trapped in the shifting vault with a single canteen — fate can decide if a hero frees him" },
          { label: "Coldly seal the stone gate behind him: \"You knew too much. Stay with the ghosts.\"" }
        ];
        break;

      case 13:
        this.heading = "Chapter 14: The Candlelit Throne";
        this.subheading = "The Pact Recorded by the Scrying Orb";
        this.sceneImage = "images/prequel/candlelit_throne.jpg";
        this.body =
          "Tall candles flicker around the dark obsidian throne. Malvel stands before you holding " +
          "a crimson scrying orb that records every syllable — the exact vision an adventurer will " +
          "one day witness on Floor 2.\n\n" +
          "\"Are you sure they'll believe this?\" Malvel asks, his voice low.\n" +
          "\"They'll believe anything,\" you reply, a calm, devastating smile crossing your lips. " +
          "\"This kingdom will tear itself apart looking for me. And when they do... we'll strike.\"\n\n" +
          "Beside your throne lies your open leather diary. Before the abduction begins, you set " +
          "quill to paper one last time.";
        this.choices = [
          { label: "Write the honest doubts gnawing at your soul: \"Can I live with the cost of my vengeance?\"" },
          { label: "Write only cold certainty: \"The weapon is ready. One command, and I will rule the ashes.\"" }
        ];
        break;

      case 14:
        this.heading = "Chapter 15: The Pedestal of Four Weapons";
        this.subheading = "Act III Finale — The Night of the Abduction";
        this.sceneImage = "images/prequel/royal_captain.jpg";
        this.body =
          "Midnight tolls over the palace towers. Smoke curls through the torchlit marble corridor " +
          "as Malvel's illusion fire splits the courtyard. Blocking the secret passage to the mountain " +
          "stands the Captain of the Palace Guard, halberd leveled.\n\n" +
          "Behind him, young knight Cassio lies stunned on the tiles — alive, as planned, so he will " +
          "wake and swear to rescue you. Clear the Royal Captain from the corridor, seal the outer " +
          "dungeon gate, and set the four weapons upon the stone pedestal.";
        this.choices = [
          { label: "Duel the Captain of the Palace Guard to spring the final stage of the trap" }
        ];
        break;

      default:
        this.finalizeCampaign();
        break;
    }
  }

  advance() {
    if (this.phase === PrequelPhase.ActIntro) {
      this.loadChapter(this.act * 5);
      return;
    }
    if (this.phase === PrequelPhase.Outcome) {
      if (this.pendingAfterOutcomeParley && this.queuedParley) {
        this.pendingAfterOutcomeParley = false;
        const p = this.queuedParley;
        this.queuedParley = null;
        this.startParley(p);
        return;
      }
      if (this.pendingAfterOutcomeCombat && this.queuedEnemy) {
        this.pendingAfterOutcomeCombat = false;
        const qe = this.queuedEnemy;
        this.queuedEnemy = null;
        this.startCombat(
          qe.name, qe.title, qe.image,
          qe.maxHp, qe.counterDamage, qe.maxStrikes,
          qe.boss, this.queuedVictoryText, this.queuedNextChapter
        );
        return;
      }
      if (this.pendingNextChapter >= 15) {
        this.finalizeCampaign();
      } else if (this.pendingNextChapter === 5) {
        this.phase = PrequelPhase.ActComplete;
        this.heading = "ACT I COMPLETE — THE FALL OF FALKREATH";
        this.subheading = "Eldred's Last Stand";
        this.sceneImage = "images/prequel/falkreath_field.jpg";
        this.body =
          "Though you broke the Iron Captain's guard and held the bridge until the last wagon " +
          "cleared the ridge, Aethoria's encircling lances closed from every side.\n\n" +
          "Bleeding and surrounded, Eldred fell to one knee as the Iron Captain drove his blade " +
          "through the throat of House Falkreath's lord — a memory that will haunt the captain " +
          "inside a dungeon cell for years to come.\n\n" +
          "Far above in the smoke of the mountain pass, Eldred's hidden son watched his father fall " +
          "and whispered an oath in the dark.";
      } else if (this.pendingNextChapter === 10) {
        this.phase = PrequelPhase.ActComplete;
        this.heading = "ACT II COMPLETE — THE OBSIDIAN PACT";
        this.subheading = "The Dungeon Is Built";
        this.sceneImage = "images/prequel/obsidian_forge.jpg";
        this.body =
          "The Luminari vault is plundered. Sir Henry lies beneath the mountain snow. " +
          "Eldred's killer rots alive behind the iron bars of the Hall of Chains.\n\n" +
          "Beneath the volcano, the Focusing Seal burns at full strength — awaiting only the " +
          "final architect who can lure Aethoria's finest heroes straight through its doors.";
      } else {
        this.loadChapter(this.pendingNextChapter);
      }
      return;
    }
    if (this.phase === PrequelPhase.ActComplete) {
      if (this.act === PrequelAct.Act1_Eldred) this.setupAct(PrequelAct.Act2_Malvel);
      else if (this.act === PrequelAct.Act2_Malvel) this.setupAct(PrequelAct.Act3_Sophie);
      else this.finalizeCampaign();
    }
  }

  choose(index) {
    if (this.phase !== PrequelPhase.Scene || index < 0 || index >= this.choices.length) return;

    switch (this.chapterIndex) {
      // --- ACT I ---
      case 0:
        if (index === 0) {
          this.legacy++;
          this.character.cunning += 2;
          this.body =
            "You signal the bailiffs to move half the granary downriver under cover of mist, " +
            "then step forward to match wits with the King's tax envoy before he searches the barns.";
        } else if (index === 1) {
          this.legacy++;
          this.character.resolve += 2;
          this.body =
            "You empty House Falkreath's ancestral coffers onto the courtyard table so not a single " +
            "son of Falkreath marches in Aethoria's war. The royal envoy narrows his eyes and presses you.";
        } else {
          this.fury++;
          this.character.power += 1.5;
          this.body =
            "The torn parchment flutters across the flagstones. The envoy's guards reach for their hilts " +
            "as the envoy demands an answer to carry back to the King.";
        }
        this.queuedParley = {
          speaker: "Royal Tax Envoy",
          prompt:
            "\"Defy the Crown's levy, Lord Eldred, and the King will brand House Falkreath traitors by sunrise. " +
            "By what right do you withhold grain and men from Aethoria?\"",
          options: [
            "\"By the Charter of the Riverlands — Falkreath feeds Aethoria in peace, never in unjust conquest.\"",
            "\"Take your threats back to your master before I toss you into the river.\"",
            "\"Perhaps the harvest simply rotted this season; check again in winter.\""
          ],
          bestIndex: 0,
          hiddenIndex: 3,
          timeLimit: 18,
          timeLeft: 18,
          successText:
            "The envoy falters before the assembled townsfolk — the Charter is royal law, and his guards dare not " +
            "draw steel before witnesses. He rides away empty-handed, buying Falkreath precious weeks (+2 Resolve, +2 Power).",
          failText:
            "The envoy smirks, scribbling treason charges on his wax tablet before galloping back to the capital. " +
            "War will reach Falkreath sooner than you hoped (-4 HP from mounting exhaustion, +1 Power)."
        };
        this.pendingAfterOutcomeParley = true;
        this.pendingNextChapter = 1;
        this.phase = PrequelPhase.Outcome;
        break;

      case 1:
        if (index === 0) {
          this.legacy++;
          this.remorse++;
          this.character.resolve += 2;
          this.character.hp = Math.min(this.character.maxHp, this.character.hp + 6);
          this.body =
            "The trembling butler bows low and swears on his late mother's memory never to betray " +
            "Princess Sophie's secret to the King. Sophie squeezes your hand in relief beside the well.\n\n" +
            "Years later, that same butler will risk everything to warn a stranger in the dungeon.";
        } else if (index === 1) {
          this.remorse++;
          this.character.cunning += 2;
          this.body =
            "You press Falkreath's warm silver signet into Sophie's fingers. \"Whatever comes, " +
            "remember what we fought for — not hatred, but a kinder realm.\" The butler retreats into the mist.";
        } else {
          this.fury++;
          this.character.power += 1.5;
          this.body =
            "Your guards escort the frightened butler to a locked guest chamber until sunrise. " +
            "No whisper reaches the palace tonight — though Sophie watches the closed door with a shadow in her eyes.";
        }
        this.pendingNextChapter = 2;
        this.phase = PrequelPhase.Outcome;
        break;

      case 2:
        if (index === 0) {
          this.legacy++;
          this.remorse++;
          this.character.resolve += 2;
          this.body =
            "Malvel dims the molten sparks around his fingers, jaw tight. \"You are a better man than " +
            "the King deserves, Father.\" He pockets your words — though embers still smolder beneath the stone.";
        } else if (index === 1) {
          this.legacy++;
          this.character.cunning += 2;
          this.character.agility += 2;
          this.body =
            "You unroll the ancient subterranean charts of Mount Aethor's caverns. " +
            "\"If the keep falls, lead our people beneath the ridge where Aethoria's cavalry cannot follow.\" " +
            "Malvel memorizes every tunnel — the very tunnels that will one day become the Dungeon.";
        } else {
          this.fury++;
          this.character.power += 2;
          this.body =
            "Malvel's eyes flare bright as furnace coal. \"Then let the King learn what sleeps beneath " +
            "his precious mountains.\" The obsidian runes hum louder in the dark.";
        }
        this.pendingNextChapter = 3;
        this.phase = PrequelPhase.Outcome;
        break;

      case 3:
        if (index === 0) {
          this.legacy++;
          this.character.power += 1.5;
          this.body =
            "You step beside Halt as his bowstring sings through the pines. \"Watch your left flank, " +
            "Lord Eldred!\" Together you meet the corrupt nobles' Vanguard Scouts in the clearing.";
        } else {
          this.legacy++;
          this.character.agility += 2;
          this.body =
            "\"Ride north, Ranger! Keep that dispatch alive!\" Halt salutes once and vanishes into the " +
            "snowy timber — surviving to clear his name years later — as the Vanguard Scouts surround you.";
        }
        this.queuedEnemy = {
          name: "Royal Vanguard Scouts",
          title: "Corrupt Cabal's Outriders",
          image: "images/prequel/falkreath_field.jpg",
          maxHp: 58,
          counterDamage: 6,
          maxStrikes: 6,
          boss: false
        };
        this.queuedVictoryText =
          "The vanguard riders scatter into the mist, leaving behind Aethoria's battle plans for Falkreath Field. " +
          "Halt's escape into the northern wilds is assured — though his name is already being branded traitor in the capital. " +
          "(+6 HP recovered, +1.5 Power)";
        this.queuedNextChapter = 4;
        this.pendingAfterOutcomeCombat = true;
        this.phase = PrequelPhase.Outcome;
        break;

      case 4:
        this.startCombat(
          "The Iron Captain",
          "Aethoria's Battle-Hardened Vanguard Commander",
          "images/prequel/iron_captain.jpg",
          86, 7.5, 7, true,
          "With a thunderous clash of steel, you shatter the Iron Captain's helmet visor and drive him " +
          "back across the scorched bridge! The wagons bearing Falkreath's survivors and young Malvel " +
          "clear the mountain ridge in safety — your duty as Lord of Falkreath is fulfilled.",
          5
        );
        break;

      // --- ACT II ---
      case 5:
        if (index === 0) {
          this.fury++;
          this.character.power += 2;
          this.body =
            "Obsidian fire scorches the rain into steam around your fists. The era of mercy died " +
            "with Eldred on Falkreath Field. Now Aethoria will learn to fear the dark.";
        } else if (index === 1) {
          this.legacy++;
          this.character.cunning += 2;
          this.character.agility += 2;
          this.body =
            "Anger without precision gets men killed. You slip Eldred's signet onto your finger " +
            "and plan your infiltration of the Ancient Library of Wan down to the second.";
        } else {
          this.legacy++;
          this.character.maxHp += 6;
          this.character.hp = this.character.maxHp;
          this.body =
            "Beneath Mount Aethor's basalt arches, surviving smiths of House Falkreath temper your " +
            "staff with volcanic glass. Their loyalty steels your resolve (+6 Max HP).";
        }
        this.pendingNextChapter = 6;
        this.phase = PrequelPhase.Outcome;
        break;

      case 6:
        if (index === 0) {
          this.character.cunning += 1.5;
          this.body =
            "You step beneath the hovering blue rune circle and chant a counter-harmonic from the " +
            "forbidden texts, challenging Arch-Mage Solon's ward before steel or flame is drawn.";
          this.queuedParley = {
            speaker: "Arch-Mage Solon of the Luminari",
            prompt:
              "\"The Luminari sealed the Obsidian Codex to protect the realm from volcanic annihilation! " +
              "Speak the counter-seal's true weakness, intruder, or be bound in golden chains!\"",
            options: [
              "\"Your ward feeds on fear — invert the inner triad with molten resonance and the circle shatters!\"",
              "\"Overload the outer candles with frost magic!\"",
              "\"Burn the bookshelves until the smoke blinds the Luminari!\""
            ],
            bestIndex: 0,
            hiddenIndex: 3,
            timeLimit: 18,
            timeLeft: 18,
            successText:
              "The Luminari ward fractures in a shower of blue sparks! Young scholar Elian dives behind a pillar " +
              "as Arch-Mage Solon's barrier weakens before the duel begins (+2 Power, +2 Resolve).",
            failText:
              "Solon's ward lashes back with radiant lightning (-5 HP), forcing you into combat with his barrier intact."
          };
          this.queuedEnemy = {
            name: "Arch-Mage Solon",
            title: "Master of the Luminari",
            image: "images/prequel/archmage_solon.jpg",
            maxHp: 84,
            counterDamage: 7.5,
            maxStrikes: 7,
            boss: true
          };
          this.queuedVictoryText =
            "Arch-Mage Solon's golden staff clatters across the marble tiles. You seize the black-bound " +
            "Codex of the Obsidian Seal from the lectern while young Elian stares from the gallery — " +
            "swearing the Luminari will hunt you to the ends of Aethoria. (+8 HP recovered, +2 Power)";
          this.queuedNextChapter = 7;
          this.pendingAfterOutcomeParley = true;
          this.pendingAfterOutcomeCombat = true;
          this.phase = PrequelPhase.Outcome;
        } else {
          this.fury++;
          this.character.power += 2;
          this.startCombat(
            "Arch-Mage Solon",
            "Master of the Luminari",
            "images/prequel/archmage_solon.jpg",
            90, 8, 7, true,
            "Golden wards shatter under waves of obsidian flame! You claim the Codex of the Obsidian Seal " +
            "as young Elian drags the wounded Arch-Mage to cover — cementing your blood feud with the Luminari. " +
            "(+8 HP recovered, +2 Power)",
            7
          );
        }
        break;

      case 7:
        if (index === 0) {
          this.remorse++;
          this.character.resolve += 2;
          this.body =
            "\"Step aside, Sir Henry,\" you murmur as snow swirls across the stone arch. \"My quarrel " +
            "is with the King who butchered Falkreath, not with a decent knight.\" Sir Henry raises his shield: " +
            "\"My oath is my life, sorcerer. Draw!\"";
        } else {
          this.fury++;
          this.character.power += 2;
          this.body =
            "\"The King's Guard stood silent while Falkreath burned!\" Obsidian shadows twist behind " +
            "you into a towering phantom as Sir Henry plants his boots on the icy bridge.";
        }
        this.queuedEnemy = {
          name: "Sir Henry",
          title: "Knight of the King's Guard — Cassio's Mentor",
          image: "images/prequel/sir_henry.jpg",
          maxHp: 94,
          counterDamage: 8,
          maxStrikes: 8,
          boss: true
        };
        this.queuedVictoryText =
          "Sir Henry's lion shield splits in two under your final surge of magma. The silver-haired knight " +
          "falls quietly onto the snow-dusted stone, whispering a prayer for his young squire Cassio.\n\n" +
          "An hour later, Cassio will find his mentor's cold dragon-steel sword on this bridge — and swear " +
          "to follow Malvel into the darkest dungeon on earth. (+8 HP recovered, +1.5 Power)";
        this.queuedNextChapter = 8;
        this.pendingAfterOutcomeCombat = true;
        this.phase = PrequelPhase.Outcome;
        break;

      case 8:
        this.startCombat(
          "The Iron Captain",
          "Slayer of Lord Eldred",
          "images/prequel/iron_captain.jpg",
          102, 8.5, 8, true,
          "Disarmed and bound in iron shackles, Eldred's killer glares up through bloodied teeth. " +
          "\"Kill me, Falkreath!\" You shake your head and drag him down into the freezing dark of the " +
          "Hall of Chains: \"Live long enough to watch what your King's greed built.\" (+8 HP recovered)",
          9
        );
        break;

      case 9:
        if (index === 0) {
          this.fury++;
          this.body =
            "You bow your head before the glowing obsidian obelisk. \"Your father built his throne " +
            "on Falkreath's bones, Sophie. Command me, and we will give Aethoria the reckoning it earned.\"";
        } else {
          this.remorse++;
          this.body =
            "Sophie stares into the churning lava below the Focusing Seal. \"I already lost the sun " +
            "the day my father murdered Eldred,\" she whispers. \"Now finish the seal.\"";
        }
        this.pendingNextChapter = 10;
        this.phase = PrequelPhase.Outcome;
        break;

      // --- ACT III ---
      case 10:
        this.startParley({
          speaker: "The King of Aethoria",
          prompt:
            "\"You have been unusually quiet since Falkreath's fall, Sophie. Rumors say you met their traitor " +
            "lord by the river before the battle. Look your father in the eye and answer me!\"",
          options: [
            "\"I walked by the river to pray for Aethoria's soldiers, Father — never doubt my loyalty to this crown.\"",
            "\"Eldred was twice the ruler you will ever be!\"",
            "\"Ask the royal butler where I was — he followed me everywhere!\""
          ],
          bestIndex: 0,
          hiddenIndex: 3,
          timeLimit: 18,
          timeLeft: 18,
          successText:
            "Your smile never flickers. The King laughs warmly, satisfied, and raises his cup to his 'dutiful daughter.' " +
            "That very night you slip down to the riverbank, weep one final time over your letter, and hide it deep in the respite well (+2 Cunning, +2 Power).",
          failText:
            "The King's eyes narrow with cold suspicion, doubling the guards outside your tower (-5 HP from stress). " +
            "Still, you slip past them at midnight and bury your tear-stained letter inside the respite well."
        });
        this.pendingNextChapter = 11;
        break;

      case 11:
        if (index === 0) {
          this.remorse++;
          this.character.cunning += 2;
          this.body =
            "You raise one quiet finger to your lips. Asher blinks in disbelief, pockets nothing, " +
            "and slips back into the shadows — carrying a sudden, fierce longing to become someone worthy " +
            "of the princess who spared him.";
        } else {
          this.fury++;
          this.character.power += 2;
          this.body =
            "You let a sapphire ribbon fall onto the marble floor before turning away without a word. " +
            "Asher catches it like a sacred relic — another pawn bound to descend when the 'abduction' begins.";
        }
        this.pendingNextChapter = 12;
        this.phase = PrequelPhase.Outcome;
        break;

      case 12:
        if (index === 0) {
          this.remorse++;
          this.legacy++;
          this.character.resolve += 2;
          this.character.hp = Math.min(this.character.maxHp, this.character.hp + 6);
          this.body =
            "Your hand trembles on the iron lever as you toss a canteen and small lamp into the " +
            "chamber before locking the gate. \"Survive, if you can,\" you whisper. " +
            "If a worthy hero enters these halls, the butler will live to speak the truth.";
        } else {
          this.fury++;
          this.character.power += 2;
          this.body =
            "You turn the heavy iron key without looking back as the butler's pleas fade behind the " +
            "shifting sands and crypt mist. No secret can be allowed to reach the palace early.";
        }
        this.pendingNextChapter = 13;
        this.phase = PrequelPhase.Outcome;
        break;

      case 13:
        if (index === 0) {
          this.remorse++;
          this.character.resolve += 2;
          this.character.hp = Math.min(this.character.maxHp, this.character.hp + 6);
          this.body =
            "Ink darkens the leather-bound page: \"I tell myself this is for Falkreath, but doubts " +
            "creep in... I wonder if I can live with the cost of my vengeance.\" " +
            "You leave the diary in the lava library — perhaps hoping someone will read it in time.";
        } else {
          this.fury++;
          this.character.power += 2.5;
          this.body =
            "Your handwriting slashes across the parchment: \"It is finished. One command, and " +
            "nothing will survive. I will be the one to rule over the ashes.\" " +
            "Malvel bows deeply as the crimson orb fades cold.";
        }
        this.pendingNextChapter = 14;
        this.phase = PrequelPhase.Outcome;
        break;

      case 14:
        this.startCombat(
          "Captain of the Palace Guard",
          "Shield of the Royal Corridor",
          "images/prequel/royal_captain.jpg",
          106, 8.5, 8, true,
          "With a flash of silver steel and molten light, you disarm the Palace Captain and slip past " +
          "the stunned form of young knight Cassio — leaving him unharmed to carry the alarm to the King.\n\n" +
          "Moments later, inside the damp entrance chamber beneath the mountains, you and Malvel lay " +
          "four gleaming weapons upon the mossy stone pedestal: a runic sword, a throwing knife, " +
          "a mystical wand, and a curved hunting longbow.",
          15
        );
        break;

      default:
        break;
    }
  }

  startParley(p) {
    const bonusSec = Math.floor(this.character.resolve / 2);
    this.parley = {
      ...p,
      options: [...p.options],
      timeLimit: p.timeLimit + bonusSec,
      timeLeft: p.timeLimit + bonusSec,
      hiddenIndex: this.character.cunning >= 8 ? (p.bestIndex + 2) % 3 : 3
    };
    this.phase = PrequelPhase.Parley;
    this.heading = `TACTICAL PARLEY — ${this.parley.speaker}`;
    this.subheading = "Choose your response before time expires";
    this.body = this.parley.prompt;
  }

  parleyOptionAvailable(index) {
    return index >= 0 && index < 3 && index !== this.parley.hiddenIndex;
  }

  tickParley(dt) {
    if (this.phase !== PrequelPhase.Parley) return;
    this.parley.timeLeft = Math.max(0, this.parley.timeLeft - Math.max(0, dt));
    if (this.parley.timeLeft <= 0) {
      this.answerParley(99);
    }
  }

  answerParley(index) {
    if (this.phase !== PrequelPhase.Parley) return;
    const ok = index === this.parley.bestIndex;
    if (ok) {
      this.parleysWon++;
      this.legacy++;
      this.character.power += 2;
      this.character.resolve += 2;
      this.body = this.parley.successText;
      this.heading = "PARLEY SUCCEEDED";
      this.subheading = this.parley.speaker;
    } else {
      this.character.hp = Math.max(6, this.character.hp - 5);
      this.body = (index >= 3 ? "Hesitation cost you the moment! " : "") + this.parley.failText;
      this.heading = "PARLEY FALTERED";
      this.subheading = this.parley.speaker;
    }
    this.phase = PrequelPhase.Outcome;
    return ok;
  }

  startCombat(name, title, img, hp, counterDmg, baseStrikes, boss, victoryText, nextAfterWin) {
    const bonusStrikes = Math.floor(this.character.agility / 3);
    this.enemy = {
      name,
      title,
      image: img,
      maxHp: hp,
      hp,
      counterDamage: counterDmg,
      maxStrikes: baseStrikes + bonusStrikes,
      strikesLeft: baseStrikes + bonusStrikes,
      turn: 0,
      boss,
      abilityUsedThisBattle: false,
      rallyGuardActive: false,
      intent: {
        active: boss,
        telegraphed: false,
        name: "",
        detail: "",
        counterMultiplier: 1,
        damageTakenMultiplier: 1
      }
    };
    this.pendingVictoryText = victoryText;
    this.pendingNextChapter = nextAfterWin;
    this.sceneImage = img;
    this.heading = name;
    this.subheading = title;
    this.body =
      "Choose your combat tactic. Watch for telegraphed boss intents — Guard Strike dampens lethal " +
      "counters, Heavy Strike trades two strikes for burst power, and your signature ability can turn the tide once per battle.";
    this.lastExchange = defaultExchange();
    this.updateBossIntent();
    this.phase = PrequelPhase.Combat;
  }

  updateBossIntent() {
    if (!this.enemy.boss) {
      this.enemy.intent = {
        active: false,
        telegraphed: false,
        name: "Skirmish Counter",
        detail: "Standard retaliation.",
        counterMultiplier: 1,
        damageTakenMultiplier: 1
      };
      return;
    }
    const cycle = this.enemy.turn % 3;
    if (cycle === 1) {
      this.enemy.intent = {
        active: true,
        telegraphed: true,
        counterMultiplier: 1.65,
        damageTakenMultiplier: 1,
        name: "TELEGRAPHED: Crushing Counter!",
        detail: "1.65x retaliation this turn — use Guard Strike or your signature ability!"
      };
    } else if (cycle === 2) {
      this.enemy.intent = {
        active: true,
        telegraphed: true,
        counterMultiplier: 0.9,
        damageTakenMultiplier: 0.72,
        name: "Defensive Stance",
        detail: "Armor/ward raised (0.72x normal damage — signature abilities pierce it)."
      };
    } else {
      this.enemy.intent = {
        active: true,
        telegraphed: false,
        counterMultiplier: 1,
        damageTakenMultiplier: 1,
        name: "Measured Stance",
        detail: "Standard retaliation."
      };
    }
  }

  combatChoiceAvailable(action) {
    if (this.phase !== PrequelPhase.Combat) return false;
    if (action === CombatAction.HeavyStrike) return this.enemy.strikesLeft >= 2;
    if (action === CombatAction.ClassAbility) {
      return !this.enemy.abilityUsedThisBattle && this.enemy.strikesLeft >= 1;
    }
    return this.enemy.strikesLeft >= 1;
  }

  combatChoices() {
    return [
      "Standard Strike (1 strike)",
      "Heavy Strike (2 strikes · 1.9x dmg)",
      "Guard Strike (1 strike · -65% counter)",
      `${this.character.abilityName} (Once/battle)`
    ];
  }

  previewCombatAction(action) {
    const available = this.combatChoiceAvailable(action);
    const resolveMitigation = Math.max(0.65, 1 - this.character.resolve * 0.025);
    const baseCounter = this.enemy.counterDamage * this.enemy.intent.counterMultiplier * resolveMitigation;
    const wardMult = this.enemy.intent.damageTakenMultiplier;

    let title = "Standard Strike";
    let strikeCost = 1;
    let minDamage = this.character.power * wardMult;
    let expectedCounter = baseCounter;
    let effect = "Reliable strike with standard retaliation.";

    if (action === CombatAction.HeavyStrike) {
      title = "Heavy Strike";
      strikeCost = 2;
      minDamage = this.character.power * 1.9 * wardMult;
      expectedCounter = baseCounter * 1.2;
      effect = "Costs 2 strikes, 1.9x damage, 1.2x counter.";
    } else if (action === CombatAction.GuardStrike) {
      title = "Guard Strike";
      strikeCost = 1;
      minDamage = this.character.power * 0.78 * wardMult;
      expectedCounter = baseCounter * 0.35;
      effect = "0.78x damage, cuts retaliation by 65%.";
    } else if (action === CombatAction.ClassAbility) {
      title = this.character.abilityName;
      strikeCost = 1;
      if (this.character.act === PrequelAct.Act1_Eldred) {
        minDamage = this.character.power * 1.4;
        expectedCounter = baseCounter * 0.4;
        effect = "Rally the Banner: pierces stance, 0.4x counter, +4 HP.";
      } else if (this.character.act === PrequelAct.Act2_Malvel) {
        minDamage = this.character.power * 1.55;
        expectedCounter = 0;
        effect = "Molten Ward: 1.55x ward-piercing surge, blocks counter.";
      } else {
        minDamage = this.character.power * 1.45;
        expectedCounter = 0;
        effect = "Sovereign Command: 1.45x strike, staggers enemy, +5 HP.";
      }
    }

    return {
      available,
      title,
      strikeCost,
      minDamage,
      critDamage: minDamage * 1.9,
      expectedCounter,
      effect
    };
  }

  attack(action = CombatAction.StandardStrike) {
    if (!this.combatChoiceAvailable(action)) return null;

    const preview = this.previewCombatAction(action);
    const ex = {
      ...defaultExchange(),
      valid: true,
      action,
      strikesSpent: preview.strikeCost
    };

    const critChance = Math.min(0.45, this.character.cunning / 32);
    const crit = this.nextUnitFloat() < critChance;
    ex.critical = crit;

    const dmg = Math.round((crit ? preview.critDamage : preview.minDamage) * 10) / 10;
    this.enemy.strikesLeft = Math.max(0, this.enemy.strikesLeft - preview.strikeCost);
    this.enemy.hp = Math.max(0, Math.round((this.enemy.hp - dmg) * 10) / 10);
    ex.damageDealt = dmg;

    let healed = 0;
    let counterNegated = false;
    if (action === CombatAction.ClassAbility) {
      this.enemy.abilityUsedThisBattle = true;
      this.abilitiesUsed++;
      if (this.character.act === PrequelAct.Act1_Eldred) {
        const before = this.character.hp;
        this.character.hp = Math.min(this.character.maxHp, this.character.hp + 4);
        healed = Math.round((this.character.hp - before) * 10) / 10;
      } else if (this.character.act === PrequelAct.Act3_Sophie) {
        const before = this.character.hp;
        this.character.hp = Math.min(this.character.maxHp, this.character.hp + 5);
        healed = Math.round((this.character.hp - before) * 10) / 10;
      }
      if (preview.expectedCounter <= 0.01) counterNegated = true;
    }

    let counter = 0;
    if (this.enemy.hp > 0) {
      counter = Math.round(preview.expectedCounter * 10) / 10;
      if (counter > 0) {
        this.character.hp = Math.max(0, Math.round((this.character.hp - counter) * 10) / 10);
        ex.counterattacked = true;
        ex.counterDamage = counter;
      }
      if (action === CombatAction.GuardStrike) ex.guarded = true;
    } else {
      ex.enemyDefeated = true;
    }

    let log = `${this.character.name} uses ${preview.title} for ${formatNum(dmg)}${crit ? " CRIT" : ""} damage!`;
    if (healed > 0) log += ` Recovered +${formatNum(healed)} HP.`;
    if (this.enemy.hp <= 0) {
      log += `\n${this.enemy.name} is defeated!`;
    } else if (counterNegated) {
      log += `\n${this.enemy.name}'s retaliation is completely staggered!`;
    } else {
      log += `\n${this.enemy.name} retaliates for ${formatNum(counter)} HP.`;
    }

    if (this.enemy.hp <= 0) {
      this.lastExchange = ex;
      this.battlesWon++;
      this.character.hp = Math.min(this.character.maxHp, this.character.hp + 8);
      this.character.power += 1.5;
      this.body = `${log}\n\n${this.pendingVictoryText}`;
      this.heading = `VICTORY — ${this.enemy.name}`;
      this.phase = PrequelPhase.Outcome;
      return ex;
    }

    if (this.character.hp <= 0 || this.enemy.strikesLeft <= 0) {
      ex.playerDefeated = true;
      this.lastExchange = ex;
      this.heading = "DEFEATED";
      this.subheading = `${this.enemy.name} prevailed`;
      this.body =
        `${log}\n\n` +
        (this.character.hp <= 0
          ? "Your strength gives out before the chronicle could reach its end."
          : "Your strike window closed before you could break the enemy's line.") +
        "\n\nRetry this Act to press onward through Ashes of Falkreath.";
      this.phase = PrequelPhase.Defeat;
      return ex;
    }

    this.lastExchange = ex;
    this.enemy.turn++;
    this.updateBossIntent();
    this.body = log;
    return ex;
  }

  finalizeCampaign() {
    this.phase = PrequelPhase.Epilogue;
    this.sceneImage = "images/prequel/pedestal_chamber.jpg";
    this.choices = [];

    if (this.remorse >= this.fury) {
      this.coda = PrequelCoda.LastDoubt;
      this.heading = prequelCodaTitle(this.coda);
      this.subheading = "The Pedestal of Four Weapons — Minutes Before Dungeon Descent";
      this.body =
        "Two torches hiss against damp moss-grown stone. Upon the central pedestal gleam four " +
        "weapons laid in silent symmetry: a runic dragon-steel sword, a throwing knife, a carved " +
        "wooden wand, and a curved hunting longbow.\n\n" +
        "Princess Sophie pauses at the threshold of the molten descent, her hand resting lightly " +
        "on Eldred's river signet hidden inside her cloak. Beneath her silk smile, a quiet prayer " +
        "lingers — that whoever wakes in this chamber will uncover the letter, the book, and the " +
        "diary she left behind, and find a justice cleaner than her fire.\n\n" +
        "The iron gate seals shut. Above the obsidian throne, the gilded cage swings closed.\n\n" +
        "Somewhere in the damp dark, a groggy adventurer slowly opens their eyes...";
    } else {
      this.coda = PrequelCoda.HollowCrown;
      this.heading = prequelCodaTitle(this.coda);
      this.subheading = "The Pedestal of Four Weapons — Minutes Before Dungeon Descent";
      this.body =
        "Two torches flare against the damp brick walls. Upon the carved stone pedestal rest four " +
        "weapons chosen for Aethoria's proudest champions: a runic sword for Sir Henry's squire, " +
        "a wand for the Luminari, a longbow for the exiled ranger, and a knife for the palace thief.\n\n" +
        "Malvel seals the outer portcullis with a twist of obsidian flame. Beside him, Princess " +
        "Sophie does not look back. No mercy remains for the crown that buried House Falkreath — " +
        "only the Focusing Seal burning beneath the lava, waiting for one final command.\n\n" +
        "She steps willingly into the gilded cage and wears her most convincing smile.\n\n" +
        "Somewhere in the damp dark, a groggy adventurer slowly opens their eyes...";
    }

    if (this.ironChronicleEligible()) {
      this.body +=
        "\n\n[IRON CHRONICLE ACHIEVED]: Completed all three Acts without using a single signature ability in combat!";
    }
  }
}
