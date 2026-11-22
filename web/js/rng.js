// Deterministic 32-bit Mersenne Twister (MT19937), FNV-1a seed parser, and
// cross-platform SplitMix64 + Fisher-Yates matching DungeonRun.cpp.

export class MT19937 {
  constructor(seed = 5489) {
    this.mt = new Uint32Array(624);
    this.index = 624;
    this.seed(seed);
  }

  seed(s) {
    let val = (Number(s) >>> 0);
    this.mt[0] = val;
    for (let i = 1; i < 624; i++) {
      const prev = this.mt[i - 1] ^ (this.mt[i - 1] >>> 30);
      this.mt[i] = (Math.imul(1812433253, prev) + i) >>> 0;
    }
    this.index = 624;
  }

  twist() {
    const UPPER = 0x80000000;
    const LOWER = 0x7fffffff;
    const MATRIX_A = 0x9908b0df;
    for (let i = 0; i < 624; i++) {
      const y = ((this.mt[i] & UPPER) | (this.mt[(i + 1) % 624] & LOWER)) >>> 0;
      this.mt[i] = (this.mt[(i + 397) % 624] ^ (y >>> 1) ^ ((y & 1) ? MATRIX_A : 0)) >>> 0;
    }
    this.index = 0;
  }

  nextUint32() {
    if (this.index >= 624) this.twist();
    let y = this.mt[this.index++];
    y ^= (y >>> 11);
    y ^= ((y << 7) & 0x9d2c5680) >>> 0;
    y ^= ((y << 15) & 0xefc60000) >>> 0;
    y ^= (y >>> 18);
    return y >>> 0;
  }

  nextInt(min, max) {
    const span = (max - min + 1) >>> 0;
    if (span <= 1) return min;
    const limit = Math.floor(0x100000000 / span) * span;
    let r = this.nextUint32();
    while (r >= limit) r = this.nextUint32();
    return min + (r % span);
  }

  shuffle(arr, start = 0, end = arr.length) {
    for (let i = end - 1; i > start; i--) {
      const j = this.nextInt(start, i);
      const tmp = arr[i];
      arr[i] = arr[j];
      arr[j] = tmp;
    }
    return arr;
  }

  serialize() {
    return { mt: Array.from(this.mt), index: this.index };
  }

  static deserialize(data) {
    const rng = new MT19937(1);
    if (data && Array.isArray(data.mt) && data.mt.length === 624) {
      rng.mt = Uint32Array.from(data.mt);
      rng.index = Number(data.index) || 0;
    }
    return rng;
  }
}

export function fnv1a32(str) {
  let h = 2166136261 >>> 0;
  for (let i = 0; i < str.length; i++) {
    h ^= (str.charCodeAt(i) & 0xff);
    h = Math.imul(h, 16777619) >>> 0;
  }
  return h === 0 ? 1 : h;
}

export function parseSeed(text) {
  const trimmed = String(text ?? "").trim();
  if (!trimmed) throw new Error("Seed text cannot be empty.");
  if (/^\d+$/.test(trimmed)) {
    const parsed = Number(trimmed);
    if (Number.isSafeInteger(parsed) && parsed >= 0 && parsed <= 0xffffffff) {
      return parsed >>> 0;
    }
  }
  return fnv1a32(trimmed);
}

export function dailyRunSeed(year, month, day) {
  const key = `DAILY-${year}-${String(month).padStart(2, "0")}-${String(day).padStart(2, "0")}`;
  return fnv1a32(key);
}

export function dailyRunDateString(date = new Date()) {
  const y = date.getFullYear();
  const m = String(date.getMonth() + 1).padStart(2, "0");
  const d = String(date.getDate()).padStart(2, "0");
  return `${y}-${m}-${d}`;
}

export function dailyRunSeedForToday(date = new Date()) {
  return dailyRunSeed(date.getFullYear(), date.getMonth() + 1, date.getDate());
}

export function randomSeed() {
  if (typeof crypto !== "undefined" && crypto.getRandomValues) {
    const buf = new Uint32Array(1);
    crypto.getRandomValues(buf);
    return (buf[0] || 1) >>> 0;
  }
  return ((Math.floor(Math.random() * 0xffffffff) >>> 0) || 1);
}

const MASK64 = 0xffffffffffffffffn;

export function roomChoiceShuffleOrder(seed, floorNumber, roomIndex, isLibrary) {
  const count = isLibrary ? 4 : 3;
  const order = [];
  for (let i = 0; i < count; i++) order.push(i);
  let mix = (BigInt(seed >>> 0) ^
    ((BigInt(floorNumber) * 0x9E3779B97F4A7C15n) & MASK64) ^
    ((BigInt(roomIndex + 1) * 0xBF58476D1CE4E5B9n) & MASK64) ^
    (isLibrary ? 0x94D049BB133111EBn : 0x2545F4914F6CDD1Dn)) & MASK64;
  mix = (mix ^ (mix >> 30n)) & MASK64;
  mix = (mix * 0xBF58476D1CE4E5B9n) & MASK64;
  mix = (mix ^ (mix >> 27n)) & MASK64;
  mix = (mix * 0x94D049BB133111EBn) & MASK64;
  mix = (mix ^ (mix >> 31n)) & MASK64;
  const seed32 = Number((mix ^ (mix >> 32n)) & 0xffffffffn) >>> 0;
  const choiceRng = new MT19937(seed32);
  for (let i = count - 1; i > 0; i--) {
    const j = choiceRng.nextUint32() % (i + 1);
    const tmp = order[i];
    order[i] = order[j];
    order[j] = tmp;
  }
  return order;
}
