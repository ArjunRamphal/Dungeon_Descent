// Web Audio engine: smooth 0.85s music crossfading, intro narration, and procedural SFX synth.

export class AudioEngine {
  constructor(assetBaseUrl = "./assets") {
    this.baseUrl = assetBaseUrl.replace(/\/+$/, "");
    this.settings = { musicVolume: 25, sfxVolume: 80, narrationVolume: 60, muted: false };
    this.currentFile = "";
    this.currentAudio = null;
    this.fadingAudio = null;
    this.fadeProgress = 1;
    this.fadeDuration = 0.85;
    this.fadeRaf = null;
    this.lastFadeTs = 0;
    this.narrationAudio = null;
    this.unlocked = false;
    this.pendingMusic = null;
    this.ctx = null;
  }

  applySettings(settings) {
    this.settings = { ...this.settings, ...settings };
    this.syncVolumes();
  }

  unlock() {
    this.unlocked = true;
    if (typeof window !== "undefined" && !this.ctx) {
      const AC = window.AudioContext || window.webkitAudioContext;
      if (AC) {
        try {
          this.ctx = new AC();
        } catch {
          this.ctx = null;
        }
      }
    }
    if (this.ctx && this.ctx.state === "suspended") {
      this.ctx.resume().catch(() => {});
    }
    if (this.pendingMusic) {
      const target = this.pendingMusic;
      this.pendingMusic = null;
      this.currentFile = "";
      this.playMusic(target);
    }
  }

  targetMusicGain() {
    return this.settings.muted ? 0 : Math.max(0, Math.min(1, this.settings.musicVolume / 100));
  }

  targetNarrationGain() {
    return this.settings.muted ? 0 : Math.max(0, Math.min(1, this.settings.narrationVolume / 100));
  }

  targetSfxGain() {
    return this.settings.muted ? 0 : Math.max(0, Math.min(1, this.settings.sfxVolume / 100));
  }

  syncVolumes() {
    const target = this.targetMusicGain();
    const f = Math.max(0, Math.min(1, this.fadeProgress));
    if (this.currentAudio) this.currentAudio.volume = target * f;
    if (this.fadingAudio) this.fadingAudio.volume = target * (1 - f);
    if (this.narrationAudio) this.narrationAudio.volume = this.targetNarrationGain();
  }

  playMusic(filename) {
    if (!filename || typeof Audio === "undefined") return;
    if (this.currentFile === filename && this.currentAudio && !this.currentAudio.paused) return;

    if (!this.unlocked) {
      this.pendingMusic = filename;
      this.currentFile = filename;
      return;
    }

    const next = new Audio(`${this.baseUrl}/music/${filename}`);
    next.loop = true;
    next.preload = "auto";

    if (this.fadingAudio) {
      try { this.fadingAudio.pause(); } catch {}
      this.fadingAudio = null;
    }

    const hadPlaying = this.currentAudio && !this.currentAudio.paused;
    this.currentFile = filename;

    if (hadPlaying) {
      this.fadingAudio = this.currentAudio;
      this.currentAudio = next;
      this.fadeProgress = 0;
      next.volume = 0;
      next.play().catch(() => {});
      this.startCrossfadeLoop();
    } else {
      if (this.currentAudio) {
        try { this.currentAudio.pause(); } catch {}
      }
      this.currentAudio = next;
      this.fadeProgress = 1;
      next.volume = this.targetMusicGain();
      next.play().catch(() => {});
    }
  }

  startCrossfadeLoop() {
    if (this.fadeRaf) cancelAnimationFrame(this.fadeRaf);
    this.lastFadeTs = performance.now();
    const step = (ts) => {
      const dt = Math.min(0.1, Math.max(0, (ts - this.lastFadeTs) / 1000));
      this.lastFadeTs = ts;
      this.fadeProgress = Math.min(1, this.fadeProgress + dt / this.fadeDuration);
      this.syncVolumes();
      if (this.fadeProgress >= 1) {
        if (this.fadingAudio) {
          try { this.fadingAudio.pause(); } catch {}
          this.fadingAudio = null;
        }
        this.fadeRaf = null;
        return;
      }
      this.fadeRaf = requestAnimationFrame(step);
    };
    this.fadeRaf = requestAnimationFrame(step);
  }

  playIntroNarration(onEnded = null) {
    if (typeof Audio === "undefined" || !this.unlocked) return;
    this.stopNarration();
    const audio = new Audio(`${this.baseUrl}/music/intro.ogg`);
    audio.loop = false;
    audio.volume = this.targetNarrationGain();
    if (onEnded) {
      audio.addEventListener("ended", () => onEnded(), { once: true });
    }
    this.narrationAudio = audio;
    audio.play().catch(() => {});
  }

  stopNarration() {
    if (this.narrationAudio) {
      try {
        this.narrationAudio.pause();
        this.narrationAudio.currentTime = 0;
      } catch {}
      this.narrationAudio = null;
    }
  }

  // Procedural WebAudio sound effects matching the desktop game's tactile audio feel
  playSfx(kind) {
    const vol = this.targetSfxGain();
    if (vol <= 0.001 || !this.ctx || this.ctx.state !== "running") return;
    const now = this.ctx.currentTime;

    const tone = (freq, endFreq, dur, type = "triangle", gainAmt = 0.25, delay = 0) => {
      const osc = this.ctx.createOscillator();
      const gain = this.ctx.createGain();
      osc.type = type;
      osc.frequency.setValueAtTime(freq, now + delay);
      if (endFreq !== freq) {
        osc.frequency.exponentialRampToValueAtTime(Math.max(20, endFreq), now + delay + dur);
      }
      gain.gain.setValueAtTime(0.0001, now + delay);
      gain.gain.linearRampToValueAtTime(gainAmt * vol, now + delay + 0.01);
      gain.gain.exponentialRampToValueAtTime(0.0001, now + delay + dur);
      osc.connect(gain);
      gain.connect(this.ctx.destination);
      osc.start(now + delay);
      osc.stop(now + delay + dur + 0.02);
    };

    switch (kind) {
      case "click":
        tone(520, 680, 0.055, "triangle", 0.14);
        break;
      case "strike":
        tone(220, 75, 0.13, "sawtooth", 0.22);
        break;
      case "crit":
        tone(340, 110, 0.18, "sawtooth", 0.26);
        tone(660, 880, 0.16, "triangle", 0.2, 0.03);
        break;
      case "guard":
        tone(420, 560, 0.12, "square", 0.14);
        tone(780, 620, 0.16, "triangle", 0.14, 0.04);
        break;
      case "ability":
        tone(290, 580, 0.16, "triangle", 0.22);
        tone(440, 880, 0.22, "sine", 0.2, 0.06);
        break;
      case "hit":
        tone(140, 55, 0.16, "sawtooth", 0.22);
        break;
      case "riddleOk":
        tone(523, 523, 0.12, "triangle", 0.18, 0);
        tone(659, 659, 0.16, "triangle", 0.18, 0.08);
        tone(784, 784, 0.22, "sine", 0.16, 0.16);
        break;
      case "riddleBad":
        tone(240, 150, 0.22, "sawtooth", 0.2);
        tone(170, 110, 0.26, "triangle", 0.18, 0.12);
        break;
      case "tick":
        tone(820, 760, 0.04, "sine", 0.1);
        break;
      case "loot":
        tone(587, 587, 0.12, "triangle", 0.18, 0);
        tone(740, 740, 0.14, "triangle", 0.18, 0.09);
        tone(880, 1175, 0.28, "sine", 0.2, 0.18);
        break;
      case "defeat":
        tone(220, 180, 0.35, "sawtooth", 0.18, 0);
        tone(165, 110, 0.5, "triangle", 0.2, 0.22);
        break;
      default:
        break;
    }
  }
}
