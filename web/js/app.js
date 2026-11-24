// Dungeon Descent — Web Edition Application Controller

import { AssetStore } from "./assets.js";
import { AudioEngine } from "./audio.js";
import {
  DungeonRun, Hero, Difficulty, Biome, Room, Phase, Ending, Stat, CombatAction,
  heroName, heroWeapon, difficultyName, biomeName, biomeKey, biomeColor,
  portraitImage, bossImage, heroAbilitySummary, combatAbilityName, combatAbilityDescription,
  statName, statTooltip, endingName, endingHint, endingImage, endingStoryFile, endingMusicFile,
  makePlayer, formatNum, formatDuration
} from "./engine.js";
import { MT19937, parseSeed, dailyRunDateString, dailyRunSeedForToday, randomSeed } from "./rng.js";
import { WebStore, getCodexMilestones, isEndingUnlocked, isPrequelUnlocked, unlockedEndingCount, mergeCodex } from "./storage.js";
import { GameApiClient } from "./api.js";
import {
  PrequelRun, PrequelAct, PrequelPhase, PrequelCoda,
  prequelActTitle, prequelActAccent
} from "./prequel.js";

const Screen = Object.freeze({
  Intro: "intro",
  Menu: "menu",
  Introduction: "introduction",
  Heroes: "heroes",
  Lore: "lore",
  Difficulty: "difficulty",
  Biomes: "biomes",
  Game: "game",
  Results: "results",
  Leaderboard: "leaderboard",
  Codex: "codex",
  Gauntlet: "gauntlet",
  Prequel: "prequel"
});

const Overlay = Object.freeze({
  None: "none",
  Pause: "pause",
  Map: "map",
  Settings: "settings",
  Stats: "stats",
  Account: "account"
});

function escHtml(str) {
  return String(str ?? "")
    .replace(/&/g, "&amp;")
    .replace(/</g, "&lt;")
    .replace(/>/g, "&gt;")
    .replace(/"/g, "&quot;");
}

function imageAspectClass() {
  return "aspect-square";
}

function paginateText(text, maxChars = 900) {
  const cleaned = String(text || "").trim();
  if (!cleaned) return [""];
  const paragraphs = cleaned.split(/\n\s*\n/);
  const pages = [];
  let current = "";
  for (const para of paragraphs) {
    const piece = para.trim();
    if (!piece) continue;
    if (current && (current.length + piece.length + 2) > maxChars) {
      pages.push(current);
      current = piece;
    } else {
      current = current ? `${current}\n\n${piece}` : piece;
    }
  }
  if (current) pages.push(current);
  return pages.length ? pages : [cleaned];
}

class DungeonWebApp {
  constructor() {
    this.root = document.getElementById("app");
    this.overlayRoot = document.getElementById("overlay-root");
    this.toastRoot = document.getElementById("toast-root");
    this.fadeEl = document.getElementById("screen-fade");

    this.assets = new AssetStore("./assets");
    this.store = new WebStore();
    this.api = new GameApiClient();
    this.audio = new AudioEngine("./assets");
    this.run = new DungeonRun(this.assets);
    this.prequel = new PrequelRun();
    this.prequelRecordedSerial = 0;

    // Optional URL playtesting flags matching desktop CLI flags (?unlock-prequel=1, ?skip-intro=1)
    let urlParams = null;
    try {
      urlParams = new URLSearchParams(window.location.search);
    } catch {}
    const flagOn = (key) => {
      const v = urlParams?.get(key);
      return v === "1" || v === "true" || (v !== null && v === "");
    };
    this.unlockPrequelParam = flagOn("unlock-prequel");
    this.skipIntroParam = flagOn("skip-intro");

    this.settings = this.store.loadSettings();
    this.audio.applySettings(this.settings);
    this.codex = this.store.loadCodex();
    this.leaderboards = this.store.loadLeaderboards();
    this.suspendPreview = this.store.loadSuspendPreview();

    this.screen = this.skipIntroParam ? Screen.Menu : Screen.Intro;
    this.overlay = Overlay.None;
    this.introPlaying = false;
    this.introFrame = 1;
    this.introTimer = 0;
    this.menuFrame = 1;
    this.menuTimer = 0;
    this.menuFrames = [];
    this.menuFramesPreloading = false;

    this.selectedHero = Hero.Warrior;
    this.selectedDifficulty = Difficulty.Easy;
    this.choosingSecondFloor = false;
    this.seedInput = "";
    this.dailyRunSelected = false;
    this.activeSeed = 1;
    this.playTimeSeconds = 0;

    this.loreReturn = Screen.Menu;
    this.loreHero = Hero.Warrior;

    this.page = 0;
    this.hoveredCombatAction = CombatAction.StandardStrike;
    this.combatBadges = [];
    this.stageShake = false;
    this.stageFlashColor = "";
    this.lastTimerSec = -1;
    this.codexRecordedSerial = 0;

    // Results & Leaderboard UI state
    this.entryPending = false;
    this.initialsValue = "";
    this.lastEntryId = null;
    this.lastGlobalEntryId = null;
    this.lbMode = "global"; // "global" | "local"
    this.lbDifficulty = -1; // -1 = All Difficulties, 0..3 = Easy..Impossible
    this.lbHeroFilter = -1;
    this.lbEndingFilter = 0;
    this.lbMineOnly = false;
    this.lbPage = 0;
    this.globalLbData = null;
    this.globalLbLoading = false;
    this.globalLbError = "";
    this.globalLbReqId = 0;
    this.submittingScore = false;

    // Account modal state
    this.accountTab = "login"; // "login" | "register"
    this.accountError = "";
    this.accountBusy = false;
    this.accountUsernameDraft = "";

    // Codex UI state
    this.codexTab = 0; // 0 = Career & Milestones, 1 = Ending Gallery
    this.codexMilestonePage = 0;
    this.codexEnding = Ending.Good;

    // Riddle Gauntlet state
    this.gauntlet = null;
    this.gauntletRng = new MT19937(randomSeed());

    this.lastFrameTs = performance.now();
    this.tickHandle = null;
  }

  prequelAccessible() {
    return isPrequelUnlocked(this.codex) || this.unlockPrequelParam;
  }

  async boot() {
    const bar = document.getElementById("loading-bar");
    try {
      await this.assets.preloadAll((ratio) => {
        if (bar) bar.style.width = `${Math.round(15 + ratio * 85)}%`;
      });
    } catch (err) {
      this.root.innerHTML = `
        <main class="loading-screen">
          <div class="loading-card">
            <h1 class="brand-title">DUNGEON DESCENT</h1>
            <p class="loading-subtitle" style="color: var(--negative);">Unable to load game assets: ${escHtml(err.message)}</p>
          </div>
        </main>`;
      return;
    }

    // Preload all 11 flickering candle intro frames for zero-flicker 10 fps playback
    this.candleFrames = Array.from({ length: 11 }, (_, i) => {
      const img = new Image();
      img.src = this.assets.assetUrl(`candle/candle${i + 1}.jpg`);
      return img;
    });

    this.bindGlobalEvents();
    this.render();
    this.startLoop();
    this.preloadMenuFrames();
    this.restoreAccountSession();
  }

  preloadMenuFrames() {
    if (this.menuFramesPreloading || this.menuFrames.length === 91) return;
    this.menuFramesPreloading = true;
    this.menuFrames = new Array(91).fill(null);
    let nextIdx = 0;
    const concurrency = 6;
    const loadNext = () => {
      if (nextIdx >= 91) return;
      const idx = nextIdx++;
      const img = new Image();
      img.onload = loadNext;
      img.onerror = loadNext;
      img.src = this.assets.assetUrl(`images/startscreen/startscreen${idx + 1}.jpg`);
      this.menuFrames[idx] = img;
    };
    for (let i = 0; i < concurrency; i++) loadNext();
  }

  toast(msg) {
    if (!this.toastRoot) return;
    const el = document.createElement("div");
    el.className = "toast";
    el.textContent = msg;
    this.toastRoot.appendChild(el);
    setTimeout(() => el.remove(), 2600);
  }

  triggerScreenFade() {
    if (!this.fadeEl) return;
    this.fadeEl.classList.add("active");
    requestAnimationFrame(() => {
      setTimeout(() => this.fadeEl.classList.remove("active"), 45);
    });
  }

  enter(nextScreen) {
    this.audio.stopNarration();
    this.introPlaying = false;
    this.screen = nextScreen;
    this.overlay = Overlay.None;
    this.page = 0;
    this.hoveredCombatAction = CombatAction.StandardStrike;
    this.combatBadges = [];
    this.stageFlashColor = "";
    this.stageShake = false;
    this.triggerScreenFade();

    if (nextScreen === Screen.Menu) {
      this.suspendPreview = this.store.loadSuspendPreview();
      this.codex = this.store.loadCodex();
      this.preloadMenuFrames();
      this.audio.playMusic("menu.mp3");
    } else if (nextScreen === Screen.Prequel) {
      if (this.prequel.serial === 0) this.prequel.start();
      this.audio.playMusic(this.prequel.musicFile());
    } else if (nextScreen === Screen.Codex) {
      this.codex = this.store.loadCodex();
      this.syncCodexMusic();
      if (this.api.isSignedIn()) this.syncCodexToCloud({ silent: true });
    } else if (nextScreen === Screen.Gauntlet) {
      this.startGauntlet();
      this.audio.playMusic("menu.mp3");
    } else if (nextScreen === Screen.Leaderboard) {
      this.leaderboards = this.store.loadLeaderboards();
      this.lbDifficulty = -1;
      this.lbPage = 0;
      this.lbHeroFilter = -1;
      this.lbEndingFilter = 0;
      this.lbMineOnly = false;
      this.entryPending = false;
      this.audio.playMusic("menu.mp3");
      if (this.lbMode === "global") this.fetchGlobalLeaderboard();
    } else if (nextScreen === Screen.Results) {
      this.leaderboards = this.store.loadLeaderboards();
      this.lbDifficulty = -1;
      this.lbPage = 0;
      this.lbHeroFilter = -1;
      this.lbEndingFilter = 0;
      this.lbMineOnly = false;
      this.entryPending = this.hasEndingResult();
      this.initialsValue = "";
      this.lastEntryId = null;
      this.lastGlobalEntryId = null;
      this.submittingScore = false;
      if (this.lbMode === "global") this.fetchGlobalLeaderboard();
    }
    this.render();
  }

  hasEndingResult() {
    return this.run.phase === Phase.Ending && this.run.ending !== Ending.None;
  }

  syncCodexMusic() {
    if (this.screen === Screen.Codex && this.codexTab === 1 && isEndingUnlocked(this.codex, this.codexEnding)) {
      this.audio.playMusic(endingMusicFile(this.codexEnding));
    } else {
      this.audio.playMusic("menu.mp3");
    }
  }

  recordCodexIfNeeded() {
    if (this.codexRecordedSerial === this.run.runSerial) return;
    if (this.run.phase !== Phase.Ending && this.run.phase !== Phase.Defeat) return;
    this.codexRecordedSerial = this.run.runSerial;
    const wasUnlocked = isPrequelUnlocked(this.codex);
    this.codex = this.store.recordCompletedRun(this.run, Math.round(this.playTimeSeconds));
    if (!wasUnlocked && isPrequelUnlocked(this.codex)) {
      this.toast("THE SEAL BREAKS! Ashes of Falkreath (Prequel) unlocked on the Main Menu!");
    }
    if (this.api.isSignedIn()) {
      this.syncCodexToCloud({ silent: true });
    }
  }

  recordPrequelIfNeeded() {
    if (this.prequel.phase !== PrequelPhase.Epilogue || this.prequel.serial === 0 ||
        this.prequelRecordedSerial === this.prequel.serial) return;
    this.prequelRecordedSerial = this.prequel.serial;
    const codaIdx = this.prequel.coda === PrequelCoda.HollowCrown ? 0
      : this.prequel.coda === PrequelCoda.LastDoubt ? 1 : -1;
    this.codex = this.store.recordPrequelCompletion(codaIdx, this.prequel.ironChronicleEligible());
    if (this.api.isSignedIn()) {
      this.syncCodexToCloud({ silent: true });
    }
  }

  async restoreAccountSession() {
    if (!this.api.isSignedIn()) return;
    try {
      const data = await this.api.restoreSession();
      if (data?.codex) {
        const localBefore = this.store.loadCodex();
        const merged = mergeCodex(localBefore, data.codex);
        this.codex = merged;
        this.store.saveCodex(merged);
        if (JSON.stringify(merged) !== JSON.stringify(data.codex)) {
          await this.api.syncCodex(merged).catch(() => {});
        }
      }
      if ([Screen.Menu, Screen.Codex, Screen.Leaderboard, Screen.Results].includes(this.screen)) {
        this.render();
      }
    } catch {
      // Offline or expired token (401 already cleared session inside GameApiClient)
      if ([Screen.Menu, Screen.Codex, Screen.Leaderboard, Screen.Results].includes(this.screen)) {
        this.render();
      }
    }
  }

  async syncCodexToCloud({ silent = true } = {}) {
    if (!this.api.isSignedIn()) return false;
    try {
      const localNow = this.store.loadCodex();
      const merged = await this.api.syncCodex(localNow);
      if (merged) {
        this.codex = mergeCodex(localNow, merged);
        this.store.saveCodex(this.codex);
        if (!silent) this.toast("Codex synced with your cloud account!");
        if (this.screen === Screen.Codex || this.screen === Screen.Menu) this.render();
        if (this.overlay === Overlay.Account) this.renderOverlay();
      }
      return true;
    } catch (err) {
      if (!silent) this.toast(err.message || "Could not sync Codex.");
      return false;
    }
  }

  async fetchGlobalLeaderboard() {
    const reqId = ++this.globalLbReqId;
    this.globalLbLoading = true;
    this.globalLbError = "";
    if (this.screen === Screen.Leaderboard || this.screen === Screen.Results) {
      this.render();
    }
    try {
      const data = await this.api.getLeaderboard({
        difficulty: this.lbDifficulty,
        hero: this.lbHeroFilter,
        ending: this.lbEndingFilter,
        page: this.lbPage + 1,
        pageSize: 10,
        mine: this.lbMineOnly && this.api.isSignedIn()
      });
      if (reqId !== this.globalLbReqId) return;
      this.globalLbData = data;
      this.lbPage = Math.max(0, (data.page || 1) - 1);
      this.globalLbLoading = false;
      this.globalLbError = "";
    } catch (err) {
      if (reqId !== this.globalLbReqId) return;
      this.globalLbLoading = false;
      this.globalLbError = err.message || "Unable to load global leaderboard.";
    }
    if (this.screen === Screen.Leaderboard || this.screen === Screen.Results) {
      this.render();
    }
  }

  openAccountModal(defaultTab = "login") {
    this.audio.playSfx("click");
    this.accountTab = defaultTab;
    this.accountError = "";
    this.accountBusy = false;
    this.overlay = Overlay.Account;
    this.renderOverlay();
  }

  renderAccountButtonHtml() {
    const user = this.api.getUser();
    if (user) {
      return `<button class="btn btn-sm btn-account-signed" id="btn-account" title="Signed in — click to manage account and cloud sync"><span class="account-dot"></span><span>${escHtml(user.username)}</span></button>`;
    }
    return `<button class="btn btn-sm btn-primary" id="btn-account">Sign In / Account</button>`;
  }

  startIntroSequence() {
    this.audio.unlock();
    this.introPlaying = true;
    this.audio.playMusic("DD.ogg");
    this.audio.playIntroNarration(() => {
      if (this.screen === Screen.Intro) this.enter(Screen.Menu);
    });
    this.render();
  }

  resolveChosenSeed() {
    if (this.dailyRunSelected) return dailyRunSeedForToday();
    const trimmed = this.seedInput.trim();
    if (trimmed) {
      try { return parseSeed(trimmed); } catch { /* fall through */ }
    }
    return randomSeed();
  }

  startNewDescent(biome) {
    this.activeSeed = this.resolveChosenSeed();
    this.store.clearSuspend();
    this.suspendPreview = null;
    this.run.start(this.selectedHero, biome, this.activeSeed, this.selectedDifficulty);
    this.playTimeSeconds = 0;
    this.combatBadges = [];
    this.choosingSecondFloor = false;
    this.enter(Screen.Game);
    this.audio.playMusic(this.run.musicFile());
  }

  resumeSuspendedRun() {
    const saved = this.store.loadSuspendPreview();
    if (!saved || !saved.state) {
      this.toast("No valid suspended run found.");
      return;
    }
    this.run.deserializeState(saved.state);
    this.selectedHero = this.run.player.hero;
    this.selectedDifficulty = this.run.difficulty;
    this.activeSeed = this.run.seed;
    this.playTimeSeconds = saved.playTimeSeconds || 0;
    this.store.clearSuspend();
    this.suspendPreview = null;
    this.enter(Screen.Game);
    this.audio.playMusic(this.run.musicFile());
  }

  handleCombatEffects(exchange, abilityLabel = null) {
    if (!exchange || !exchange.valid) return;
    const badges = [];
    if (exchange.critical) {
      this.audio.playSfx("crit");
      badges.push({ text: `CRIT! -${formatNum(exchange.damageDealt)}`, color: "#dfbb77" });
      this.triggerStageEffect("rgba(223, 187, 119, 0.45)", true);
    } else if (exchange.action === CombatAction.ClassAbility) {
      this.audio.playSfx("ability");
      const label = abilityLabel || combatAbilityName(this.run.player.hero);
      badges.push({ text: `${label} -${formatNum(exchange.damageDealt)}`, color: "#72cae8" });
      this.triggerStageEffect("rgba(114, 202, 232, 0.38)", true);
    } else if (exchange.guarded) {
      this.audio.playSfx("guard");
      badges.push({ text: `GUARDED -${formatNum(exchange.damageDealt)}`, color: "#72cae8" });
    } else {
      this.audio.playSfx("strike");
      badges.push({ text: `-${formatNum(exchange.damageDealt)} HP`, color: "#e2e7e9" });
    }

    if (exchange.counterattacked && exchange.counterDamage > 0) {
      this.audio.playSfx("hit");
      badges.push({ text: `Counter -${formatNum(exchange.counterDamage)} HP`, color: "#ea6c6c" });
      if (!exchange.guarded) this.triggerStageEffect("rgba(234, 108, 108, 0.42)", true);
    } else if (exchange.action === CombatAction.ClassAbility && !exchange.enemyDefeated &&
               (abilityLabel || this.run.player.hero === Hero.Warrior || this.run.player.hero === Hero.Rogue)) {
      badges.push({ text: "Counter Negated!", color: "#70d690" });
    }
    if (exchange.enemyHealed > 0) {
      badges.push({ text: `Enemy +${formatNum(exchange.enemyHealed)} HP`, color: "#ea6c6c" });
    }
    if (exchange.strikesDrained > 0) {
      badges.push({ text: "-1 Strike Drained!", color: "#eab354" });
    }
    if (exchange.enemyDefeated) {
      this.audio.playSfx("loot");
    } else if (exchange.playerDefeated) {
      this.audio.playSfx("defeat");
    }

    this.combatBadges = badges;
    setTimeout(() => {
      this.combatBadges = [];
      const container = document.getElementById("floating-badges");
      if (container) container.innerHTML = "";
    }, 1150);
  }

  triggerStageEffect(flashColor, shake) {
    this.stageFlashColor = flashColor;
    this.stageShake = shake;
    setTimeout(() => {
      this.stageFlashColor = "";
      this.stageShake = false;
    }, 260);
  }

  handleRoomChoice(idx) {
    const prevPhase = this.run.phase;
    if (prevPhase === Phase.Riddle) {
      const beforeCorrect = this.run.player.riddlesCorrect;
      this.run.answer(idx);
      const solved = this.run.player.riddlesCorrect > beforeCorrect;
      this.audio.playSfx(solved ? "riddleOk" : "riddleBad");
      if (this.run.phase === Phase.Defeat) this.audio.playSfx("defeat");
    } else if (prevPhase === Phase.Combat) {
      if (!this.run.combatChoiceAvailable(idx)) return;
      this.run.choose(idx);
      this.handleCombatEffects(this.run.lastCombatExchange);
      if (!this.run.combatChoiceAvailable(this.hoveredCombatAction)) {
        this.hoveredCombatAction = CombatAction.StandardStrike;
      }
    } else {
      this.audio.playSfx("click");
      this.run.choose(idx);
    }

    this.page = 0;
    if (!this.run.inProgress()) {
      this.recordCodexIfNeeded();
      if (this.run.phase === Phase.Ending) {
        this.audio.playMusic(this.run.musicFile());
      }
      this.store.clearSuspend();
      this.suspendPreview = null;
    }
    this.render();
  }

  handleAdvance() {
    const prevPhase = this.run.phase;
    const prevRoom = this.run.roomNumber();
    this.run.advance();
    if (prevPhase === Phase.Loot) this.audio.playSfx("loot");
    else this.audio.playSfx("click");
    if (this.run.roomNumber() !== prevRoom) this.triggerScreenFade();
    this.page = 0;
    this.render();
  }

  // Riddle Gauntlet
  startGauntlet() {
    const deck = [];
    for (let tier = 0; tier < 3; tier++) {
      const copy = this.assets.sourceRiddles[tier].map((r) => ({
        question: r.question,
        answers: [...r.answers],
        correct: r.correct
      }));
      this.gauntletRng.shuffle(copy);
      for (const r of copy) {
        const correctText = r.answers[r.correct];
        this.gauntletRng.shuffle(r.answers);
        r.correct = r.answers.indexOf(correctText);
        deck.push({ tier, riddle: r });
      }
    }
    this.gauntlet = {
      active: true,
      finished: false,
      showingFeedback: false,
      feedbackText: "",
      feedbackOk: false,
      lives: 3,
      score: 0,
      streak: 0,
      bestStreak: 0,
      index: 0,
      deck,
      timeLimit: 20,
      timeLeft: 20
    };
    this.setupGauntletTimer();
  }

  setupGauntletTimer() {
    if (!this.gauntlet || this.gauntlet.index >= this.gauntlet.deck.length || this.gauntlet.lives <= 0) {
      if (this.gauntlet) {
        this.gauntlet.finished = true;
        this.gauntlet.showingFeedback = false;
        this.codex = this.store.recordGauntletResult(this.gauntlet.score, this.gauntlet.bestStreak);
      }
      return;
    }
    const tier = this.gauntlet.deck[this.gauntlet.index].tier;
    this.gauntlet.timeLimit = tier === 0 ? 20 : (tier === 1 ? 16 : 13);
    this.gauntlet.timeLeft = this.gauntlet.timeLimit;
    this.gauntlet.showingFeedback = false;
  }

  answerGauntlet(choiceIdx, timeout = false) {
    if (!this.gauntlet || this.gauntlet.finished || this.gauntlet.showingFeedback) return;
    const item = this.gauntlet.deck[this.gauntlet.index];
    const ok = !timeout && choiceIdx === item.riddle.correct;
    this.gauntlet.showingFeedback = true;
    this.gauntlet.feedbackOk = ok;
    if (ok) {
      this.gauntlet.score++;
      this.gauntlet.streak++;
      this.gauntlet.bestStreak = Math.max(this.gauntlet.bestStreak, this.gauntlet.streak);
      this.gauntlet.feedbackText = `Correct! Streak x${this.gauntlet.streak}.`;
      this.audio.playSfx("riddleOk");
    } else {
      this.gauntlet.lives--;
      this.gauntlet.streak = 0;
      const reason = timeout ? "Time expired." : "Incorrect.";
      this.gauntlet.feedbackText = `${reason} The answer was: ${item.riddle.answers[item.riddle.correct]}.`;
      this.audio.playSfx("riddleBad");
    }
    this.render();
  }

  nextGauntlet() {
    if (!this.gauntlet) return;
    this.gauntlet.index++;
    this.setupGauntletTimer();
    this.render();
  }

  // Main Animation & Countdown Loop
  startLoop() {
    this.lastFrameTs = performance.now();
    const loop = (ts) => {
      const dt = Math.min(0.1, Math.max(0, (ts - this.lastFrameTs) / 1000));
      this.lastFrameTs = ts;
      this.update(dt);
      this.tickHandle = requestAnimationFrame(loop);
    };
    this.tickHandle = requestAnimationFrame(loop);
  }

  update(dt) {
    if (this.screen === Screen.Intro) {
      this.introTimer += dt;
      const targetFrame = (Math.floor(this.introTimer * 10) % 11) + 1;
      if (targetFrame !== this.introFrame) {
        this.introFrame = targetFrame;
        const bg = document.getElementById("intro-bg-img");
        if (bg) bg.src = this.assets.assetUrl(`candle/candle${this.introFrame}.jpg`);
      }
      return;
    }

    if (this.screen === Screen.Menu) {
      this.menuTimer += dt;
      const targetFrame = (Math.floor(this.menuTimer * 12) % 91) + 1;
      if (targetFrame !== this.menuFrame) {
        const cached = this.menuFrames[targetFrame - 1];
        if (cached && cached.complete && cached.naturalWidth > 0) {
          this.menuFrame = targetFrame;
          const bg = document.getElementById("menu-bg-img");
          if (bg) bg.src = cached.src;
        }
      }
      return;
    }

    if (this.screen === Screen.Game && this.overlay === Overlay.None && this.run.inProgress()) {
      this.playTimeSeconds += dt;
      if (this.run.phase === Phase.Riddle) {
        const beforePhase = this.run.phase;
        this.run.tick(dt);
        const sec = Math.ceil(this.run.timeLeft);
        if (sec > 0 && sec <= 5 && sec !== this.lastTimerSec) {
          this.lastTimerSec = sec;
          this.audio.playSfx("tick");
        }
        if (this.run.phase !== beforePhase) {
          this.audio.playSfx(this.run.phase === Phase.Defeat ? "defeat" : "riddleBad");
          if (!this.run.inProgress()) this.recordCodexIfNeeded();
          this.render();
        } else {
          this.updateRiddleTimerDom(this.run.timeLeft, this.run.timeLimit);
        }
      }
    }

    if (this.screen === Screen.Prequel && this.overlay === Overlay.None && this.prequel.phase === PrequelPhase.Parley) {
      const beforePhase = this.prequel.phase;
      this.prequel.tickParley(dt);
      const sec = Math.ceil(this.prequel.parley.timeLeft);
      if (sec > 0 && sec <= 5 && sec !== this.lastTimerSec) {
        this.lastTimerSec = sec;
        this.audio.playSfx("tick");
      }
      if (this.prequel.phase !== beforePhase) {
        this.audio.playSfx("riddleBad");
        this.audio.playMusic(this.prequel.musicFile());
        this.render();
      } else {
        this.updateRiddleTimerDom(this.prequel.parley.timeLeft, this.prequel.parley.timeLimit);
      }
    }

    if (this.screen === Screen.Gauntlet && this.overlay === Overlay.None && this.gauntlet && !this.gauntlet.finished && !this.gauntlet.showingFeedback) {
      this.gauntlet.timeLeft = Math.max(0, this.gauntlet.timeLeft - dt);
      const sec = Math.ceil(this.gauntlet.timeLeft);
      if (sec > 0 && sec <= 5 && sec !== this.lastTimerSec) {
        this.lastTimerSec = sec;
        this.audio.playSfx("tick");
      }
      if (this.gauntlet.timeLeft <= 0) {
        this.answerGauntlet(-1, true);
      } else {
        this.updateRiddleTimerDom(this.gauntlet.timeLeft, this.gauntlet.timeLimit);
      }
    }
  }

  updateRiddleTimerDom(left, limit) {
    const label = document.getElementById("riddle-timer-label");
    const fill = document.getElementById("riddle-timer-fill");
    if (label) label.textContent = `${left.toFixed(1)}s remaining`;
    if (fill) {
      const pct = limit > 0 ? Math.max(0, Math.min(100, (left / limit) * 100)) : 0;
      fill.style.width = `${pct}%`;
      fill.style.background = left <= 5 ? "var(--negative)" : "var(--gold)";
    }
  }

  toggleMute() {
    this.settings.muted = !this.settings.muted;
    this.settings = this.store.saveSettings(this.settings);
    this.audio.applySettings(this.settings);
    this.render();
  }

  toggleFullscreen() {
    if (!document.fullscreenElement) {
      document.documentElement.requestFullscreen?.().catch(() => {});
    } else {
      document.exitFullscreen?.().catch(() => {});
    }
  }

  // Event Binding
  bindGlobalEvents() {
    window.addEventListener("pointerdown", () => this.audio.unlock(), { once: false, passive: true });
    window.addEventListener("keydown", (e) => {
      this.audio.unlock();
      if (e.target && (e.target.tagName === "INPUT" || e.target.tagName === "TEXTAREA")) {
        if (e.key === "Escape") e.target.blur();
        return;
      }

      if (e.key === "v" || e.key === "V") {
        this.toggleMute();
        return;
      }

      if (this.overlay !== Overlay.None) {
        if (e.key === "Escape") {
          this.overlay = Overlay.None;
          this.renderOverlay();
        }
        return;
      }

      if (this.screen === Screen.Intro) {
        if (e.key === " " || e.key === "Enter" || e.key === "Escape") {
          e.preventDefault();
          if (this.introPlaying) this.enter(Screen.Menu);
          else this.startIntroSequence();
        }
        return;
      }

      if (this.screen === Screen.Prequel) {
        const ph = this.prequel.phase;
        if (e.key === "Escape" && ph !== PrequelPhase.Parley) {
          this.audio.playSfx("click");
          this.enter(Screen.Menu);
          return;
        }
        if (["1", "2", "3", "4"].includes(e.key)) {
          const idx = Number(e.key) - 1;
          if (ph === PrequelPhase.Scene || ph === PrequelPhase.Parley || ph === PrequelPhase.Combat) {
            this.handlePrequelChoice(idx);
          }
          return;
        }
        if (e.key === "Enter" || e.key === " ") {
          if (ph === PrequelPhase.ActIntro || ph === PrequelPhase.Outcome || ph === PrequelPhase.ActComplete) {
            e.preventDefault();
            this.handlePrequelAdvance();
          } else if (ph === PrequelPhase.Defeat) {
            e.preventDefault();
            this.handlePrequelRetryAct();
          }
          return;
        }
        return;
      }

      if (this.screen === Screen.Game) {
        const inRiddle = this.run.phase === Phase.Riddle;
        if (e.key === "Escape") {
          if (this.run.phase === Phase.Ending) {
            this.enter(Screen.Results);
          } else if (!inRiddle && this.run.inProgress()) {
            this.overlay = Overlay.Pause;
            this.renderOverlay();
          }
          return;
        }
        if ((e.key === "m" || e.key === "M") && !inRiddle && this.run.inProgress()) {
          this.overlay = Overlay.Map;
          this.renderOverlay();
          return;
        }
        if ((e.key === "i" || e.key === "I") && !inRiddle) {
          this.overlay = Overlay.Stats;
          this.renderOverlay();
          return;
        }
        if (["1", "2", "3", "4"].includes(e.key)) {
          const idx = Number(e.key) - 1;
          if (this.run.phase === Phase.Room || this.run.phase === Phase.Combat || this.run.phase === Phase.Riddle) {
            this.handleRoomChoice(idx);
          }
          return;
        }
        if (e.key === "Enter" || e.key === " ") {
          if (this.run.phase === Phase.Outcome || this.run.phase === Phase.RiddleOutcome || this.run.phase === Phase.Loot) {
            e.preventDefault();
            this.handleAdvance();
          } else if (this.run.phase === Phase.Complete) {
            e.preventDefault();
            this.descendFromComplete();
          } else if (this.run.phase === Phase.Ending || this.run.phase === Phase.Defeat) {
            e.preventDefault();
            this.enter(Screen.Results);
          }
          return;
        }
        if (e.key === "ArrowLeft") this.changePage(-1);
        if (e.key === "ArrowRight") this.changePage(1);
        return;
      }

      if (this.screen === Screen.Gauntlet && this.gauntlet) {
        if (e.key === "Escape") {
          if (this.gauntlet.score > 0 || this.gauntlet.bestStreak > 0) {
            this.codex = this.store.recordGauntletResult(this.gauntlet.score, this.gauntlet.bestStreak);
          }
          this.enter(Screen.Menu);
          return;
        }
        if (["1", "2", "3"].includes(e.key) && !this.gauntlet.finished && !this.gauntlet.showingFeedback) {
          this.answerGauntlet(Number(e.key) - 1, false);
          return;
        }
        if ((e.key === "Enter" || e.key === " ") && this.gauntlet.showingFeedback) {
          e.preventDefault();
          this.nextGauntlet();
        }
        return;
      }

      if (this.screen === Screen.Heroes && ["1", "2", "3", "4"].includes(e.key)) {
        this.selectedHero = Number(e.key) - 1;
        this.choosingSecondFloor = false;
        this.audio.playSfx("click");
        this.enter(Screen.Difficulty);
        return;
      }

      if (this.screen === Screen.Difficulty && ["1", "2", "3", "4"].includes(e.key)) {
        this.selectedDifficulty = Number(e.key) - 1;
        this.audio.playSfx("click");
        this.enter(Screen.Biomes);
        return;
      }

      if (this.screen === Screen.Biomes && ["1", "2"].includes(e.key)) {
        const idx = Number(e.key) - 1;
        const biome = idx + (this.choosingSecondFloor ? 2 : 0);
        this.selectBiomeDoor(biome);
        return;
      }

      if (e.key === "ArrowLeft") this.changePage(-1);
      if (e.key === "ArrowRight") this.changePage(1);
      if (e.key === "Escape") {
        if (this.screen === Screen.Lore) this.enter(this.loreReturn);
        else if (this.screen === Screen.Codex || this.screen === Screen.Leaderboard || this.screen === Screen.Introduction || this.screen === Screen.Heroes) {
          this.enter(Screen.Menu);
        } else if (this.screen === Screen.Difficulty) {
          this.enter(Screen.Heroes);
        } else if (this.screen === Screen.Biomes) {
          this.enter(this.choosingSecondFloor ? Screen.Game : Screen.Difficulty);
        }
      }
    });
  }

  changePage(delta) {
    const pages = this.currentNarrativePages();
    if (!pages || pages.length <= 1) return;
    const next = Math.max(0, Math.min(pages.length - 1, this.page + delta));
    if (next !== this.page) {
      this.page = next;
      this.audio.playSfx("click");
      this.render();
    }
  }

  currentNarrativePages() {
    if (this.screen === Screen.Introduction) {
      return paginateText(this.assets.getText("textfiles/introduction/introduction.txt"), 950);
    }
    if (this.screen === Screen.Lore) {
      return paginateText(this.assets.getText(`textfiles/charIntro/${heroName(this.loreHero)}Intro.txt`), 1100);
    }
    if (this.screen === Screen.Codex && this.codexTab === 1 && isEndingUnlocked(this.codex, this.codexEnding)) {
      return paginateText(this.assets.getText(endingStoryFile(this.codexEnding)), 950);
    }
    if (this.screen === Screen.Game) {
      if (this.run.phase === Phase.Ending) return paginateText(this.run.message, 980);
      if (this.run.phase === Phase.Room) return paginateText(this.assets.getText(this.run.storyFile()), 920);
      return paginateText(this.run.message, 920);
    }
    return null;
  }

  descendFromComplete() {
    this.audio.playSfx("click");
    if (this.run.floorNumber === 1) {
      this.choosingSecondFloor = true;
      this.enter(Screen.Biomes);
    } else if (this.run.descend(Biome.Lava)) {
      this.enter(Screen.Game);
      this.audio.playMusic(this.run.musicFile());
    }
  }

  selectBiomeDoor(biome) {
    this.audio.playSfx("click");
    if (this.choosingSecondFloor) {
      if (this.run.descend(biome)) {
        this.choosingSecondFloor = false;
        this.enter(Screen.Game);
        this.audio.playMusic(this.run.musicFile());
      }
    } else {
      this.startNewDescent(biome);
    }
  }

  // Ashes of Falkreath — Prequel Campaign Handlers
  startPrequel() {
    this.prequel.start();
    this.lastTimerSec = -1;
    this.enter(Screen.Prequel);
  }

  handlePrequelAdvance() {
    const prevAct = this.prequel.act;
    this.prequel.advance();
    this.combatBadges = [];
    this.stageFlashColor = "";
    this.stageShake = false;
    this.hoveredCombatAction = CombatAction.StandardStrike;
    this.lastTimerSec = -1;
    this.audio.playSfx("click");
    if (this.prequel.act !== prevAct || this.prequel.phase === PrequelPhase.Epilogue) {
      this.triggerScreenFade();
    }
    this.audio.playMusic(this.prequel.musicFile());
    this.recordPrequelIfNeeded();
    this.render();
  }

  handlePrequelChoice(idx) {
    const ph = this.prequel.phase;
    if (ph === PrequelPhase.Scene) {
      if (idx < 0 || idx >= this.prequel.choices.length) return;
      this.audio.playSfx("click");
      this.prequel.choose(idx);
      this.combatBadges = [];
      this.hoveredCombatAction = CombatAction.StandardStrike;
      this.lastTimerSec = -1;
    } else if (ph === PrequelPhase.Parley) {
      if (!this.prequel.parleyOptionAvailable(idx)) return;
      const ok = this.prequel.answerParley(idx);
      this.audio.playSfx(ok ? "riddleOk" : "riddleBad");
      this.lastTimerSec = -1;
    } else if (ph === PrequelPhase.Combat) {
      if (!this.prequel.combatChoiceAvailable(idx)) return;
      const ex = this.prequel.attack(idx);
      if (ex) this.handleCombatEffects(ex, this.prequel.character.abilityName);
      if (!this.prequel.combatChoiceAvailable(this.hoveredCombatAction)) {
        this.hoveredCombatAction = CombatAction.StandardStrike;
      }
    } else {
      return;
    }
    this.audio.playMusic(this.prequel.musicFile());
    this.recordPrequelIfNeeded();
    this.render();
  }

  handlePrequelRetryAct() {
    this.audio.playSfx("click");
    this.prequel.restartAct();
    this.combatBadges = [];
    this.stageFlashColor = "";
    this.stageShake = false;
    this.hoveredCombatAction = CombatAction.StandardStrike;
    this.lastTimerSec = -1;
    this.triggerScreenFade();
    this.audio.playMusic(this.prequel.musicFile());
    this.render();
  }

  handlePrequelRestart() {
    this.audio.playSfx("click");
    this.prequel.start();
    this.combatBadges = [];
    this.stageFlashColor = "";
    this.stageShake = false;
    this.hoveredCombatAction = CombatAction.StandardStrike;
    this.lastTimerSec = -1;
    this.triggerScreenFade();
    this.audio.playMusic(this.prequel.musicFile());
    this.render();
  }

  // Rendering Router
  render() {
    switch (this.screen) {
      case Screen.Intro: this.renderIntro(); break;
      case Screen.Menu: this.renderMenu(); break;
      case Screen.Introduction: this.renderIntroduction(); break;
      case Screen.Heroes: this.renderHeroes(); break;
      case Screen.Lore: this.renderLore(); break;
      case Screen.Difficulty: this.renderDifficulty(); break;
      case Screen.Biomes: this.renderBiomes(); break;
      case Screen.Game:
        if (this.run.phase === Phase.Ending) this.renderEnding();
        else this.renderGame();
        break;
      case Screen.Results:
      case Screen.Leaderboard:
        this.renderResultsOrLeaderboard();
        break;
      case Screen.Codex: this.renderCodex(); break;
      case Screen.Gauntlet: this.renderGauntlet(); break;
      case Screen.Prequel: this.renderPrequel(); break;
    }
    this.renderOverlay();
  }

  renderIntro() {
    const imgPath = `candle/candle${this.introFrame}.jpg`;
    this.root.innerHTML = `
      <main class="intro-screen" id="intro-screen">
        <img id="intro-bg-img" class="intro-bg" src="${this.assets.assetUrl(imgPath)}" alt="Flickering dungeon candle">
        <div class="intro-vignette"></div>
        <section class="intro-content">
          <h1 class="brand-title">DUNGEON DESCENT</h1>
          <p class="intro-tagline">${this.introPlaying
            ? "Listen to the prologue of Aethoria — or press Space / click anywhere to enter the Main Menu."
            : "Princess Sophie has vanished beneath the mountains of Aethoria. Click anywhere or choose below to begin."}</p>
          <div class="intro-actions">
            ${this.introPlaying
              ? `<button class="btn btn-primary" id="btn-skip-intro">Skip Intro to Main Menu [Space]</button>`
              : `<button class="btn btn-primary" id="btn-play-intro">Enter with Prologue &amp; Audio</button>
                 <button class="btn" id="btn-skip-intro">Skip Straight to Menu</button>`}
          </div>
        </section>
      </main>`;

    const introScreen = document.getElementById("intro-screen");
    if (introScreen) {
      introScreen.addEventListener("click", () => {
        if (this.introPlaying) {
          this.audio.playSfx("click");
          this.enter(Screen.Menu);
        } else {
          this.startIntroSequence();
        }
      });
    }
    const playBtn = document.getElementById("btn-play-intro");
    if (playBtn) playBtn.addEventListener("click", (e) => { e.stopPropagation(); this.startIntroSequence(); });
    const skipBtn = document.getElementById("btn-skip-intro");
    if (skipBtn) skipBtn.addEventListener("click", (e) => {
      e.stopPropagation();
      this.audio.unlock();
      this.audio.playSfx("click");
      this.enter(Screen.Menu);
    });
  }

  renderMenu() {
    const sp = this.suspendPreview;
    const user = this.api.getUser();
    const milestones = getCodexMilestones(this.codex);
    const unlockedMs = milestones.filter((m) => m.unlocked).length;
    const prequelOpen = this.prequelAccessible();

    const resumeHtml = sp ? `
      <button class="btn btn-primary span-2 resume-card" id="btn-resume">
        <span class="resume-meta">
          <strong>Resume Suspended Run</strong>
          <small>${escHtml(heroName(sp.hero))} &middot; ${escHtml(difficultyName(sp.difficulty))} &middot; Floor ${sp.floorNumber} (${escHtml(biomeName(sp.biome))}), Room ${sp.roomNumber}/9 &middot; ${formatDuration(sp.playTimeSeconds)}</small>
        </span>
        <span>Continue &rarr;</span>
      </button>` : "";

    const prequelBtnHtml = prequelOpen ? `
      <button class="btn btn-prequel-unlocked span-2" id="btn-prequel">
        <span class="prequel-btn-meta">
          <strong>Ashes of Falkreath &mdash; Prequel Campaign</strong>
          <small>${this.codex.prequelCompleted
            ? "3-Act Chronicle Completed &middot; Replay as Eldred, Malvel &amp; Princess Sophie"
            : "Unlocked! Play the 3-Act prelude as Eldred, Malvel &amp; Princess Sophie"}</small>
        </span>
        <span>Play &rarr;</span>
      </button>` : `
      <button class="btn btn-prequel-locked span-2" id="btn-prequel-locked">
        <span class="prequel-btn-meta">
          <strong>Sealed Chronicle</strong>
          <small>Earn the Perfect Ending on Impossible difficulty to unlock Ashes of Falkreath</small>
        </span>
        <span>Locked</span>
      </button>`;

    this.root.innerHTML = `
      <main class="menu-screen">
        <img id="menu-bg-img" class="menu-hero-bg" src="${this.assets.assetUrl(`images/startscreen/startscreen${this.menuFrame}.jpg`)}" alt="">
        <div class="menu-hero-bg-overlay"></div>
        <div class="menu-layout">
          <header class="menu-top-bar">
            <span class="kicker">Web Edition &middot; dungeondescent.duckdns.org</span>
            <div class="top-header-actions">
              ${this.renderAccountButtonHtml()}
              <button class="btn btn-sm" id="btn-sound">${this.settings.muted ? "Sound off [V]" : "Sound on [V]"}</button>
              <button class="btn btn-sm" id="btn-fullscreen">Fullscreen</button>
            </div>
          </header>

          <section class="menu-center">
            <h1 class="brand-title">DUNGEON DESCENT</h1>
            <p class="menu-lead">A three-floor dark fantasy tactical RPG. Master telegraphed boss intents, 102 timed riddles, and uncover the truth behind Falkreath.</p>
            <div class="menu-grid">
              ${resumeHtml}
              <button class="btn btn-primary span-2" id="btn-start">New Descent</button>
              ${prequelBtnHtml}
              <button class="btn" id="btn-codex">Chronicles of Aethoria (Codex)</button>
              <button class="btn" id="btn-gauntlet">Riddle Gauntlet (102 Riddles)</button>
              <button class="btn" id="btn-leaderboards">Global Leaderboards &amp; Seeds</button>
              <button class="btn" id="btn-lore">Lore of the Heroes</button>
              <button class="btn span-2" id="btn-settings">Audio &amp; Display Settings</button>
            </div>
          </section>

          <footer class="menu-footer">
            <span>Endings unlocked: ${unlockedEndingCount(this.codex)} / 4 &middot; Milestones: ${unlockedMs} / ${milestones.length}</span>
            <span>${user ? `Cloud synced as ${escHtml(user.username)}` : "Playing as Guest &middot; Sign in for Global Leaderboards &amp; Cloud Sync"}</span>
          </footer>
        </div>
      </main>`;

    document.getElementById("btn-account")?.addEventListener("click", () => this.openAccountModal("login"));
    document.getElementById("btn-resume")?.addEventListener("click", () => { this.audio.playSfx("click"); this.resumeSuspendedRun(); });
    document.getElementById("btn-start").addEventListener("click", () => { this.audio.playSfx("click"); this.enter(Screen.Introduction); });
    document.getElementById("btn-prequel")?.addEventListener("click", () => { this.audio.playSfx("click"); this.startPrequel(); });
    document.getElementById("btn-prequel-locked")?.addEventListener("click", () => {
      this.audio.playSfx("riddleBad");
      this.toast("Sealed: Earn the Perfect Ending (9/9 battles, 24/24 riddles, 9/9 rep) on Impossible difficulty to break the seal.");
    });
    document.getElementById("btn-codex").addEventListener("click", () => {
      this.audio.playSfx("click");
      this.codexTab = 0;
      this.codexMilestonePage = 0;
      this.enter(Screen.Codex);
    });
    document.getElementById("btn-gauntlet").addEventListener("click", () => { this.audio.playSfx("click"); this.enter(Screen.Gauntlet); });
    document.getElementById("btn-leaderboards").addEventListener("click", () => { this.audio.playSfx("click"); this.enter(Screen.Leaderboard); });
    document.getElementById("btn-lore").addEventListener("click", () => {
      this.audio.playSfx("click");
      this.loreReturn = Screen.Menu;
      this.loreHero = Hero.Warrior;
      this.enter(Screen.Lore);
    });
    document.getElementById("btn-settings").addEventListener("click", () => {
      this.audio.playSfx("click");
      this.overlay = Overlay.Settings;
      this.renderOverlay();
    });
    document.getElementById("btn-sound").addEventListener("click", () => this.toggleMute());
    document.getElementById("btn-fullscreen").addEventListener("click", () => this.toggleFullscreen());
  }

  renderIntroduction() {
    const fullText = this.assets.getText("textfiles/introduction/introduction.txt").trim();
    this.root.innerHTML = `
      <main class="screen">
        <div class="screen-backdrop" style="background-image:url('${this.assets.assetUrl("images/startscreen.jpg")}')"></div>
        <div class="container container-centered">
          <div class="prologue-shell">
            <header class="top-header" style="margin-bottom:4px;">
              <div class="top-header-left">
                <span class="kicker">Prologue</span>
                <h1 class="page-title">THE CALL OF AETHORIA</h1>
              </div>
              <div class="top-header-actions">
                <button class="btn btn-sm" id="btn-back-menu">Back to menu</button>
              </div>
            </header>

            <div class="prologue-banner">
              <img src="${this.assets.assetUrl("images/startscreen.jpg")}" alt="Aethoria dungeon entrance">
            </div>

            <section class="panel prologue-card">
              <span class="kicker">Chronicle</span>
              <div class="narrative-text">${escHtml(fullText)}</div>
              <div style="display:flex;justify-content:flex-end;">
                <button class="btn btn-primary" id="btn-choose-hero">Choose Your Hero &rarr;</button>
              </div>
            </section>
          </div>
        </div>
      </main>`;

    document.getElementById("btn-back-menu").addEventListener("click", () => { this.audio.playSfx("click"); this.enter(Screen.Menu); });
    document.getElementById("btn-choose-hero").addEventListener("click", () => {
      this.audio.playSfx("click");
      this.choosingSecondFloor = false;
      this.enter(Screen.Heroes);
    });
  }

  renderHeroes() {
    const shortStats = ["STR", "WIS", "OBS", "AGI", "HP", "ACC"];
    const statMax = [10, 10, 10, 10, 45, 10];
    const statColors = ["#e48062", "#92b2ee", "#a9c0d3", "#77c693", "#dfbb77", "#7bc6e0"];

    const cards = [0, 1, 2, 3].map((h) => {
      const p = makePlayer(h);
      const bars = p.stats.map((val, s) => `
        <div class="stat-bar-row">
          <span class="stat-key">${shortStats[s]}</span>
          <div class="progress-track"><div class="progress-fill" style="width:${Math.min(100, (val / statMax[s]) * 100)}%;background:${statColors[s]}"></div></div>
          <span class="stat-val">${formatNum(val)}</span>
        </div>`).join("");
      return `
        <article class="panel hero-card">
          <div class="square-portrait-frame">
            <img src="${this.assets.assetUrl(portraitImage(h))}" alt="${escHtml(heroName(h))}">
          </div>
          <div class="card-body">
            <h2 class="card-title">${escHtml(heroName(h))}</h2>
            <div class="weapon-label">${escHtml(heroWeapon(h))}</div>
            <p class="ability-summary">${escHtml(heroAbilitySummary(h))}</p>
            <div class="stat-bars">${bars}</div>
            <button class="btn btn-primary card-footer-btn" data-hero="${h}">
              <span class="choice-key">${h + 1}</span> Choose ${escHtml(heroName(h))}
            </button>
          </div>
        </article>`;
    }).join("");

    this.root.innerHTML = `
      <main class="screen">
        <div class="screen-backdrop" style="background-image:url('${this.assets.assetUrl("images/startscreen.jpg")}')"></div>
        <div class="container container-centered">
          <header class="top-header">
            <div class="top-header-left">
              <h1 class="page-title">CHOOSE YOUR WEAPON</h1>
              <p class="subtitle">Four paths into the dark. Compare all six starting stats and class abilities.</p>
            </div>
            <div class="top-header-actions">
              <button class="btn btn-sm" id="btn-view-lore">Hero Lore</button>
              <button class="btn btn-sm" id="btn-back-menu">Back to menu</button>
            </div>
          </header>
          <section class="card-grid-4">${cards}</section>
        </div>
      </main>`;

    this.root.querySelectorAll("[data-hero]").forEach((btn) => {
      btn.addEventListener("click", () => {
        this.selectedHero = Number(btn.dataset.hero);
        this.choosingSecondFloor = false;
        this.audio.playSfx("click");
        this.enter(Screen.Difficulty);
      });
    });
    document.getElementById("btn-view-lore").addEventListener("click", () => {
      this.audio.playSfx("click");
      this.loreReturn = Screen.Heroes;
      this.loreHero = this.selectedHero;
      this.enter(Screen.Lore);
    });
    document.getElementById("btn-back-menu").addEventListener("click", () => { this.audio.playSfx("click"); this.enter(Screen.Menu); });
  }

  renderLore() {
    const fullLore = this.assets.getText(`textfiles/charIntro/${heroName(this.loreHero)}Intro.txt`).trim();
    const p = makePlayer(this.loreHero);
    const shortStats = ["STR", "WIS", "OBS", "AGI", "HP", "ACC"];
    const statMax = [10, 10, 10, 10, 45, 10];
    const statColors = ["#e48062", "#92b2ee", "#a9c0d3", "#77c693", "#dfbb77", "#7bc6e0"];

    const tabs = [0, 1, 2, 3].map((h) => `
      <button class="btn btn-tab ${this.loreHero === h ? "active" : ""}" data-lore-hero="${h}">${escHtml(heroName(h))}</button>
    `).join("");

    const statBars = p.stats.map((val, s) => `
      <div class="stat-bar-row">
        <span class="stat-key">${shortStats[s]}</span>
        <div class="progress-track"><div class="progress-fill" style="width:${Math.min(100, (val / statMax[s]) * 100)}%;background:${statColors[s]}"></div></div>
        <span class="stat-val">${formatNum(val)}</span>
      </div>`).join("");

    this.root.innerHTML = `
      <main class="screen">
        <div class="screen-backdrop" style="background-image:url('${this.assets.assetUrl("images/startscreen.jpg")}')"></div>
        <div class="container">
          <header class="top-header">
            <div class="top-header-left">
              <h1 class="page-title">LORE OF THE HEROES</h1>
              <p class="subtitle">Chronicles, starting attributes, and combat arts of the four guild champions.</p>
            </div>
            <div class="top-header-actions">
              <button class="btn btn-sm" id="btn-lore-back">Back [Esc]</button>
            </div>
          </header>

          <div class="tabs-row">${tabs}</div>

          <div class="lore-layout">
            <aside class="panel lore-hero-card">
              <div class="lore-portrait-box">
                <img src="${this.assets.assetUrl(portraitImage(this.loreHero))}" alt="${escHtml(heroName(this.loreHero))}">
              </div>
              <div>
                <h2 class="section-title">${escHtml(heroName(this.loreHero))}</h2>
                <div class="weapon-label">Weapon: ${escHtml(heroWeapon(this.loreHero))}</div>
              </div>
              <div class="stat-bars">${statBars}</div>
              <div class="panel-inset lore-ability-box">
                <strong>${escHtml(combatAbilityName(this.loreHero))}</strong>
                <div>${escHtml(combatAbilityDescription(this.loreHero))}</div>
              </div>
            </aside>

            <section class="panel narrative-panel">
              <span class="kicker">Dossier</span>
              <div class="narrative-text tall">${escHtml(fullLore)}</div>
            </section>
          </div>
        </div>
      </main>`;

    this.root.querySelectorAll("[data-lore-hero]").forEach((btn) => {
      btn.addEventListener("click", () => {
        this.loreHero = Number(btn.dataset.loreHero);
        this.page = 0;
        this.audio.playSfx("click");
        this.render();
      });
    });
    document.getElementById("btn-lore-back").addEventListener("click", () => { this.audio.playSfx("click"); this.enter(this.loreReturn); });
  }

  renderDifficulty() {
    const portraits = [Biome.Ice, Biome.Desert, Biome.Ghost, Biome.Lava];
    const tags = [
      "Original Balance · Malvel 567 HP",
      "Scaled Encounters · Malvel 1,000 HP",
      "Tactical Mastery · Malvel 1,200 HP",
      "Extreme Gauntlet · Malvel 1,450 HP"
    ];
    const descs = [
      "Gentler foes. Room to learn, recover and explore.",
      "Stronger foes and longer battles. Prepare well and keep your health up.",
      "Relentless enemies. Strong growth and careful choices are essential.",
      "An extreme challenge. Even a well-prepared hero may not survive."
    ];

    const cards = [0, 1, 2, 3].map((d) => `
      <article class="panel difficulty-card">
        <div class="square-portrait-frame">
          <img src="${this.assets.assetUrl(bossImage(portraits[d]))}" alt="${escHtml(difficultyName(d))}">
        </div>
        <div class="card-body">
          <h2 class="card-title" style="color:${biomeColor(portraits[d])}">${escHtml(difficultyName(d))}</h2>
          <div class="diff-meta-line">${tags[d]}</div>
          <p class="diff-desc">${escHtml(descs[d])}</p>
          <button class="btn btn-primary card-footer-btn" data-diff="${d}">
            <span class="choice-key">${d + 1}</span> Choose ${escHtml(difficultyName(d))}
          </button>
        </div>
      </article>`).join("");

    this.root.innerHTML = `
      <main class="screen">
        <div class="screen-backdrop" style="background-image:url('${this.assets.assetUrl("images/startscreen.jpg")}')"></div>
        <div class="container container-centered">
          <header class="top-header">
            <div class="top-header-left">
              <h1 class="page-title">CHOOSE YOUR CHALLENGE</h1>
              <p class="subtitle">${escHtml(heroName(this.selectedHero))} &middot; Enemy health and counterattacks scale per tier; riddles and endings remain unchanged.</p>
            </div>
            <div class="top-header-actions">
              <button class="btn btn-sm" id="btn-back-heroes">Change hero</button>
            </div>
          </header>
          <section class="card-grid-4">${cards}</section>
        </div>
      </main>`;

    this.root.querySelectorAll("[data-diff]").forEach((btn) => {
      btn.addEventListener("click", () => {
        this.selectedDifficulty = Number(btn.dataset.diff);
        this.audio.playSfx("click");
        this.enter(Screen.Biomes);
      });
    });
    document.getElementById("btn-back-heroes").addEventListener("click", () => { this.audio.playSfx("click"); this.enter(Screen.Heroes); });
  }

  renderBiomes() {
    const floor = this.choosingSecondFloor ? 2 : 1;
    const descs = [
      "Frozen chambers, a captive warrior, and bitter crystal halls leading down to the stairway.",
      "Forgotten stone halls, tangled living roots, and ancient overgrown ruins waiting below.",
      "Sun-parched sandstone crypts, an injured butler, and forgotten royal tombs beneath the sands.",
      "Haunted corridors, a chilling vision of the past, and gothic vaults shrouded in mist."
    ];
    const guardians = [
      "Guardian: The Yeti — Glacial Slam (2x counterattack)",
      "Guardian: The Jungle Warden — Thorned Carapace (1.6x thorns)",
      "Guardian: Ahmose the Pharaoh — Curse of the Sands (strike drain)",
      "Guardian: Count Dracula — Blood Siphon (lifesteal counter)"
    ];

    const todayStr = dailyRunDateString();
    const todaySeed = dailyRunSeedForToday();
    let seedStatus = "RANDOM SEED (Auto)";
    if (this.dailyRunSelected) {
      seedStatus = `DAILY RUN (${todayStr} -> #${todaySeed})`;
    } else if (this.seedInput.trim()) {
      try { seedStatus = `CUSTOM SEED: ${this.seedInput.trim()} (#${parseSeed(this.seedInput.trim())})`; }
      catch { seedStatus = "RANDOM SEED (Auto)"; }
    }

    const seedBar = this.choosingSecondFloor ? "" : `
      <section class="panel seed-bar">
        <div class="seed-bar-left">
          <span class="kicker">Run Seed</span>
          <input id="seed-input" class="seed-input" type="text" maxlength="24" placeholder="Custom seed (optional)" value="${escHtml(this.seedInput)}">
          <span style="font-size:0.86rem;color:var(--muted);" id="seed-status">${escHtml(seedStatus)}</span>
        </div>
        <div class="seed-bar-actions">
          <button class="btn btn-sm btn-tab ${this.dailyRunSelected ? "active" : ""}" id="btn-seed-daily">Daily Run</button>
          <button class="btn btn-sm" id="btn-seed-clear" ${(!this.seedInput && !this.dailyRunSelected) ? "disabled" : ""}>Random Seed</button>
        </div>
      </section>`;

    const cards = [0, 1].map((i) => {
      const b = (floor - 1) * 2 + i;
      const imgUrl = this.assets.assetUrl(bossImage(b));
      return `
        <article class="panel biome-card">
          <div class="biome-art-square">
            <img src="${imgUrl}" alt="${escHtml(biomeName(b))}">
          </div>
          <div class="biome-info">
            <div>
              <span class="kicker">Floor ${floor} Path</span>
              <h2 class="section-title" style="color:${biomeColor(b)};margin-top:4px;">${escHtml(biomeName(b))}</h2>
              <p class="biome-desc">${escHtml(descs[b])}</p>
              <div class="panel-inset biome-guardian-note">${escHtml(guardians[b])}</div>
            </div>
            <button class="btn btn-primary card-footer-btn" data-biome="${b}">
              <span class="choice-key">${i + 1}</span> Enter the ${escHtml(biomeKey(b))} door
            </button>
          </div>
        </article>`;
    }).join("");

    this.root.innerHTML = `
      <main class="screen">
        <div class="screen-backdrop" style="background-image:url('${this.assets.assetUrl("images/startscreen.jpg")}')"></div>
        <div class="container container-centered">
          <header class="top-header">
            <div class="top-header-left">
              <h1 class="page-title">${this.choosingSecondFloor ? "DEEPER INTO THE DUNGEON" : "TWO DOORS. ONE DESCENT."}</h1>
              <p class="subtitle">${this.choosingSecondFloor
                ? "The stairway leads deeper. Choose your next path."
                : `${escHtml(heroName(this.selectedHero))} / ${escHtml(difficultyName(this.selectedDifficulty))}. Which door will you open?`}</p>
            </div>
            <div class="top-header-actions">
              <button class="btn btn-sm" id="btn-biomes-back">${this.choosingSecondFloor ? "Back to stairway" : "Change difficulty"}</button>
            </div>
          </header>
          ${seedBar}
          <section class="card-grid-2">${cards}</section>
        </div>
      </main>`;

    const inputEl = document.getElementById("seed-input");
    if (inputEl) {
      inputEl.addEventListener("input", (e) => {
        this.seedInput = e.target.value;
        this.dailyRunSelected = false;
        const status = document.getElementById("seed-status");
        const clearBtn = document.getElementById("btn-seed-clear");
        const dailyBtn = document.getElementById("btn-seed-daily");
        if (dailyBtn) dailyBtn.classList.remove("active");
        if (clearBtn) clearBtn.disabled = !this.seedInput.trim();
        if (status) {
          if (this.seedInput.trim()) {
            status.textContent = `CUSTOM SEED: ${this.seedInput.trim()} (#${parseSeed(this.seedInput.trim())})`;
          } else {
            status.textContent = "RANDOM SEED (Auto)";
          }
        }
      });
    }
    document.getElementById("btn-seed-daily")?.addEventListener("click", () => {
      this.audio.playSfx("click");
      this.dailyRunSelected = !this.dailyRunSelected;
      if (this.dailyRunSelected) this.seedInput = "";
      this.render();
    });
    document.getElementById("btn-seed-clear")?.addEventListener("click", () => {
      this.audio.playSfx("click");
      this.seedInput = "";
      this.dailyRunSelected = false;
      this.render();
    });
    this.root.querySelectorAll("[data-biome]").forEach((btn) => {
      btn.addEventListener("click", () => this.selectBiomeDoor(Number(btn.dataset.biome)));
    });
    document.getElementById("btn-biomes-back").addEventListener("click", () => {
      this.audio.playSfx("click");
      if (this.choosingSecondFloor) {
        this.choosingSecondFloor = false;
        this.enter(Screen.Game);
      } else {
        this.enter(Screen.Difficulty);
      }
    });
  }

  renderGame() {
    const run = this.run;
    const p = run.player;
    const inRiddle = run.phase === Phase.Riddle;
    const accent = biomeColor(run.biome);
    const relImg = run.image();
    const imgUrl = this.assets.assetUrl(relImg);
    const aspectCls = imageAspectClass(relImg);

    // 9-room tracker
    const pips = Array.from({ length: 9 }, (_, i) => {
      const cls = i + 1 < run.roomNumber() ? "done" : (i + 1 === run.roomNumber() ? "current" : "");
      return `<span class="room-pip ${cls}">${i + 1}</span>`;
    }).join("");

    // Combat Enemy Bar + Boss Intent cleanly BELOW the chamber artwork (zero overlap over monster art!)
    let combatStatusHtml = "";
    if (run.phase === Phase.Combat) {
      const hpPct = Math.max(0, Math.min(100, (run.enemyHealth / run.enemyMaxHealth()) * 100));
      const intent = run.bossIntent();
      const intentHtml = intent.active ? `
        <div class="boss-intent-banner ${intent.telegraphed ? "telegraphed" : ""}">
          <strong>${intent.telegraphed ? "WARNING — " : "BOSS INTENT: "}${escHtml(intent.name)}</strong>
          <span>${escHtml(intent.detail)}</span>
        </div>` : "";
      combatStatusHtml = `
        <div class="combat-status-stack">
          <div class="enemy-bar-card">
            <div class="enemy-bar-top">
              <strong>${escHtml(run.enemyName())} &middot; HP ${formatNum(run.enemyHealth)} / ${formatNum(run.enemyMaxHealth())}</strong>
              <span class="strikes-pill">Strikes left: ${run.strikesLeft}</span>
            </div>
            <div class="progress-track"><div class="progress-fill" style="width:${hpPct}%;background:var(--negative)"></div></div>
          </div>
          ${intentHtml}
        </div>`;
    }

    const badgesHtml = this.combatBadges.map((b) =>
      `<div class="combat-badge" style="color:${b.color}">${escHtml(b.text)}</div>`
    ).join("");

    // Compact horizontal Hero Stat Strip
    const statOrder = [
      { id: Stat.Strength, short: "STR" },
      { id: Stat.Wisdom, short: "WIS" },
      { id: Stat.Observation, short: "OBS" },
      { id: Stat.Agility, short: "AGI" },
      { id: Stat.Accuracy, short: "ACC" }
    ];
    const hpDelta = run.lastStatDeltas[Stat.Health];
    const hpDeltaHtml = Math.abs(hpDelta) > 0.001
      ? `<span class="${hpDelta > 0 ? "delta-pos" : "delta-neg"}">(${hpDelta > 0 ? "+" : ""}${formatNum(hpDelta)})</span>` : "";
    const repDeltaHtml = run.lastReputationDelta !== 0
      ? `<span class="${run.lastReputationDelta > 0 ? "delta-pos" : "delta-neg"}">(${run.lastReputationDelta > 0 ? "+" : ""}${run.lastReputationDelta})</span>` : "";

    const statChipsHtml = statOrder.map(({ id, short }) => {
      const d = run.lastStatDeltas[id];
      const dHtml = Math.abs(d) > 0.001
        ? `<span class="${d > 0 ? "delta-pos" : "delta-neg"}">${d > 0 ? "+" : ""}${formatNum(d)}</span>` : "";
      return `
        <div class="panel-inset stat-chip" title="${escHtml(statTooltip(id))}">
          <span class="stat-chip-label">${short}</span>
          <span class="stat-chip-val"><span>${formatNum(p.stats[id])}</span>${dHtml}</span>
        </div>`;
    }).join("");

    // Choice deck content by phase
    let deckTitle = "YOUR CHOICE";
    let deckBody = "";
    if (run.phase === Phase.Room) {
      const choices = run.choices();
      deckBody = `<div class="choice-list">${choices.map((label, idx) => `
        <button class="btn btn-choice" data-choice="${idx}">
          <span class="choice-key">${idx + 1}</span>
          <span>${escHtml(label)}</span>
        </button>`).join("")}</div>`;
    } else if (run.phase === Phase.Combat) {
      deckTitle = "COMBAT TACTICS";
      const choices = run.choices();
      const preview = run.previewCombatAction(this.hoveredCombatAction);
      deckBody = `
        <div class="choice-list">${choices.map((label, idx) => `
          <button class="btn btn-choice ${this.hoveredCombatAction === idx ? "btn-primary" : ""}" data-choice="${idx}" data-combat-idx="${idx}" ${run.combatChoiceAvailable(idx) ? "" : "disabled"}>
            <span class="choice-key">${idx + 1}</span>
            <span>${escHtml(label)}</span>
          </button>`).join("")}</div>
        <div class="panel-inset tactical-preview" id="tactical-preview">
          <div class="tactical-preview-head">
            <span>${escHtml(preview.title)}</span>
            <span>Cost: ${preview.strikeCost} strike${preview.strikeCost > 1 ? "s" : ""}</span>
          </div>
          <div class="tactical-preview-metrics">
            <span>Dmg: <strong>${formatNum(preview.minDamage)}</strong> (Crit ${formatNum(preview.critDamage)})</span>
            <span>Counter: <strong style="color:${preview.expectedCounter > 0 ? "var(--negative)" : "var(--positive)"}">${formatNum(preview.expectedCounter)} HP</strong></span>
          </div>
          <div class="tactical-preview-note">${escHtml(preview.effect)}</div>
        </div>`;
    } else if (run.phase === Phase.Riddle) {
      deckTitle = "THE RIDDLE KEEPER";
      const r = run.riddle();
      const pct = run.timeLimit > 0 ? Math.max(0, Math.min(100, (run.timeLeft / run.timeLimit) * 100)) : 0;
      const streakNote = p.riddleStreak >= 2 ? `<span class="streak-tag">Streak x${p.riddleStreak}${p.riddleStreak >= 4 ? " (+30% next!)" : " (+15% next!)"}</span>` : "";
      const obsNote = run.hasObservationHint() ? `<div style="font-size:0.8rem;color:var(--ice);">Observation &gt; 30 eliminated one false answer.</div>` : "";
      deckBody = `
        <div class="riddle-timer-wrap">
          <div class="riddle-timer-head">
            <span id="riddle-timer-label">${run.timeLeft.toFixed(1)}s remaining</span>
            ${streakNote}
          </div>
          <div class="progress-track"><div class="progress-fill" id="riddle-timer-fill" style="width:${pct}%;background:${run.timeLeft <= 5 ? "var(--negative)" : "var(--gold)"}"></div></div>
          ${obsNote}
        </div>
        <div class="choice-list">${r.answers.map((ans, idx) => {
          const avail = run.answerAvailable(idx);
          return `
            <button class="btn btn-choice" data-choice="${idx}" ${avail ? "" : "disabled"}>
              <span class="choice-key">${idx + 1}</span>
              <span style="${avail ? "" : "text-decoration:line-through;opacity:0.55;"}">${escHtml(avail ? ans : `${ans} (Eliminated)`)}</span>
            </button>`;
        }).join("")}</div>`;
    } else if (run.phase === Phase.Outcome || run.phase === Phase.RiddleOutcome || run.phase === Phase.Loot) {
      deckTitle = run.phase === Phase.Loot ? "CHEST UNLOCKED" : "OUTCOME";
      const label = run.phase === Phase.Loot ? "Claim the Chest's Treasure [Enter]" : "Continue [Enter]";
      deckBody = `<button class="btn btn-primary btn-choice" id="btn-advance"><span class="choice-key">&crarr;</span><span>${label}</span></button>`;
    } else if (run.phase === Phase.Complete) {
      deckTitle = "STAIRWAY UNLOCKED";
      const label = run.floorNumber === 1 ? "Choose Floor 2 Door [Enter]" : "Descend into Malvel's Domain (Floor 3) [Enter]";
      deckBody = `<button class="btn btn-primary btn-choice" id="btn-descend"><span class="choice-key">&crarr;</span><span>${label}</span></button>`;
    } else if (run.phase === Phase.Defeat) {
      deckTitle = "DESCENT ENDED";
      deckBody = `<button class="btn btn-danger btn-choice" id="btn-to-results"><span class="choice-key">&crarr;</span><span>View Run Summary [Enter]</span></button>`;
    }

    // Full naturally-flowing narrative text (no awkward mid-paragraph empty pagination!)
    let narrativeHeading = "THE CHAMBER";
    let narrativeText = "";
    if (run.phase === Phase.Riddle) {
      narrativeHeading = "SOLVE THE RIDDLE";
      narrativeText = run.riddle().question;
    } else if (run.phase === Phase.Room) {
      narrativeHeading = "THE CHAMBER";
      narrativeText = this.assets.getText(run.storyFile()).trim();
    } else {
      narrativeHeading = run.phase === Phase.Combat ? "COMBAT LOG" : run.phase === Phase.Defeat ? "FALLEN IN THE DEPTHS" : "CHRONICLE";
      narrativeText = run.message.trim();
    }

    this.root.innerHTML = `
      <main class="screen">
        <div class="container">
          <header class="game-header">
            <div class="game-brand-block">
              <h1 class="brand-title">DUNGEON DESCENT</h1>
              <div class="game-breadcrumb" style="color:${accent}">
                FLOOR ${run.floorNumber} / ${escHtml(biomeName(run.biome))} / ${escHtml(difficultyName(run.difficulty))} &middot; Seed #${run.seed}
              </div>
            </div>
            <div class="room-tracker" aria-label="Floor progress">${pips}</div>
            <div class="top-header-actions">
              <button class="btn btn-sm" id="btn-map" ${inRiddle || !run.inProgress() ? "disabled" : ""}>Map [M]</button>
              <button class="btn btn-sm" id="btn-pause" ${inRiddle || !run.inProgress() ? "disabled" : ""}>Pause [Esc]</button>
              <button class="btn btn-sm" id="btn-sound">${this.settings.muted ? "Sound off [V]" : "Sound on [V]"}</button>
            </div>
          </header>

          <div class="game-main-grid">
            <div class="chamber-column">
              <section class="panel chamber-card">
                <div class="chamber-card-header">
                  <span class="chamber-room-num">ROOM ${run.roomNumber()} / 9</span>
                  <span class="chamber-title">${escHtml(run.title().toUpperCase())}</span>
                </div>
                <div class="chamber-stage ${aspectCls} ${this.stageShake ? "shake" : ""}">
                  <img class="chamber-art" src="${imgUrl}" alt="${escHtml(run.title())}">
                  <div class="chamber-flash ${this.stageFlashColor ? "active" : ""}" style="background:${this.stageFlashColor || "transparent"}"></div>
                  <div class="floating-badges" id="floating-badges">${badgesHtml}</div>
                </div>
                ${combatStatusHtml}
              </section>
            </div>

            <div class="play-column">
              <section class="panel hero-strip">
                <div class="hero-strip-top">
                  <div class="hero-strip-identity">
                    <img class="hero-mini-avatar" src="${this.assets.assetUrl(portraitImage(p.hero))}" alt="${escHtml(heroName(p.hero))}">
                    <div>
                      <h2 class="hero-strip-name">${escHtml(heroName(p.hero))}</h2>
                      <div class="hero-strip-vitals">
                        <span title="${escHtml(statTooltip(Stat.Health))}"><strong>HP ${formatNum(p.stats[Stat.Health])}</strong> ${hpDeltaHtml}</span>
                        <span style="color:var(--muted);">Rep ${p.reputation}/9 ${repDeltaHtml}</span>
                      </div>
                    </div>
                  </div>
                  <div style="display:flex;gap:8px;">
                    <button class="btn btn-sm" id="btn-in-game-lore" ${inRiddle ? "disabled" : ""}>Hero lore</button>
                    <button class="btn btn-sm" id="btn-stats-info" ${inRiddle ? "disabled" : ""}>Stats [I]</button>
                  </div>
                </div>
                <div class="hero-strip-stats">${statChipsHtml}</div>
              </section>

              <section class="panel narrative-panel">
                <span class="kicker">${narrativeHeading}</span>
                <div class="narrative-text ${inRiddle ? "riddle-question" : ""}">${escHtml(narrativeText)}</div>
              </section>

              <section class="panel choice-deck">
                <span class="kicker">${deckTitle}</span>
                ${deckBody}
              </section>
            </div>
          </div>

          <div class="hotkey-bar">
            <span>[1-4] Choose / Tactic</span>
            <span>[I] or hover stats for tooltips</span>
            <span>[M] Map</span>
            <span>[Esc] Pause / Save &amp; Quit</span>
            <span>[V] Mute</span>
          </div>
        </div>
      </main>`;

    this.root.querySelectorAll("[data-choice]").forEach((btn) => {
      btn.addEventListener("click", () => this.handleRoomChoice(Number(btn.dataset.choice)));
    });
    this.root.querySelectorAll("[data-combat-idx]").forEach((btn) => {
      const updatePreview = () => {
        const idx = Number(btn.dataset.combatIdx);
        if (this.hoveredCombatAction === idx) return;
        this.hoveredCombatAction = idx;
        const preview = this.run.previewCombatAction(idx);
        const box = document.getElementById("tactical-preview");
        if (box) {
          box.innerHTML = `
            <div class="tactical-preview-head">
              <span>${escHtml(preview.title)}</span>
              <span>Cost: ${preview.strikeCost} strike${preview.strikeCost > 1 ? "s" : ""}</span>
            </div>
            <div class="tactical-preview-metrics">
              <span>Dmg: <strong>${formatNum(preview.minDamage)}</strong> (Crit ${formatNum(preview.critDamage)})</span>
              <span>Counter: <strong style="color:${preview.expectedCounter > 0 ? "var(--negative)" : "var(--positive)"}">${formatNum(preview.expectedCounter)} HP</strong></span>
            </div>
            <div class="tactical-preview-note">${escHtml(preview.effect)}</div>`;
        }
        this.root.querySelectorAll("[data-combat-idx]").forEach((b) => {
          b.classList.toggle("btn-primary", Number(b.dataset.combatIdx) === idx);
        });
      };
      btn.addEventListener("mouseenter", updatePreview);
      btn.addEventListener("focus", updatePreview);
    });

    document.getElementById("btn-advance")?.addEventListener("click", () => this.handleAdvance());
    document.getElementById("btn-descend")?.addEventListener("click", () => this.descendFromComplete());
    document.getElementById("btn-to-results")?.addEventListener("click", () => { this.audio.playSfx("click"); this.enter(Screen.Results); });
    document.getElementById("btn-map")?.addEventListener("click", () => { this.audio.playSfx("click"); this.overlay = Overlay.Map; this.renderOverlay(); });
    document.getElementById("btn-pause")?.addEventListener("click", () => { this.audio.playSfx("click"); this.overlay = Overlay.Pause; this.renderOverlay(); });
    document.getElementById("btn-sound")?.addEventListener("click", () => this.toggleMute());
    document.getElementById("btn-in-game-lore")?.addEventListener("click", () => {
      this.audio.playSfx("click");
      this.loreReturn = Screen.Game;
      this.loreHero = this.run.player.hero;
      this.enter(Screen.Lore);
    });
    document.getElementById("btn-stats-info")?.addEventListener("click", () => { this.audio.playSfx("click"); this.overlay = Overlay.Stats; this.renderOverlay(); });
  }

  renderEnding() {
    const ending = this.run.ending;
    const kind = ending === Ending.Perfect ? "PERFECT ENDING" :
      ending === Ending.True ? "TRUE ENDING" : ending === Ending.Good ? "GOOD ENDING" : "BAD ENDING";
    const imgUrl = this.assets.assetUrl(endingImage(ending));
    const fullStory = this.run.message.trim();

    this.root.innerHTML = `
      <main class="screen">
        <div class="screen-backdrop" style="background-image:url('${imgUrl}')"></div>
        <div class="container">
          <header class="top-header">
            <div class="top-header-left">
              <span class="kicker">${kind}</span>
              <h1 class="page-title">${escHtml(endingName(ending).toUpperCase())}</h1>
            </div>
            <div class="top-header-actions">
              <button class="btn btn-sm" id="btn-sound">${this.settings.muted ? "Sound off [V]" : "Sound on [V]"}</button>
              <button class="btn btn-primary" id="btn-to-results">Continue to Results &rarr;</button>
            </div>
          </header>

          <div class="ending-split">
            <div class="ending-art-square">
              <img src="${imgUrl}" alt="${escHtml(endingName(ending))}">
            </div>
            <section class="panel narrative-panel">
              <span class="kicker">Epilogue</span>
              <div class="narrative-text tall">${escHtml(fullStory)}</div>
              <div style="display:flex;justify-content:flex-end;padding-top:10px;border-top:1px solid var(--border-subtle);">
                <button class="btn btn-primary btn-sm" id="btn-to-results-bottom">Results &amp; Leaderboards [Esc]</button>
              </div>
            </section>
          </div>
        </div>
      </main>`;

    document.getElementById("btn-sound").addEventListener("click", () => this.toggleMute());
    document.getElementById("btn-to-results").addEventListener("click", () => { this.audio.playSfx("click"); this.enter(Screen.Results); });
    document.getElementById("btn-to-results-bottom").addEventListener("click", () => { this.audio.playSfx("click"); this.enter(Screen.Results); });
  }

  renderResultsOrLeaderboard() {
    const isResults = this.screen === Screen.Results;
    const isGlobal = this.lbMode === "global";
    const isAllDiff = this.lbDifficulty < 0;
    const colCount = isAllDiff ? 9 : 8;
    const user = this.api.getUser();
    const lockedByPending = isResults && this.entryPending;

    let totalPages = 1;
    let totalCount = 0;
    let rowsHtml = "";

    if (isGlobal) {
      if (this.globalLbLoading && !this.globalLbData) {
        rowsHtml = `<tr><td colspan="${colCount}" class="empty-state">Loading global rankings from server...</td></tr>`;
      } else if (this.globalLbError) {
        rowsHtml = `
          <tr>
            <td colspan="${colCount}" class="empty-state">
              <div style="margin-bottom:10px;color:var(--warning);">${escHtml(this.globalLbError)}</div>
              <div style="display:flex;justify-content:center;gap:8px;flex-wrap:wrap;">
                <button class="btn btn-sm btn-primary" id="btn-lb-retry">Retry Global Server</button>
                <button class="btn btn-sm" id="btn-lb-fallback-local">View Local Board</button>
              </div>
            </td>
          </tr>`;
      } else {
        const data = this.globalLbData || { entries: [], page: 1, totalPages: 1, totalEntries: 0 };
        totalPages = Math.max(1, data.totalPages || 1);
        totalCount = data.totalEntries || 0;
        this.lbPage = Math.max(0, Math.min(this.lbPage, totalPages - 1));
        const entries = Array.isArray(data.entries) ? data.entries : [];
        rowsHtml = entries.length ? entries.map((entry) => {
          const endTag = entry.ending === Ending.Perfect ? "PERFECT" : entry.ending === Ending.True ? "TRUE" : entry.ending === Ending.Good ? "GOOD" : "BAD";
          const endCol = entry.ending === Ending.Perfect ? "var(--gold)" : entry.ending === Ending.True ? "var(--positive)" : entry.ending === Ending.Bad ? "var(--negative)" : "var(--ink)";
          const isJustSubmitted = this.lastGlobalEntryId !== null && entry.id === this.lastGlobalEntryId;
          const isMine = user && entry.userId === user.id;
          const rowClass = isJustSubmitted ? "own-row" : (isMine ? "mine-row" : "");
          const entryDiff = Number.isInteger(entry.difficulty) ? entry.difficulty : Math.max(0, this.lbDifficulty);
          return `
            <tr class="${rowClass}">
              <td>#${entry.rank}</td>
              <td><strong>${escHtml(entry.username)}</strong>${isMine ? ` <span class="you-tag">YOU</span>` : ""}</td>
              <td>${entry.score}</td>
              ${isAllDiff ? `<td>${escHtml(difficultyName(entryDiff))}</td>` : ""}
              <td>${escHtml(heroName(entry.hero))}</td>
              <td style="color:${endCol};font-weight:600;">${endTag}</td>
              <td>${formatDuration(entry.durationSeconds)}</td>
              <td>#${entry.seed}</td>
              <td><button class="btn btn-sm" data-replay-seed="${entry.seed}" data-replay-hero="${entry.hero}" data-replay-diff="${entryDiff}" ${lockedByPending ? "disabled" : ""}>Replay</button></td>
            </tr>`;
        }).join("") : `<tr><td colspan="${colCount}" class="empty-state">${this.lbMineOnly ? "You have not posted any runs matching these filters yet." : "No global records match the selected filters yet — be the first!"}</td></tr>`;
      }
    } else {
      let rawBoard;
      if (isAllDiff) {
        rawBoard = [];
        for (let d = 0; d < 4; d++) {
          for (const e of (this.leaderboards[d] || [])) {
            rawBoard.push({ ...e, difficulty: d });
          }
        }
        rawBoard.sort((a, b) => (b.score - a.score) || ((a.durationSeconds || 0) - (b.durationSeconds || 0)) || ((a.id || 0) - (b.id || 0)));
      } else {
        rawBoard = (this.leaderboards[this.lbDifficulty] || []).map((e) => ({ ...e, difficulty: this.lbDifficulty }));
      }
      const filtered = [];
      for (let i = 0; i < rawBoard.length; i++) {
        const e = rawBoard[i];
        const heroOk = this.lbHeroFilter < 0 || e.hero === this.lbHeroFilter;
        const endOk = this.lbEndingFilter === 0 || e.ending === this.lbEndingFilter;
        if (heroOk && endOk) filtered.push({ rank: i + 1, entry: e });
      }
      const perPage = 10;
      totalPages = Math.max(1, Math.ceil(filtered.length / perPage));
      totalCount = filtered.length;
      this.lbPage = Math.max(0, Math.min(this.lbPage, totalPages - 1));
      const slice = filtered.slice(this.lbPage * perPage, (this.lbPage + 1) * perPage);
      rowsHtml = slice.length ? slice.map(({ rank, entry }) => {
        const endTag = entry.ending === Ending.Perfect ? "PERFECT" : entry.ending === Ending.True ? "TRUE" : entry.ending === Ending.Good ? "GOOD" : "BAD";
        const endCol = entry.ending === Ending.Perfect ? "var(--gold)" : entry.ending === Ending.True ? "var(--positive)" : entry.ending === Ending.Bad ? "var(--negative)" : "var(--ink)";
        const own = entry.id === this.lastEntryId;
        const playerName = entry.username || entry.initials || "Guest";
        return `
          <tr class="${own ? "own-row" : ""}">
            <td>#${rank}</td>
            <td><strong>${escHtml(playerName)}</strong></td>
            <td>${entry.score}</td>
            ${isAllDiff ? `<td>${escHtml(difficultyName(entry.difficulty))}</td>` : ""}
            <td>${escHtml(heroName(entry.hero))}</td>
            <td style="color:${endCol};font-weight:600;">${endTag}</td>
            <td>${formatDuration(entry.durationSeconds)}</td>
            <td>#${entry.seed}</td>
            <td><button class="btn btn-sm" data-replay-seed="${entry.seed}" data-replay-hero="${entry.hero}" data-replay-diff="${entry.difficulty}" ${lockedByPending ? "disabled" : ""}>Replay</button></td>
          </tr>`;
      }).join("") : `<tr><td colspan="${colCount}" class="empty-state">No local records match the selected filters yet.</td></tr>`;
    }

    const diffTabs = [
      { id: -1, label: "All Difficulties" },
      { id: 0, label: difficultyName(0) },
      { id: 1, label: difficultyName(1) },
      { id: 2, label: difficultyName(2) },
      { id: 3, label: difficultyName(3) }
    ].map((d) =>
      `<button class="btn btn-sm btn-tab ${this.lbDifficulty === d.id ? "active" : ""}" data-lb-diff="${d.id}" ${lockedByPending ? "disabled" : ""}>${escHtml(d.label)}</button>`
    ).join("");

    const heroFilters = [
      { id: -1, label: "All Heroes" },
      { id: 0, label: "Warrior" },
      { id: 1, label: "Ranger" },
      { id: 2, label: "Mage" },
      { id: 3, label: "Rogue" }
    ].map((f) =>
      `<button class="btn btn-sm btn-tab ${this.lbHeroFilter === f.id ? "active" : ""}" data-lb-hero="${f.id}" ${lockedByPending ? "disabled" : ""}>${f.label}</button>`
    ).join("");

    const endingFilters = ["All Endings", "Good", "True", "Bad", "Perfect"].map((label, idx) =>
      `<button class="btn btn-sm btn-tab ${this.lbEndingFilter === idx ? "active" : ""}" data-lb-ending="${idx}" ${lockedByPending ? "disabled" : ""}>${label}</button>`
    ).join("");

    let summaryCol = "";
    if (isResults) {
      const p = this.run.player;
      let submitForm = "";
      if (this.entryPending) {
        if (user) {
          submitForm = `
            <div class="panel-inset" style="padding:14px;display:flex;flex-direction:column;gap:10px;">
              <div class="kicker">Record Your Descent</div>
              <div style="font-size:0.88rem;color:var(--muted);">Posting to the Global Leaderboard as <strong style="color:var(--gold)">${escHtml(user.username)}</strong>.</div>
              <div style="display:flex;gap:8px;flex-wrap:wrap;">
                <button class="btn btn-primary btn-sm" id="btn-submit-global" ${this.submittingScore ? "disabled" : ""}>${this.submittingScore ? "Posting..." : "Post Global Score"}</button>
                <button class="btn btn-sm" id="btn-skip-score" ${this.submittingScore ? "disabled" : ""}>Skip</button>
              </div>
            </div>`;
        } else {
          submitForm = `
            <div class="panel-inset" style="padding:14px;display:flex;flex-direction:column;gap:10px;">
              <div class="kicker">Record Your Descent</div>
              <div style="font-size:0.86rem;color:var(--muted);">Sign in or create a free account to post your score under your username on the Global Leaderboard.</div>
              <button class="btn btn-primary btn-sm" id="btn-results-signin">Sign In / Create Account to Post Globally</button>
              <div class="guest-divider">or save locally in this browser</div>
              <div class="initials-form">
                <input id="initials-input" class="player-name-input" type="text" maxlength="20" placeholder="Guest name" value="${escHtml(this.initialsValue)}">
                <button class="btn btn-sm" id="btn-submit-local">Save Local</button>
                <button class="btn btn-sm" id="btn-skip-score">Skip</button>
              </div>
            </div>`;
        }
      }

      const runSeconds = Math.round(this.playTimeSeconds);
      const bd = this.run.scoreBreakdown(runSeconds);
      summaryCol = `
        <aside class="panel summary-card">
          <span class="kicker">Run Summary</span>
          <h2 class="section-title">${escHtml(this.hasEndingResult() ? endingName(this.run.ending) : "Fallen in the Depths")}</h2>
          <div class="kv-list">
            <div class="kv-row"><span>Hero</span><span>${escHtml(heroName(p.hero))}</span></div>
            <div class="kv-row"><span>Difficulty</span><span>${escHtml(difficultyName(this.run.difficulty))}</span></div>
            <div class="kv-row"><span>Final Score</span><span style="color:var(--gold);font-weight:700;">${bd.total}</span></div>
          </div>
          <div class="panel-inset" style="padding:10px 12px;display:grid;grid-template-columns:1fr 1fr;gap:4px 14px;font-size:0.84rem;">
            <div style="display:flex;justify-content:space-between;gap:8px;"><span style="color:var(--muted);">Stats</span><span>+${bd.statsPoints}</span></div>
            <div style="display:flex;justify-content:space-between;gap:8px;"><span style="color:var(--muted);">Health</span><span>+${bd.healthPoints}</span></div>
            <div style="display:flex;justify-content:space-between;gap:8px;"><span style="color:var(--muted);">Strikes</span><span>+${bd.strikesPoints}</span></div>
            <div style="display:flex;justify-content:space-between;gap:8px;"><span style="color:var(--muted);">Time</span><span style="color:${bd.timePoints > 0 ? "var(--ink)" : "var(--muted)"};">+${bd.timePoints}</span></div>
          </div>
          <div class="kv-list">
            <div class="kv-row"><span>Battles Won</span><span>${p.battlesWon} / ${p.battlesFought}</span></div>
            <div class="kv-row"><span>Strikes Leftover</span><span>${p.strikesLeftover || 0}</span></div>
            <div class="kv-row"><span>Ending Health</span><span>${formatNum(p.stats[Stat.Health])}</span></div>
            <div class="kv-row"><span>Riddles Solved</span><span>${p.riddlesCorrect} / ${p.riddlesAnswered}</span></div>
            <div class="kv-row"><span>Best Streak</span><span>${p.bestRiddleStreak}</span></div>
            <div class="kv-row"><span>Reputation</span><span>${p.reputation} / 9</span></div>
            <div class="kv-row"><span>Time</span><span>${formatDuration(this.playTimeSeconds)}</span></div>
            <div class="kv-row"><span>Seed</span><span>#${this.run.seed}</span></div>
          </div>
          ${submitForm}
        </aside>`;
    }

    this.root.innerHTML = `
      <main class="screen">
        <div class="screen-backdrop" style="background-image:url('${this.assets.assetUrl("images/startscreen.jpg")}')"></div>
        <div class="container">
          <header class="top-header">
            <div class="top-header-left">
              <h1 class="page-title">${isResults ? "DESCENT RESULTS" : "HIGH SCORES"}</h1>
              <p class="subtitle">${isGlobal
                ? "Global rankings across all players by difficulty, hero, and ending — click Replay to challenge any seed."
                : "Local browser scores saved on this device — switch to Global to compete worldwide."}</p>
            </div>
            <div class="top-header-actions">
              ${this.renderAccountButtonHtml()}
              <button class="btn btn-primary btn-sm" id="btn-back-menu" ${lockedByPending ? "disabled" : ""}>Back to menu</button>
            </div>
          </header>

          <div class="${isResults ? "results-layout" : ""}">
            ${summaryCol}
            <section class="panel leaderboard-panel">
              <div class="lb-mode-row">
                <div class="filter-strip">
                  <button class="btn btn-sm btn-tab ${isGlobal ? "active" : ""}" id="btn-mode-global" ${lockedByPending ? "disabled" : ""}>Global Leaderboard</button>
                  <button class="btn btn-sm btn-tab ${!isGlobal ? "active" : ""}" id="btn-mode-local" ${lockedByPending ? "disabled" : ""}>Local Browser</button>
                  ${isGlobal && user ? `<button class="btn btn-sm btn-tab ${this.lbMineOnly ? "active" : ""}" id="btn-mode-mine" ${lockedByPending ? "disabled" : ""}>My Runs Only</button>` : ""}
                </div>
                ${isGlobal ? `<button class="btn btn-sm" id="btn-lb-refresh" ${this.globalLbLoading || lockedByPending ? "disabled" : ""}>${this.globalLbLoading ? "Syncing..." : "Refresh"}</button>` : ""}
              </div>
              <div class="filter-strip">${diffTabs}</div>
              <div class="filter-strip">${heroFilters}</div>
              <div class="filter-strip">${endingFilters}</div>
              <div class="lb-table-wrap">
                <table class="lb-table">
                  <thead>
                    <tr>
                      <th>Rank</th><th>Player</th><th>Score</th>${isAllDiff ? "<th>Difficulty</th>" : ""}<th>Hero</th><th>Ending</th><th>Time</th><th>Seed</th><th>Replay</th>
                    </tr>
                  </thead>
                  <tbody>${rowsHtml}</tbody>
                </table>
              </div>
              <div class="pagination-bar">
                <button class="btn btn-sm" id="btn-lb-prev" ${this.lbPage <= 0 || lockedByPending || (isGlobal && this.globalLbLoading) ? "disabled" : ""}>&lt; Prev</button>
                <span class="pagination-label">Page ${this.lbPage + 1} / ${totalPages} (${totalCount} records)</span>
                <button class="btn btn-sm" id="btn-lb-next" ${this.lbPage >= totalPages - 1 || lockedByPending || (isGlobal && this.globalLbLoading) ? "disabled" : ""}>Next &gt;</button>
              </div>
            </section>
          </div>
        </div>
      </main>`;

    document.getElementById("btn-account")?.addEventListener("click", () => this.openAccountModal("login"));
    document.getElementById("btn-results-signin")?.addEventListener("click", () => this.openAccountModal("register"));

    const initInput = document.getElementById("initials-input");
    if (initInput) {
      initInput.addEventListener("input", (e) => {
        this.initialsValue = e.target.value.replace(/[^A-Za-z0-9_-]/g, "").slice(0, 20);
        e.target.value = this.initialsValue;
      });
      initInput.addEventListener("keydown", (e) => {
        if (e.key === "Enter") document.getElementById("btn-submit-local")?.click();
      });
    }

    document.getElementById("btn-submit-global")?.addEventListener("click", async () => {
      if (this.submittingScore || !this.api.isSignedIn()) return;
      this.submittingScore = true;
      this.render();
      try {
        const durationSeconds = Math.round(this.playTimeSeconds);
        const finalScore = this.run.score(durationSeconds);
        const res = await this.api.submitScore({
          difficulty: this.run.difficulty,
          score: finalScore,
          hero: this.run.player.hero,
          ending: this.run.ending,
          seed: this.run.seed,
          durationSeconds
        });
        // Also save a local backup copy under the player's username
        try {
          const localRes = this.store.submitLeaderboard(
            this.run.difficulty, this.api.getUser().username, finalScore,
            this.run.player.hero, this.run.ending, this.run.seed, durationSeconds
          );
          this.leaderboards = localRes.tables;
          this.lastEntryId = localRes.entryId;
        } catch {}

        this.lastGlobalEntryId = res.entry?.id ?? null;
        this.entryPending = false;
        this.submittingScore = false;
        this.lbMode = "global";
        this.lbDifficulty = -1;
        this.lbHeroFilter = -1;
        this.lbEndingFilter = 0;
        this.lbMineOnly = false;
        this.lbPage = 0;
        this.audio.playSfx("loot");
        this.toast(`Posted globally! Rank #${res.rank} on ${difficultyName(this.run.difficulty)}.`);
        this.fetchGlobalLeaderboard();
      } catch (err) {
        this.submittingScore = false;
        this.toast(err.message || "Could not post global score.");
        this.render();
      }
    });

    document.getElementById("btn-submit-local")?.addEventListener("click", () => {
      const name = this.initialsValue.trim();
      if (!name) {
        this.toast("Enter a guest name (1–20 letters or digits) first.");
        return;
      }
      const durationSeconds = Math.round(this.playTimeSeconds);
      const res = this.store.submitLeaderboard(
        this.run.difficulty, name, this.run.score(durationSeconds),
        this.run.player.hero, this.run.ending, this.run.seed, durationSeconds
      );
      this.leaderboards = res.tables;
      this.lastEntryId = res.entryId;
      this.entryPending = false;
      this.lbMode = "local";
      this.audio.playSfx("loot");
      this.toast("Score saved to local browser leaderboard!");
      this.render();
    });

    document.getElementById("btn-skip-score")?.addEventListener("click", () => {
      this.entryPending = false;
      this.audio.playSfx("click");
      this.render();
    });

    document.getElementById("btn-back-menu").addEventListener("click", () => { this.audio.playSfx("click"); this.enter(Screen.Menu); });

    document.getElementById("btn-mode-global")?.addEventListener("click", () => {
      this.lbMode = "global";
      this.lbPage = 0;
      this.audio.playSfx("click");
      this.fetchGlobalLeaderboard();
    });
    document.getElementById("btn-mode-local")?.addEventListener("click", () => {
      this.lbMode = "local";
      this.lbPage = 0;
      this.audio.playSfx("click");
      this.render();
    });
    document.getElementById("btn-mode-mine")?.addEventListener("click", () => {
      this.lbMineOnly = !this.lbMineOnly;
      this.lbPage = 0;
      this.audio.playSfx("click");
      this.fetchGlobalLeaderboard();
    });
    document.getElementById("btn-lb-refresh")?.addEventListener("click", () => {
      this.audio.playSfx("click");
      this.fetchGlobalLeaderboard();
    });
    document.getElementById("btn-lb-retry")?.addEventListener("click", () => {
      this.audio.playSfx("click");
      this.fetchGlobalLeaderboard();
    });
    document.getElementById("btn-lb-fallback-local")?.addEventListener("click", () => {
      this.lbMode = "local";
      this.lbPage = 0;
      this.audio.playSfx("click");
      this.render();
    });

    const reloadBoard = () => {
      if (this.lbMode === "global") this.fetchGlobalLeaderboard();
      else this.render();
    };
    this.root.querySelectorAll("[data-lb-diff]").forEach((b) => b.addEventListener("click", () => {
      this.lbDifficulty = Number(b.dataset.lbDiff);
      this.lbPage = 0;
      this.audio.playSfx("click");
      reloadBoard();
    }));
    this.root.querySelectorAll("[data-lb-hero]").forEach((b) => b.addEventListener("click", () => {
      this.lbHeroFilter = Number(b.dataset.lbHero);
      this.lbPage = 0;
      this.audio.playSfx("click");
      reloadBoard();
    }));
    this.root.querySelectorAll("[data-lb-ending]").forEach((b) => b.addEventListener("click", () => {
      this.lbEndingFilter = Number(b.dataset.lbEnding);
      this.lbPage = 0;
      this.audio.playSfx("click");
      reloadBoard();
    }));
    document.getElementById("btn-lb-prev")?.addEventListener("click", () => {
      this.lbPage = Math.max(0, this.lbPage - 1);
      this.audio.playSfx("click");
      reloadBoard();
    });
    document.getElementById("btn-lb-next")?.addEventListener("click", () => {
      this.lbPage = Math.min(totalPages - 1, this.lbPage + 1);
      this.audio.playSfx("click");
      reloadBoard();
    });
    this.root.querySelectorAll("[data-replay-seed]").forEach((b) => b.addEventListener("click", () => {
      this.selectedHero = Number(b.dataset.replayHero);
      this.selectedDifficulty = Number(b.dataset.replayDiff);
      this.seedInput = String(b.dataset.replaySeed);
      this.dailyRunSelected = false;
      this.choosingSecondFloor = false;
      this.audio.playSfx("click");
      this.enter(Screen.Biomes);
    }));
  }

  renderCodex() {
    const c = this.codex;
    const milestones = getCodexMilestones(c);
    const unlockedCount = milestones.filter((m) => m.unlocked).length;
    const endCount = unlockedEndingCount(c);

    let bodyHtml = "";
    if (this.codexTab === 0) {
      const accPct = c.totalRiddlesAnswered > 0 ? Math.round((c.totalRiddlesCorrect / c.totalRiddlesAnswered) * 100) : 0;
      const perPage = 8;
      const totalPages = Math.max(1, Math.ceil(milestones.length / perPage));
      this.codexMilestonePage = Math.max(0, Math.min(this.codexMilestonePage, totalPages - 1));
      const slice = milestones.slice(this.codexMilestonePage * perPage, (this.codexMilestonePage + 1) * perPage);

      const heroRows = [0, 1, 2, 3].map((h) => `
        <div class="panel-inset hero-record-row">
          <strong style="color:var(--gold)">${escHtml(heroName(h))}</strong>
          <span>Wins: ${c.heroVictories[h]}</span>
          <span style="color:var(--positive)">Best: ${c.heroBestScore[h]}</span>
        </div>`).join("");

      const msCards = slice.map((m) => `
        <article class="milestone-card ${m.unlocked ? "unlocked" : ""}">
          <div class="milestone-status">${m.unlocked ? "UNLOCKED" : "LOCKED"}</div>
          <div class="milestone-title">${escHtml(m.title)}</div>
          <div class="milestone-desc">${escHtml(m.description)}</div>
        </article>`).join("");

      bodyHtml = `
        <div class="codex-grid">
          <aside class="panel career-panel">
            <div>
              <div class="kicker" style="margin-bottom:10px;">Career Summary</div>
              <div class="kv-list">
                <div class="kv-row"><span>Completed runs</span><span>${c.totalRuns}</span></div>
                <div class="kv-row"><span>Victories vs Malvel</span><span style="color:var(--positive)">${c.totalVictories}</span></div>
                <div class="kv-row"><span>Fallen descents</span><span>${c.totalDefeats}</span></div>
                <div class="kv-row"><span>Impossible clears</span><span>${c.impossibleVictories}</span></div>
                <div class="kv-row"><span>Riddle accuracy</span><span>${c.totalRiddlesCorrect} / ${c.totalRiddlesAnswered} (${accPct}%)</span></div>
                <div class="kv-row"><span>Best riddle streak</span><span>${c.bestRiddleStreak}</span></div>
                <div class="kv-row"><span>Fastest victory</span><span>${c.fastestVictorySeconds > 0 ? formatDuration(c.fastestVictorySeconds) : "—"}</span></div>
                <div class="kv-row"><span>Best Gauntlet run</span><span>${c.bestGauntletScore} pts (Streak ${c.bestGauntletStreak})</span></div>
                <div class="kv-row"><span>Ashes of Falkreath</span><span style="color:${c.prequelCompleted ? "var(--gold)" : (isPrequelUnlocked(c) ? "var(--positive)" : "var(--muted)")}">${c.prequelCompleted ? "Completed" : (isPrequelUnlocked(c) ? "Unlocked" : "Sealed")}</span></div>
              </div>
            </div>
            <div>
              <div class="kicker" style="margin-bottom:10px;">Hero Records</div>
              <div class="hero-records-list">${heroRows}</div>
            </div>
          </aside>

          <section class="panel milestones-panel">
            <div class="milestones-header">
              <span class="kicker">Milestones (${unlockedCount} / ${milestones.length})</span>
              <div style="display:flex;align-items:center;gap:8px;">
                <button class="btn btn-sm" id="btn-ms-prev" ${this.codexMilestonePage <= 0 ? "disabled" : ""}>&lt;</button>
                <span class="pagination-label">Page ${this.codexMilestonePage + 1} / ${totalPages}</span>
                <button class="btn btn-sm" id="btn-ms-next" ${this.codexMilestonePage >= totalPages - 1 ? "disabled" : ""}>&gt;</button>
              </div>
            </div>
            <div class="milestones-grid">${msCards}</div>
          </section>
        </div>`;
    } else {
      const endings = [Ending.Good, Ending.True, Ending.Bad, Ending.Perfect];
      const endButtons = endings.map((e, i) => {
        const unlocked = isEndingUnlocked(c, e);
        const label = unlocked ? endingName(e) : `Locked Ending #${i + 1}`;
        return `<button class="btn btn-sm btn-tab ${this.codexEnding === e ? "active" : ""}" data-codex-ending="${e}">${escHtml(label)}</button>`;
      }).join("");

      const unlocked = isEndingUnlocked(c, this.codexEnding);
      if (unlocked) {
        const imgUrl = this.assets.assetUrl(endingImage(this.codexEnding));
        const fullStory = this.assets.getText(endingStoryFile(this.codexEnding)).trim();
        bodyHtml = `
          <div class="tabs-row">${endButtons}</div>
          <div class="ending-split">
            <div class="ending-art-square">
              <img src="${imgUrl}" alt="${escHtml(endingName(this.codexEnding))}">
            </div>
            <section class="panel narrative-panel">
              <span class="kicker">Now Playing Dedicated Ending Theme</span>
              <h2 class="section-title">${escHtml(endingName(this.codexEnding))}</h2>
              <div class="narrative-text tall">${escHtml(fullStory)}</div>
            </section>
          </div>`;
      } else {
        bodyHtml = `
          <div class="tabs-row">${endButtons}</div>
          <section class="panel" style="padding:42px 28px;text-align:center;">
            <span class="kicker">Locked Ending</span>
            <h2 class="section-title" style="margin-top:8px;">Undiscovered Fate</h2>
            <p class="subtitle" style="max-width:560px;margin:12px auto 0;">Hint: ${escHtml(endingHint(this.codexEnding))}</p>
          </section>`;
      }
    }

    const user = this.api.getUser();
    this.root.innerHTML = `
      <main class="screen">
        <div class="screen-backdrop" style="background-image:url('${this.assets.assetUrl("images/startscreen.jpg")}')"></div>
        <div class="container">
          <header class="top-header">
            <div class="top-header-left">
              <h1 class="page-title">CHRONICLES OF AETHORIA</h1>
              <p class="subtitle">${user
                ? `Cloud synced as ${escHtml(user.username)} — your endings, milestones, and records follow your account.`
                : "Saved locally in this browser — sign in to sync your Codex across devices."}</p>
            </div>
            <div class="top-header-actions">
              ${this.renderAccountButtonHtml()}
              <button class="btn btn-sm" id="btn-sound">${this.settings.muted ? "Sound off [V]" : "Sound on [V]"}</button>
              <button class="btn btn-sm" id="btn-back-menu">Back to menu [Esc]</button>
            </div>
          </header>

          <div class="tabs-row">
            <button class="btn btn-tab ${this.codexTab === 0 ? "active" : ""}" id="btn-codex-tab-0">Career &amp; Milestones</button>
            <button class="btn btn-tab ${this.codexTab === 1 ? "active" : ""}" id="btn-codex-tab-1">Ending Gallery (${endCount} / 4)</button>
          </div>

          ${bodyHtml}
        </div>
      </main>`;

    document.getElementById("btn-account")?.addEventListener("click", () => this.openAccountModal("login"));
    document.getElementById("btn-sound").addEventListener("click", () => this.toggleMute());
    document.getElementById("btn-back-menu").addEventListener("click", () => { this.audio.playSfx("click"); this.enter(Screen.Menu); });
    document.getElementById("btn-codex-tab-0").addEventListener("click", () => {
      this.codexTab = 0; this.page = 0; this.audio.playSfx("click"); this.syncCodexMusic(); this.render();
    });
    document.getElementById("btn-codex-tab-1").addEventListener("click", () => {
      this.codexTab = 1; this.page = 0; this.audio.playSfx("click"); this.syncCodexMusic(); this.render();
    });
    document.getElementById("btn-ms-prev")?.addEventListener("click", () => { this.codexMilestonePage--; this.audio.playSfx("click"); this.render(); });
    document.getElementById("btn-ms-next")?.addEventListener("click", () => { this.codexMilestonePage++; this.audio.playSfx("click"); this.render(); });
    this.root.querySelectorAll("[data-codex-ending]").forEach((b) => b.addEventListener("click", () => {
      this.codexEnding = Number(b.dataset.codexEnding);
      this.page = 0;
      this.audio.playSfx("click");
      this.syncCodexMusic();
      this.render();
    }));
    document.getElementById("btn-prev-page")?.addEventListener("click", () => this.changePage(-1));
    document.getElementById("btn-next-page")?.addEventListener("click", () => this.changePage(1));
  }

  renderPrequel() {
    const pq = this.prequel;
    const ch = pq.character;
    const ph = pq.phase;
    const accent = prequelActAccent(pq.act);
    const imgUrl = this.assets.assetUrl(pq.sceneImage);

    // Epilogue screen bridges straight into the Pedestal of Four Weapons
    if (ph === PrequelPhase.Epilogue) {
      const codaKind = pq.coda === PrequelCoda.HollowCrown ? "PRELUDE CODA I · RUTHLESS PATH" : "PRELUDE CODA II · REMORSEFUL PATH";
      this.root.innerHTML = `
        <main class="screen">
          <div class="screen-backdrop" style="background-image:url('${imgUrl}')"></div>
          <div class="container">
            <header class="top-header">
              <div class="top-header-left">
                <span class="kicker" style="color:${accent}">${codaKind}</span>
                <h1 class="page-title">${escHtml(pq.heading)}</h1>
                <p class="subtitle">${escHtml(pq.subheading)}</p>
              </div>
              <div class="top-header-actions">
                <button class="btn btn-sm" id="btn-sound">${this.settings.muted ? "Sound off [V]" : "Sound on [V]"}</button>
                <button class="btn btn-sm" id="btn-back-menu">Main Menu [Esc]</button>
              </div>
            </header>

            <div class="ending-split">
              <div class="ending-art-square">
                <img src="${imgUrl}" alt="The Pedestal of Four Weapons">
              </div>
              <section class="panel narrative-panel">
                <span class="kicker">The Chronicle Closes — Dungeon Descent Begins</span>
                <div class="narrative-text tall">${escHtml(pq.body)}</div>
                <div class="prequel-summary-row">
                  <span class="thread-pill legacy">Legacy ${pq.legacy}</span>
                  <span class="thread-pill fury">Fury ${pq.fury}</span>
                  <span class="thread-pill remorse">Remorse ${pq.remorse}</span>
                  <span class="thread-pill">Battles ${pq.battlesWon}/6</span>
                  <span class="thread-pill">Parleys ${pq.parleysWon}</span>
                  ${pq.ironChronicleEligible() ? `<span class="thread-pill legacy">Iron Chronicle</span>` : ""}
                </div>
                <div class="prequel-epilogue-actions">
                  <div style="display:flex;gap:8px;flex-wrap:wrap;">
                    <button class="btn btn-sm" id="btn-prequel-replay">Replay Chronicle</button>
                    <button class="btn btn-sm" id="btn-back-menu-bottom">Return to Menu</button>
                  </div>
                  <button class="btn btn-primary" id="btn-prequel-to-main">Awaken in the Pedestal Chamber &rarr;</button>
                </div>
              </section>
            </div>
          </div>
        </main>`;

      document.getElementById("btn-sound")?.addEventListener("click", () => this.toggleMute());
      document.getElementById("btn-back-menu")?.addEventListener("click", () => { this.audio.playSfx("click"); this.enter(Screen.Menu); });
      document.getElementById("btn-back-menu-bottom")?.addEventListener("click", () => { this.audio.playSfx("click"); this.enter(Screen.Menu); });
      document.getElementById("btn-prequel-replay")?.addEventListener("click", () => this.handlePrequelRestart());
      document.getElementById("btn-prequel-to-main")?.addEventListener("click", () => {
        this.audio.playSfx("click");
        this.enter(Screen.Introduction);
      });
      return;
    }

    const inParley = ph === PrequelPhase.Parley;
    const inCombat = ph === PrequelPhase.Combat;
    const showBadges = inCombat || ph === PrequelPhase.Outcome;

    // 5-chapter progress pips for the current Act
    const actChapter = pq.actChapterNumber();
    const pips = Array.from({ length: 5 }, (_, i) => {
      const done = ph === PrequelPhase.ActComplete || (ph !== PrequelPhase.ActIntro && i + 1 < actChapter);
      const current = ph !== PrequelPhase.ActIntro && ph !== PrequelPhase.ActComplete && i + 1 === actChapter;
      const cls = done ? "done" : (current ? "current" : "");
      return `<span class="room-pip ${cls}">${i + 1}</span>`;
    }).join("");

    // Combat Enemy Bar + Telegraphed Boss Intent cleanly BELOW the square artwork
    let combatStatusHtml = "";
    if (inCombat) {
      const en = pq.enemy;
      const hpPct = en.maxHp > 0 ? Math.max(0, Math.min(100, (en.hp / en.maxHp) * 100)) : 0;
      const intent = en.intent;
      const intentLabel = intent && intent.active
        ? (intent.telegraphed
            ? `WARNING — ${intent.name.replace(/^TELEGRAPHED:\s*/i, "")}`
            : `BOSS INTENT: ${intent.name}`)
        : "";
      const intentHtml = intent && intent.active ? `
        <div class="boss-intent-banner ${intent.telegraphed ? "telegraphed" : ""}">
          <strong>${escHtml(intentLabel)}</strong>
          <span>${escHtml(intent.detail)}</span>
        </div>` : "";
      combatStatusHtml = `
        <div class="combat-status-stack">
          <div class="enemy-bar-card">
            <div class="enemy-bar-top">
              <strong>${escHtml(en.name)} &middot; HP ${formatNum(en.hp)} / ${formatNum(en.maxHp)}</strong>
              <span class="strikes-pill">Strikes left: ${en.strikesLeft}</span>
            </div>
            <div class="progress-track"><div class="progress-fill" style="width:${hpPct}%;background:var(--negative)"></div></div>
          </div>
          ${intentHtml}
        </div>`;
    }

    const badgesHtml = showBadges
      ? this.combatBadges.map((b) => `<div class="combat-badge" style="color:${b.color}">${escHtml(b.text)}</div>`).join("")
      : "";

    // 4 POV stat chips with prequel-specific tooltips
    const povStats = [
      { short: "PWR", val: ch.power, tip: "Power: governs base strike and signature ability damage." },
      { short: "RES", val: ch.resolve, tip: "Resolve: extends Parley timers (+1s per 2 Resolve) and mitigates enemy counterattacks." },
      { short: "CUN", val: ch.cunning, tip: "Cunning: raises critical strike chance and eliminates a weak bluff in Parleys at 8+." },
      { short: "AGI", val: ch.agility, tip: "Agility: grants bonus strikes per battle (+1 strike per 3 Agility)." }
    ];
    const statChipsHtml = povStats.map((s) => `
      <div class="panel-inset stat-chip" title="${escHtml(s.tip)}">
        <span class="stat-chip-label">${s.short}</span>
        <span class="stat-chip-val"><span>${formatNum(s.val)}</span></span>
      </div>`).join("");

    // Choice deck content by prequel phase
    let deckTitle = "YOUR DECISION";
    let deckBody = "";
    if (ph === PrequelPhase.ActIntro) {
      deckTitle = `ACT ${pq.actNumber()} PROLOGUE`;
      deckBody = `<button class="btn btn-primary btn-choice" id="btn-prequel-advance"><span class="choice-key">&crarr;</span><span>Begin Act ${pq.actNumber()} — ${escHtml(ch.name)} [Enter]</span></button>`;
    } else if (ph === PrequelPhase.Scene) {
      deckTitle = "YOUR CHOICE";
      deckBody = `<div class="choice-list">${pq.choices.map((c, idx) => `
        <button class="btn btn-choice" data-prequel-choice="${idx}">
          <span class="choice-key">${idx + 1}</span>
          <span>${escHtml(c.label)}</span>
        </button>`).join("")}</div>`;
    } else if (ph === PrequelPhase.Parley) {
      deckTitle = `TIMED PARLEY — ${pq.parley.speaker.toUpperCase()}`;
      const pl = pq.parley;
      const pct = pl.timeLimit > 0 ? Math.max(0, Math.min(100, (pl.timeLeft / pl.timeLimit) * 100)) : 0;
      const cunningNote = pl.hiddenIndex < 3
        ? `<div style="font-size:0.8rem;color:var(--ice);">Cunning &ge; 8 revealed &amp; eliminated one weak bluff.</div>` : "";
      deckBody = `
        <div class="riddle-timer-wrap">
          <div class="riddle-timer-head">
            <span id="riddle-timer-label">${pl.timeLeft.toFixed(1)}s remaining</span>
            <span style="font-size:0.8rem;color:var(--muted);">Resolve bonus active</span>
          </div>
          <div class="progress-track"><div class="progress-fill" id="riddle-timer-fill" style="width:${pct}%;background:${pl.timeLeft <= 5 ? "var(--negative)" : "var(--gold)"}"></div></div>
          ${cunningNote}
        </div>
        <div class="choice-list">${pl.options.map((opt, idx) => {
          const avail = pq.parleyOptionAvailable(idx);
          return `
            <button class="btn btn-choice" data-prequel-choice="${idx}" ${avail ? "" : "disabled"}>
              <span class="choice-key">${idx + 1}</span>
              <span style="${avail ? "" : "text-decoration:line-through;opacity:0.55;"}">${escHtml(avail ? opt : `${opt} (Bluff Exposed)`)}</span>
            </button>`;
        }).join("")}</div>`;
    } else if (ph === PrequelPhase.Combat) {
      deckTitle = "COMBAT TACTICS";
      const choices = pq.combatChoices();
      const preview = pq.previewCombatAction(this.hoveredCombatAction);
      deckBody = `
        <div class="choice-list">${choices.map((label, idx) => `
          <button class="btn btn-choice ${this.hoveredCombatAction === idx ? "btn-primary" : ""}" data-prequel-choice="${idx}" data-prequel-combat-idx="${idx}" ${pq.combatChoiceAvailable(idx) ? "" : "disabled"}>
            <span class="choice-key">${idx + 1}</span>
            <span>${escHtml(label)}</span>
          </button>`).join("")}</div>
        <div class="panel-inset tactical-preview" id="tactical-preview">
          <div class="tactical-preview-head">
            <span>${escHtml(preview.title)}</span>
            <span>Cost: ${preview.strikeCost} strike${preview.strikeCost > 1 ? "s" : ""}</span>
          </div>
          <div class="tactical-preview-metrics">
            <span>Dmg: <strong>${formatNum(preview.minDamage)}</strong> (Crit ${formatNum(preview.critDamage)})</span>
            <span>Counter: <strong style="color:${preview.expectedCounter > 0 ? "var(--negative)" : "var(--positive)"}">${formatNum(preview.expectedCounter)} HP</strong></span>
          </div>
          <div class="tactical-preview-note">${escHtml(preview.effect)}</div>
        </div>`;
    } else if (ph === PrequelPhase.Outcome) {
      deckTitle = "CHRONICLE";
      deckBody = `<button class="btn btn-primary btn-choice" id="btn-prequel-advance"><span class="choice-key">&crarr;</span><span>Continue [Enter]</span></button>`;
    } else if (ph === PrequelPhase.ActComplete) {
      deckTitle = `ACT ${pq.actNumber()} COMPLETE`;
      const nextLabel = pq.act === PrequelAct.Act1_Eldred
        ? "Proceed to Act II — Malvel [Enter]"
        : pq.act === PrequelAct.Act2_Malvel
          ? "Proceed to Act III — Princess Sophie [Enter]"
          : "Enter the Epilogue [Enter]";
      deckBody = `<button class="btn btn-primary btn-choice" id="btn-prequel-advance"><span class="choice-key">&crarr;</span><span>${nextLabel}</span></button>`;
    } else if (ph === PrequelPhase.Defeat) {
      deckTitle = "CHRONICLE BROKEN";
      deckBody = `
        <div class="prequel-defeat-actions">
          <button class="btn btn-primary" id="btn-prequel-retry-act">Retry Act ${pq.actNumber()} [Enter]</button>
          <button class="btn" id="btn-prequel-restart">Restart Prequel</button>
          <button class="btn" id="btn-back-menu-bottom">Return to Menu</button>
        </div>`;
    }

    const stageLabel = ph === PrequelPhase.ActIntro
      ? `ACT ${pq.actNumber()} / 3`
      : ph === PrequelPhase.ActComplete
        ? `ACT ${pq.actNumber()} COMPLETE`
        : `CH ${pq.chapterNumber()} / 15`;

    const narrativeKicker = inCombat ? "COMBAT LOG"
      : inParley ? `${pq.parley.speaker.toUpperCase()} SPEAKS`
      : ph === PrequelPhase.Defeat ? "FALLEN BEFORE THE DESCENT"
      : pq.subheading.toUpperCase();

    this.root.innerHTML = `
      <main class="screen">
        <div class="container">
          <header class="game-header">
            <div class="game-brand-block">
              <h1 class="brand-title">ASHES OF FALKREATH</h1>
              <div class="game-breadcrumb" style="color:${accent}">
                ${escHtml(prequelActTitle(pq.act))} &middot; ${escHtml(ch.name)}
              </div>
            </div>
            <div class="room-tracker" aria-label="Act chapter progress">${pips}</div>
            <div class="top-header-actions">
              <button class="btn btn-sm" id="btn-sound">${this.settings.muted ? "Sound off [V]" : "Sound on [V]"}</button>
              <button class="btn btn-sm" id="btn-back-menu" ${inParley ? "disabled" : ""}>Main Menu [Esc]</button>
            </div>
          </header>

          <div class="game-main-grid">
            <div class="chamber-column">
              <section class="panel chamber-card">
                <div class="chamber-card-header">
                  <span class="chamber-room-num">${stageLabel}</span>
                  <span class="chamber-title">${escHtml(pq.heading.toUpperCase())}</span>
                </div>
                <div class="chamber-stage aspect-square ${this.stageShake ? "shake" : ""}">
                  <img class="chamber-art" src="${imgUrl}" alt="${escHtml(pq.heading)}">
                  <div class="chamber-flash ${this.stageFlashColor ? "active" : ""}" style="background:${this.stageFlashColor || "transparent"}"></div>
                  <div class="floating-badges" id="floating-badges">${badgesHtml}</div>
                </div>
                ${combatStatusHtml}
              </section>
            </div>

            <div class="play-column">
              <section class="panel hero-strip">
                <div class="hero-strip-top">
                  <div class="hero-strip-identity">
                    <img class="hero-mini-avatar" src="${this.assets.assetUrl(ch.portrait)}" alt="${escHtml(ch.name)}">
                    <div>
                      <h2 class="hero-strip-name">${escHtml(ch.name)} <span style="font-weight:400;font-size:0.84rem;color:${accent}">&middot; ${escHtml(ch.title)}</span></h2>
                      <div class="hero-strip-vitals">
                        <span><strong>HP ${formatNum(ch.hp)} / ${formatNum(ch.maxHp)}</strong></span>
                        <span style="color:var(--muted);">${escHtml(ch.weapon)}</span>
                      </div>
                    </div>
                  </div>
                  <div class="prequel-threads" title="Chronicle Threads shape later Acts and the final Epilogue Coda">
                    <span class="thread-pill legacy">Legacy ${pq.legacy}</span>
                    <span class="thread-pill fury">Fury ${pq.fury}</span>
                    <span class="thread-pill remorse">Remorse ${pq.remorse}</span>
                  </div>
                </div>
                <div class="hero-strip-stats cols-4">${statChipsHtml}</div>
              </section>

              <section class="panel narrative-panel">
                <span class="kicker">${escHtml(narrativeKicker)}</span>
                <div class="narrative-text ${inParley ? "riddle-question" : ""}">${escHtml(pq.body)}</div>
              </section>

              <section class="panel choice-deck">
                <span class="kicker">${escHtml(deckTitle)}</span>
                ${deckBody}
              </section>
            </div>
          </div>

          <div class="hotkey-bar">
            <span>[1-4] Choose / Tactic</span>
            <span>[Enter] Continue</span>
            <span>Hover stats for prequel tooltips</span>
            <span>[Esc] Main Menu</span>
            <span>[V] Mute</span>
          </div>
        </div>
      </main>`;

    this.root.querySelectorAll("[data-prequel-choice]").forEach((btn) => {
      btn.addEventListener("click", () => this.handlePrequelChoice(Number(btn.dataset.prequelChoice)));
    });
    this.root.querySelectorAll("[data-prequel-combat-idx]").forEach((btn) => {
      const updatePreview = () => {
        const idx = Number(btn.dataset.prequelCombatIdx);
        if (this.hoveredCombatAction === idx) return;
        this.hoveredCombatAction = idx;
        const preview = pq.previewCombatAction(idx);
        const box = document.getElementById("tactical-preview");
        if (box) {
          box.innerHTML = `
            <div class="tactical-preview-head">
              <span>${escHtml(preview.title)}</span>
              <span>Cost: ${preview.strikeCost} strike${preview.strikeCost > 1 ? "s" : ""}</span>
            </div>
            <div class="tactical-preview-metrics">
              <span>Dmg: <strong>${formatNum(preview.minDamage)}</strong> (Crit ${formatNum(preview.critDamage)})</span>
              <span>Counter: <strong style="color:${preview.expectedCounter > 0 ? "var(--negative)" : "var(--positive)"}">${formatNum(preview.expectedCounter)} HP</strong></span>
            </div>
            <div class="tactical-preview-note">${escHtml(preview.effect)}</div>`;
        }
        this.root.querySelectorAll("[data-prequel-combat-idx]").forEach((b) => {
          b.classList.toggle("btn-primary", Number(b.dataset.prequelCombatIdx) === idx);
        });
      };
      btn.addEventListener("mouseenter", updatePreview);
      btn.addEventListener("focus", updatePreview);
    });

    document.getElementById("btn-prequel-advance")?.addEventListener("click", () => this.handlePrequelAdvance());
    document.getElementById("btn-prequel-retry-act")?.addEventListener("click", () => this.handlePrequelRetryAct());
    document.getElementById("btn-prequel-restart")?.addEventListener("click", () => this.handlePrequelRestart());
    document.getElementById("btn-sound")?.addEventListener("click", () => this.toggleMute());
    document.getElementById("btn-back-menu")?.addEventListener("click", () => { this.audio.playSfx("click"); this.enter(Screen.Menu); });
    document.getElementById("btn-back-menu-bottom")?.addEventListener("click", () => { this.audio.playSfx("click"); this.enter(Screen.Menu); });
  }

  renderGauntlet() {
    const g = this.gauntlet;
    const tierNames = ["Easy Tier", "Medium Tier", "Hard Tier"];
    let mainCard = "";

    if (g.finished) {
      mainCard = `
        <section class="panel" style="padding:36px 28px;text-align:center;">
          <span class="kicker">Gauntlet Complete</span>
          <h2 class="section-title" style="margin-top:8px;">Final Score: ${g.score} / ${g.deck.length}</h2>
          <p class="subtitle">Best Streak: ${g.bestStreak} &middot; Career Best: ${this.codex.bestGauntletScore} pts</p>
          <div style="display:flex;justify-content:center;gap:12px;margin-top:24px;flex-wrap:wrap;">
            <button class="btn btn-primary" id="btn-gauntlet-retry">Challenge Again</button>
            <button class="btn" id="btn-back-menu">Back to menu</button>
          </div>
        </section>`;
    } else {
      const item = g.deck[g.index];
      const pct = g.timeLimit > 0 ? Math.max(0, Math.min(100, (g.timeLeft / g.timeLimit) * 100)) : 0;
      const body = g.showingFeedback ? `
        <div class="panel-inset" style="padding:18px;border-color:${g.feedbackOk ? "var(--positive)" : "var(--negative)"};">
          <strong style="color:${g.feedbackOk ? "var(--positive)" : "var(--negative)"}">${escHtml(g.feedbackText)}</strong>
        </div>
        <button class="btn btn-primary btn-choice" id="btn-gauntlet-next"><span class="choice-key">&crarr;</span><span>Next Riddle [Enter]</span></button>
      ` : `
        <div class="riddle-timer-wrap">
          <div class="riddle-timer-head">
            <span id="riddle-timer-label">${g.timeLeft.toFixed(1)}s remaining</span>
            <span style="color:var(--muted)">${tierNames[item.tier]}</span>
          </div>
          <div class="progress-track"><div class="progress-fill" id="riddle-timer-fill" style="width:${pct}%"></div></div>
        </div>
        <div class="choice-list">${item.riddle.answers.map((ans, idx) => `
          <button class="btn btn-choice" data-g-choice="${idx}">
            <span class="choice-key">${idx + 1}</span>
            <span>${escHtml(ans)}</span>
          </button>`).join("")}</div>`;

      mainCard = `
        <section class="panel" style="padding:24px 26px;display:flex;flex-direction:column;gap:16px;">
          <span class="kicker">Riddle #${g.index + 1} of ${g.deck.length} &middot; ${tierNames[item.tier]}</span>
          <div class="narrative-text riddle-question" style="max-height:none;">${escHtml(item.riddle.question)}</div>
          ${body}
        </section>`;
    }

    this.root.innerHTML = `
      <main class="screen">
        <div class="screen-backdrop" style="background-image:url('${this.assets.assetUrl("images/startscreen.jpg")}')"></div>
        <div class="container">
          <header class="top-header">
            <div class="top-header-left">
              <h1 class="page-title">RIDDLE GAUNTLET</h1>
              <p class="subtitle">Face all 102 original riddles (Easy &rarr; Medium &rarr; Hard) with three lives.</p>
            </div>
            <div class="top-header-actions">
              <button class="btn btn-sm" id="btn-leave-gauntlet">Back to menu [Esc]</button>
            </div>
          </header>

          <div class="gauntlet-layout">
            <div class="gauntlet-stats">
              <div class="panel gauntlet-stat-box"><small>Score</small><strong>${g.score}</strong></div>
              <div class="panel gauntlet-stat-box"><small>Lives</small><strong style="color:${g.lives <= 1 ? "var(--negative)" : "var(--gold)"}">${g.lives} / 3</strong></div>
              <div class="panel gauntlet-stat-box"><small>Streak</small><strong style="color:var(--positive)">${g.streak}</strong></div>
              <div class="panel gauntlet-stat-box"><small>Best</small><strong>${Math.max(this.codex.bestGauntletScore, g.score)}</strong></div>
            </div>
            ${mainCard}
          </div>
        </div>
      </main>`;

    this.root.querySelectorAll("[data-g-choice]").forEach((b) => b.addEventListener("click", () => {
      this.answerGauntlet(Number(b.dataset.gChoice), false);
    }));
    document.getElementById("btn-gauntlet-next")?.addEventListener("click", () => { this.audio.playSfx("click"); this.nextGauntlet(); });
    document.getElementById("btn-gauntlet-retry")?.addEventListener("click", () => { this.audio.playSfx("click"); this.startGauntlet(); this.render(); });
    const leave = () => {
      if (this.gauntlet && (this.gauntlet.score > 0 || this.gauntlet.bestStreak > 0)) {
        this.codex = this.store.recordGauntletResult(this.gauntlet.score, this.gauntlet.bestStreak);
        if (this.api.isSignedIn()) this.syncCodexToCloud({ silent: true });
      }
      this.audio.playSfx("click");
      this.enter(Screen.Menu);
    };
    document.getElementById("btn-leave-gauntlet")?.addEventListener("click", leave);
    document.getElementById("btn-back-menu")?.addEventListener("click", leave);
  }

  renderOverlay() {
    if (this.overlay === Overlay.None) {
      this.overlayRoot.classList.add("hidden");
      this.overlayRoot.innerHTML = "";
      return;
    }

    let html = "";
    if (this.overlay === Overlay.Pause) {
      html = `
        <div class="panel overlay-card">
          <span class="kicker">Paused</span>
          <h2 class="section-title" style="margin-top:4px;">DESCENT PAUSED</h2>
          <p class="subtitle">Floor ${this.run.floorNumber} (${escHtml(biomeName(this.run.biome))}), Room ${this.run.roomNumber()}/9 &middot; Time ${formatDuration(this.playTimeSeconds)}</p>
          <div class="overlay-actions">
            <button class="btn btn-primary" id="ov-resume">Resume Descent [Esc]</button>
            <button class="btn" id="ov-settings">Audio &amp; Display Settings</button>
            <button class="btn" id="ov-suspend">Save &amp; Quit to Menu (Suspend Run)</button>
            <button class="btn btn-danger" id="ov-discard">Return to Main Menu (Discard Run)</button>
          </div>
        </div>`;
    } else if (this.overlay === Overlay.Map) {
      html = `
        <div class="panel overlay-card wide">
          <div class="top-header" style="margin-bottom:12px;">
            <div>
              <span class="kicker">Floor ${this.run.floorNumber} Chart</span>
              <h2 class="section-title">${escHtml(biomeName(this.run.biome))} — Room ${this.run.roomNumber()} / 9</h2>
            </div>
            <button class="btn btn-sm" id="ov-close">Close [Esc]</button>
          </div>
          <img class="map-img" src="${this.assets.assetUrl(this.run.mapImage())}" alt="Floor ${this.run.floorNumber} map">
        </div>`;
    } else if (this.overlay === Overlay.Settings) {
      html = `
        <div class="panel overlay-card">
          <div class="top-header" style="margin-bottom:8px;">
            <div>
              <span class="kicker">Preferences</span>
              <h2 class="section-title">AUDIO &amp; DISPLAY</h2>
            </div>
            <button class="btn btn-sm" id="ov-close">Close [Esc]</button>
          </div>

          <div class="settings-row">
            <div class="settings-row-head"><span>Music Volume</span><strong id="val-music">${this.settings.musicVolume}%</strong></div>
            <input id="rng-music" type="range" min="0" max="100" value="${this.settings.musicVolume}">
          </div>
          <div class="settings-row">
            <div class="settings-row-head"><span>SFX Volume</span><strong id="val-sfx">${this.settings.sfxVolume}%</strong></div>
            <input id="rng-sfx" type="range" min="0" max="100" value="${this.settings.sfxVolume}">
          </div>
          <div class="settings-row">
            <div class="settings-row-head"><span>Narration Volume</span><strong id="val-narr">${this.settings.narrationVolume}%</strong></div>
            <input id="rng-narr" type="range" min="0" max="100" value="${this.settings.narrationVolume}">
          </div>

          <div class="overlay-actions">
            <button class="btn" id="ov-mute">${this.settings.muted ? "Unmute All Audio" : "Mute All Audio"}</button>
            <button class="btn" id="ov-fullscreen">Toggle Browser Fullscreen</button>
          </div>
        </div>`;
    } else if (this.overlay === Overlay.Stats) {
      const items = [Stat.Strength, Stat.Wisdom, Stat.Observation, Stat.Agility, Stat.Health, Stat.Accuracy].map((s) => `
        <div class="panel-inset" style="padding:10px 12px;margin-bottom:8px;font-size:0.9rem;">
          ${escHtml(statTooltip(s))}
        </div>`).join("");
      html = `
        <div class="panel overlay-card">
          <div class="top-header" style="margin-bottom:12px;">
            <div>
              <span class="kicker">Reference</span>
              <h2 class="section-title">HERO ATTRIBUTES</h2>
            </div>
            <button class="btn btn-sm" id="ov-close">Close [Esc]</button>
          </div>
          ${items}
        </div>`;
    } else if (this.overlay === Overlay.Account) {
      const user = this.api.getUser();
      if (user) {
        const msUnlocked = getCodexMilestones(this.codex).filter((m) => m.unlocked).length;
        const endUnlocked = unlockedEndingCount(this.codex);
        html = `
          <div class="panel overlay-card">
            <div class="top-header" style="margin-bottom:12px;">
              <div>
                <span class="kicker">Player Account</span>
                <h2 class="section-title">${escHtml(user.username)}</h2>
              </div>
              <button class="btn btn-sm" id="ov-close">Close [Esc]</button>
            </div>
            <div class="panel-inset" style="padding:14px;display:flex;flex-direction:column;gap:8px;margin-bottom:14px;">
              <div class="kv-row"><span>Global Leaderboard Name</span><span style="color:var(--gold)">${escHtml(user.username)}</span></div>
              <div class="kv-row"><span>Endings Synced</span><span>${endUnlocked} / 4</span></div>
              <div class="kv-row"><span>Milestones Synced</span><span>${msUnlocked} / 19</span></div>
              <div class="kv-row"><span>Completed Runs</span><span>${this.codex.totalRuns}</span></div>
            </div>
            <p class="subtitle" style="margin-bottom:14px;font-size:0.88rem;">Your unlocked endings, 19 milestones, and career statistics automatically merge and sync across any browser where you sign in.</p>
            <div class="overlay-actions">
              <button class="btn btn-primary" id="ov-sync-codex" ${this.accountBusy ? "disabled" : ""}>${this.accountBusy ? "Syncing..." : "Sync Codex Now"}</button>
              <button class="btn btn-danger" id="ov-logout" ${this.accountBusy ? "disabled" : ""}>Sign Out</button>
            </div>
          </div>`;
      } else {
        const isRegister = this.accountTab === "register";
        html = `
          <div class="panel overlay-card">
            <div class="top-header" style="margin-bottom:12px;">
              <div>
                <span class="kicker">Global Account</span>
                <h2 class="section-title">${isRegister ? "CREATE ACCOUNT" : "SIGN IN"}</h2>
              </div>
              <button class="btn btn-sm" id="ov-close">Close [Esc]</button>
            </div>

            <div class="tabs-row" style="margin-bottom:14px;">
              <button class="btn btn-sm btn-tab ${!isRegister ? "active" : ""}" id="ov-tab-login" type="button">Sign In</button>
              <button class="btn btn-sm btn-tab ${isRegister ? "active" : ""}" id="ov-tab-register" type="button">Create Account</button>
            </div>

            <p class="subtitle" style="margin:0 0 14px;font-size:0.88rem;">Post runs under your username on the Global Leaderboard and sync your Codex (4 endings &amp; 19 milestones) across devices. No email required.</p>

            ${this.accountError ? `<div class="auth-error">${escHtml(this.accountError)}</div>` : ""}

            <form id="ov-auth-form" class="auth-form" autocomplete="on">
              <label class="auth-field">
                <span>Username <small>(3–20 letters, digits, _ or -)</small></span>
                <input id="ov-username" class="auth-input" type="text" minlength="3" maxlength="20" required autocomplete="username" placeholder="e.g. ShadowBlade" value="${escHtml(this.accountUsernameDraft)}">
              </label>
              <label class="auth-field">
                <span>Password <small>(at least 6 characters)</small></span>
                <input id="ov-password" class="auth-input" type="password" minlength="6" maxlength="128" required autocomplete="${isRegister ? "new-password" : "current-password"}" placeholder="Enter password">
              </label>
              ${isRegister ? `
                <label class="auth-field">
                  <span>Confirm Password</span>
                  <input id="ov-password-confirm" class="auth-input" type="password" minlength="6" maxlength="128" required autocomplete="new-password" placeholder="Re-enter password">
                </label>` : ""}
              <button class="btn btn-primary" type="submit" id="ov-auth-submit" ${this.accountBusy ? "disabled" : ""}>
                ${this.accountBusy ? (isRegister ? "Creating Account..." : "Signing In...") : (isRegister ? "Create Account &amp; Sync Codex" : "Sign In &amp; Sync Codex")}
              </button>
            </form>
          </div>`;
      }
    }

    this.overlayRoot.innerHTML = html;
    this.overlayRoot.classList.remove("hidden");

    this.overlayRoot.onclick = (e) => {
      if (e.target === this.overlayRoot) {
        this.overlay = Overlay.None;
        this.renderOverlay();
      }
    };

    document.getElementById("ov-close")?.addEventListener("click", () => { this.audio.playSfx("click"); this.overlay = Overlay.None; this.renderOverlay(); });
    document.getElementById("ov-resume")?.addEventListener("click", () => { this.audio.playSfx("click"); this.overlay = Overlay.None; this.renderOverlay(); });
    document.getElementById("ov-settings")?.addEventListener("click", () => { this.audio.playSfx("click"); this.overlay = Overlay.Settings; this.renderOverlay(); });
    document.getElementById("ov-suspend")?.addEventListener("click", () => {
      try {
        this.store.saveSuspend(this.run, Math.round(this.playTimeSeconds));
        this.audio.playSfx("click");
        this.toast("Run suspended — resume anytime from the Main Menu.");
        this.enter(Screen.Menu);
      } catch (err) {
        this.toast(err.message);
      }
    });
    document.getElementById("ov-discard")?.addEventListener("click", () => {
      this.audio.playSfx("click");
      this.run = new DungeonRun(this.assets);
      this.store.clearSuspend();
      this.suspendPreview = null;
      this.toast("Returned to Main Menu without saving.");
      this.enter(Screen.Menu);
    });
    document.getElementById("ov-mute")?.addEventListener("click", () => { this.toggleMute(); this.renderOverlay(); });
    document.getElementById("ov-fullscreen")?.addEventListener("click", () => this.toggleFullscreen());

    document.getElementById("ov-tab-login")?.addEventListener("click", () => {
      this.accountTab = "login";
      this.accountError = "";
      this.renderOverlay();
    });
    document.getElementById("ov-tab-register")?.addEventListener("click", () => {
      this.accountTab = "register";
      this.accountError = "";
      this.renderOverlay();
    });

    const userField = document.getElementById("ov-username");
    if (userField) {
      userField.focus();
      userField.addEventListener("input", (e) => {
        this.accountUsernameDraft = e.target.value;
      });
    }

    document.getElementById("ov-auth-form")?.addEventListener("submit", async (e) => {
      e.preventDefault();
      if (this.accountBusy) return;
      const username = (document.getElementById("ov-username")?.value || "").trim();
      const password = document.getElementById("ov-password")?.value || "";
      const confirm = document.getElementById("ov-password-confirm")?.value || "";
      this.accountUsernameDraft = username;

      if (this.accountTab === "register" && password !== confirm) {
        this.accountError = "Passwords do not match.";
        this.renderOverlay();
        return;
      }

      this.accountBusy = true;
      this.accountError = "";
      this.renderOverlay();

      try {
        const localCodex = this.store.loadCodex();
        const data = this.accountTab === "register"
          ? await this.api.register(username, password, localCodex)
          : await this.api.login(username, password, localCodex);

        if (data?.codex) {
          this.codex = mergeCodex(localCodex, data.codex);
          this.store.saveCodex(this.codex);
        }
        this.accountBusy = false;
        this.accountError = "";
        this.overlay = Overlay.None;
        this.audio.playSfx("loot");
        this.toast(this.accountTab === "register"
          ? `Welcome, ${data.user.username}! Account created & Codex synced.`
          : `Signed in as ${data.user.username}! Codex synced.`);
        this.renderOverlay();
        this.render();
        if ((this.screen === Screen.Leaderboard || this.screen === Screen.Results) && this.lbMode === "global") {
          this.fetchGlobalLeaderboard();
        }
      } catch (err) {
        this.accountBusy = false;
        this.accountError = err.message || "Authentication failed.";
        this.renderOverlay();
      }
    });

    document.getElementById("ov-sync-codex")?.addEventListener("click", async () => {
      if (this.accountBusy) return;
      this.accountBusy = true;
      this.renderOverlay();
      await this.syncCodexToCloud({ silent: false });
      this.accountBusy = false;
      this.renderOverlay();
    });

    document.getElementById("ov-logout")?.addEventListener("click", async () => {
      if (this.accountBusy) return;
      this.accountBusy = true;
      this.renderOverlay();
      await this.api.logout();
      this.accountBusy = false;
      this.lbMineOnly = false;
      this.overlay = Overlay.None;
      this.audio.playSfx("click");
      this.toast("Signed out. Local progress remains saved in this browser.");
      this.renderOverlay();
      this.render();
      if ((this.screen === Screen.Leaderboard || this.screen === Screen.Results) && this.lbMode === "global") {
        this.fetchGlobalLeaderboard();
      }
    });

    const bindSlider = (id, labelId, key) => {
      const el = document.getElementById(id);
      if (!el) return;
      el.addEventListener("input", () => {
        this.settings[key] = Number(el.value);
        const lbl = document.getElementById(labelId);
        if (lbl) lbl.textContent = `${this.settings[key]}%`;
        this.settings = this.store.saveSettings(this.settings);
        this.audio.applySettings(this.settings);
      });
    };
    bindSlider("rng-music", "val-music", "musicVolume");
    bindSlider("rng-sfx", "val-sfx", "sfxVolume");
    bindSlider("rng-narr", "val-narr", "narrationVolume");
  }
}

const app = new DungeonWebApp();
app.boot();
