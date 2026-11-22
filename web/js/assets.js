// Asset loader for textfiles and 102 original riddles (works in browser via fetch and in Node via injected reader).

const STORY_FILES = [
  "textfiles/introduction/introduction.txt",
  "textfiles/entrance/entrance.txt",
  "textfiles/charIntro/WarriorIntro.txt",
  "textfiles/charIntro/RangerIntro.txt",
  "textfiles/charIntro/MageIntro.txt",
  "textfiles/charIntro/RogueIntro.txt",
  "textfiles/shopkeeper.txt",
  "textfiles/shopkeeperbook.txt",
  "textfiles/respitedrink.txt",
  "textfiles/floor2chestopen.txt",
  "textfiles/ending/goodending.txt",
  "textfiles/ending/trueending.txt",
  "textfiles/ending/badending.txt",
  "textfiles/ending/perfectending.txt",
  // Ice
  "textfiles/biomes/ice/library/icelibrary.txt",
  "textfiles/biomes/ice/chest/icechest.txt",
  "textfiles/biomes/ice/battle/icebattle.txt",
  "textfiles/biomes/ice/battle/icebattle2.txt",
  "textfiles/biomes/ice/battle/iceboss.txt",
  "textfiles/biomes/ice/encounter/iceprison.txt",
  "textfiles/biomes/ice/encounter/iceprisonfree.txt",
  "textfiles/biomes/ice/respite/icerespite.txt",
  // Jungle
  "textfiles/biomes/jungle/library/junglelibrary.txt",
  "textfiles/biomes/jungle/chest/junglechest.txt",
  "textfiles/biomes/jungle/battle/junglebattle.txt",
  "textfiles/biomes/jungle/battle/junglebattle2.txt",
  "textfiles/biomes/jungle/battle/jungleboss.txt",
  "textfiles/biomes/jungle/encounter/jungleprison.txt",
  "textfiles/biomes/jungle/encounter/jungleprisonfree.txt",
  "textfiles/biomes/jungle/respite/junglerespite.txt",
  // Desert
  "textfiles/biomes/desert/library/desertlibrary.txt",
  "textfiles/biomes/desert/chest/desertchest.txt",
  "textfiles/biomes/desert/battle/desertbattle.txt",
  "textfiles/biomes/desert/battle/desertbattle2.txt",
  "textfiles/biomes/desert/battle/desertboss.txt",
  "textfiles/biomes/desert/battle/desertbosswin.txt",
  "textfiles/biomes/desert/encounter/desertbutler.txt",
  "textfiles/biomes/desert/encounter/desertbutlerfree.txt",
  "textfiles/biomes/desert/respite/desertrespite.txt",
  "textfiles/biomes/desert/shop/desertshop.txt",
  // Ghost
  "textfiles/biomes/ghost/library/ghostlibrary.txt",
  "textfiles/biomes/ghost/chest/ghostchest.txt",
  "textfiles/biomes/ghost/battle/ghostbattle.txt",
  "textfiles/biomes/ghost/battle/ghostbattle2.txt",
  "textfiles/biomes/ghost/battle/ghostboss.txt",
  "textfiles/biomes/ghost/battle/ghostbosswin.txt",
  "textfiles/biomes/ghost/encounter/ghostbutler.txt",
  "textfiles/biomes/ghost/encounter/ghostbutlerfree.txt",
  "textfiles/biomes/ghost/respite/ghostrespite.txt",
  "textfiles/biomes/ghost/shop/ghostshop.txt",
  // Lava
  "textfiles/biomes/lava/library/lavalibrary.txt",
  "textfiles/biomes/lava/library/lavalibrarydiary.txt",
  "textfiles/biomes/lava/chest/lavachest.txt",
  "textfiles/biomes/lava/chest/lavachestopen.txt",
  "textfiles/biomes/lava/battle/lavabattle.txt",
  "textfiles/biomes/lava/battle/lavabattle2.txt",
  "textfiles/biomes/lava/battle/lavaboss.txt",
  "textfiles/biomes/lava/encounter/lavaminion.txt",
  "textfiles/biomes/lava/encounter/lavaminionfree.txt",
  "textfiles/biomes/lava/respite/lavarespite.txt",
  "textfiles/biomes/lava/shop/lavashop.txt"
];

const RIDDLE_FILES = [
  "textfiles/riddles/1/EasyRiddles.txt",
  "textfiles/riddles/2/MediumRiddles.txt",
  "textfiles/riddles/3/HardRiddles.txt"
];

function cleanText(raw) {
  let text = String(raw).replace(/\r/g, "");
  if (text.startsWith("\uFEFF")) text = text.slice(1);
  return text;
}

export function parseRiddleBank(rawText) {
  const lines = cleanText(rawText).split("\n");
  const riddles = [];
  let i = 0;
  while (i < lines.length) {
    if (!lines[i]) {
      i++;
      continue;
    }
    const question = lines[i++];
    const answers = [];
    for (let a = 0; a < 3; a++) {
      if (i >= lines.length || !lines[i]) {
        throw new Error(`Incomplete four-line riddle near "${question}"`);
      }
      answers.push(lines[i++]);
    }
    riddles.push({ question, answers, correct: 0 });
  }
  if (riddles.length < 8) {
    throw new Error("Each riddle tier requires at least 8 riddles.");
  }
  return riddles;
}

export class AssetStore {
  constructor(baseUrl = "./assets", customReader = null) {
    this.baseUrl = baseUrl.replace(/\/+$/, "");
    this.customReader = customReader;
    this.texts = new Map();
    this.sourceRiddles = [[], [], []];
  }

  assetUrl(relPath) {
    return `${this.baseUrl}/${relPath}`;
  }

  async readRaw(relPath) {
    if (this.customReader) {
      return cleanText(await this.customReader(relPath));
    }
    const resp = await fetch(this.assetUrl(relPath));
    if (!resp.ok) {
      throw new Error(`Failed to fetch asset ${relPath} (${resp.status})`);
    }
    return cleanText(await resp.text());
  }

  async preloadAll(onProgress = null) {
    const allFiles = [...RIDDLE_FILES, ...STORY_FILES];
    let loaded = 0;
    const total = allFiles.length;

    const loadBatch = async (paths) => {
      await Promise.all(paths.map(async (rel) => {
        const txt = await this.readRaw(rel);
        this.texts.set(rel, txt);
        loaded++;
        if (onProgress) onProgress(loaded / total);
      }));
    };

    // Load in gentle chunks to avoid saturating browser connection pools
    const chunkSize = 12;
    for (let i = 0; i < allFiles.length; i += chunkSize) {
      await loadBatch(allFiles.slice(i, i + chunkSize));
    }

    for (let tier = 0; tier < 3; tier++) {
      this.sourceRiddles[tier] = parseRiddleBank(this.texts.get(RIDDLE_FILES[tier]));
    }
  }

  getText(relPath) {
    const val = this.texts.get(relPath);
    if (val === undefined) {
      throw new Error(`Missing preloaded text asset: ${relPath}`);
    }
    return val;
  }
}
