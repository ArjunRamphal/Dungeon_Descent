#include "PrequelRun.h"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace dungeon {
namespace {
std::string fmtNum(float v) {
    std::ostringstream out;
    if (std::fabs(v - std::round(v)) < 0.05f) {
        out << static_cast<int>(std::round(v));
    } else {
        out.setf(std::ios::fixed);
        out.precision(1);
        out << v;
    }
    return out.str();
}
} // namespace

PrequelRun::PrequelRun(std::filesystem::path assetsRoot)
    : assets_(std::move(assetsRoot)) {}

float PrequelRun::nextUnitFloat() {
    // Deterministic xorshift32 for reproducible combat crits
    rngState_ ^= rngState_ << 13;
    rngState_ ^= rngState_ >> 17;
    rngState_ ^= rngState_ << 5;
    return static_cast<float>(rngState_ & 0xFFFFu) / 65536.f;
}

std::string PrequelRun::actTitle(PrequelAct act) {
    switch (act) {
    case PrequelAct::Act1_Eldred: return "ACT I: THE RIVER & THE LEVY";
    case PrequelAct::Act2_Malvel: return "ACT II: THE HIDDEN HEIR";
    case PrequelAct::Act3_Sophie: return "ACT III: THE SILK SMILE";
    }
    return "ASHES OF FALKREATH";
}

std::string PrequelRun::codaTitle(PrequelCoda coda) {
    switch (coda) {
    case PrequelCoda::HollowCrown: return "PRELUDE CODA: THE HOLLOW CROWN";
    case PrequelCoda::LastDoubt:   return "PRELUDE CODA: THE LAST DOUBT";
    case PrequelCoda::None:        return "ASHES OF FALKREATH";
    }
    return "ASHES OF FALKREATH";
}

std::string PrequelRun::musicFile() const {
    if (phase_ == PrequelPhase::Epilogue) {
        return coda_ == PrequelCoda::HollowCrown ? "ending_perfect.ogg" : "ending_true.ogg";
    }
    if (phase_ == PrequelPhase::Defeat) return "ending_bad.ogg";
    switch (act_) {
    case PrequelAct::Act1_Eldred:
        return phase_ == PrequelPhase::Combat ? "ice.ogg" : "DD.ogg";
    case PrequelAct::Act2_Malvel:
        return phase_ == PrequelPhase::Combat ? "lava.ogg" : "ghost.ogg";
    case PrequelAct::Act3_Sophie:
        return phase_ == PrequelPhase::Combat ? "lava.ogg" : "menu.mp3";
    }
    return "DD.ogg";
}

void PrequelRun::start() {
    ++serial_;
    rngState_ = 0x5EED1337u ^ static_cast<std::uint32_t>(serial_ * 2654435761u);
    legacy_ = 0;
    fury_ = 0;
    remorse_ = 0;
    abilitiesUsed_ = 0;
    battlesWon_ = 0;
    parleysWon_ = 0;
    coda_ = PrequelCoda::None;
    pendingAfterOutcomeCombat_ = false;
    pendingAfterOutcomeParley_ = false;
    setupAct(PrequelAct::Act1_Eldred);
}

void PrequelRun::restartAct() {
    legacy_ = actStartLegacy_;
    fury_ = actStartFury_;
    remorse_ = actStartRemorse_;
    abilitiesUsed_ = actStartAbilities_;
    battlesWon_ = actStartBattles_;
    parleysWon_ = actStartParleys_;
    pendingAfterOutcomeCombat_ = false;
    pendingAfterOutcomeParley_ = false;
    setupAct(act_);
}

void PrequelRun::setupAct(PrequelAct act) {
    act_ = act;
    actStartLegacy_ = legacy_;
    actStartFury_ = fury_;
    actStartRemorse_ = remorse_;
    actStartAbilities_ = abilitiesUsed_;
    actStartBattles_ = battlesWon_;
    actStartParleys_ = parleysWon_;
    chapterIndex_ = static_cast<int>(act) * 5;
    phase_ = PrequelPhase::ActIntro;
    choices_.clear();
    lastExchange_ = CombatExchangeResult{};

    if (act == PrequelAct::Act1_Eldred) {
        character_.act = act;
        character_.name = "Lord Eldred";
        character_.title = "Lord of House Falkreath";
        character_.weapon = "Falkreath Greatsword";
        character_.portrait = "images/prequel/eldred_pfp.jpg";
        character_.abilityName = "Rally the Banner";
        character_.abilityDesc = "Once per battle: 1.4x blow, halves counter damage, and restores 4 HP.";
        character_.power = 10.f;
        character_.resolve = 6.f;
        character_.cunning = 5.f;
        character_.agility = 6.f;
        character_.maxHp = 48.f;
        character_.hp = 48.f;

        heading_ = actTitle(act);
        subheading_ = "Years Before the Descent — The Riverlands of House Falkreath";
        sceneImage_ = "images/prequel/falkreath_river.jpg";
        body_ =
            "Before Malvel sat upon his obsidian throne, before a gilded cage hung over the magma "
            "of Mount Aethor, House Falkreath was the green breadbasket of the realm.\n\n"
            "Its fields fed the capital. Its looms clothed the King's court. And its lord, Eldred, "
            "bore two burdens no chronicle in the palace dared print: a starving province pushed to "
            "the brink by Aethoria's crown — and a secret love for the King's own daughter, Sophie.\n\n"
            "You take up Falkreath's banner as the King's final levy rides across the river bridge.";
    } else if (act == PrequelAct::Act2_Malvel) {
        character_.act = act;
        character_.name = "Malvel";
        character_.title = "Hidden Heir of Falkreath";
        character_.weapon = "Obsidian Flame";
        character_.portrait = "images/prequel/malvel_pfp.jpg";
        character_.abilityName = "Molten Ward";
        character_.abilityDesc = "Once per battle: 1.55x ward-piercing surge that negates counterattack.";
        character_.power = 11.5f + static_cast<float>(std::max(0, legacy_)) * 0.5f;
        character_.resolve = 7.f;
        character_.cunning = 7.f;
        character_.agility = 6.f;
        character_.maxHp = 52.f;
        character_.hp = 52.f;

        heading_ = actTitle(act);
        subheading_ = "After the Fall — Ruins of Falkreath & The Ancient Library of Wan";
        sceneImage_ = "images/prequel/falkreath_field.jpg";
        body_ =
            "Aethoria celebrated the end of House Falkreath as a clean victory over traitors. "
            "The King never knew Eldred had a hidden son.\n\n"
            "While the smoke still clung to the wheat fields, Malvel walked through the ashes and "
            "picked up the broken crest of his house. The Luminari mages had called his molten arts "
            "forbidden; the King's knights had called his father's defense treason.\n\n"
            "The war ended for Aethoria. For Falkreath's last heir, it is only beginning.";
        if (legacy_ >= 2) {
            body_ += "\n\n[Falkreath's Legacy]: Because Eldred safeguarded his people's forge and loyalty, "
                     "Falkreath's survivors smuggled tempered obsidian into your hands (+Power).";
        }
    } else {
        character_.act = act;
        character_.name = "Princess Sophie";
        character_.title = "Architect of the Trap";
        character_.weapon = "Royal Stiletto & Seal";
        character_.portrait = "images/prequel/sophie_pfp.jpg";
        character_.abilityName = "Sovereign Command";
        character_.abilityDesc = "Once per battle: 1.45x precision strike that staggers enemy and heals 5 HP.";
        character_.power = 11.f + static_cast<float>(std::max(0, fury_)) * 0.35f;
        character_.resolve = 8.f;
        character_.cunning = 9.f;
        character_.agility = 9.f;
        character_.maxHp = 50.f;
        character_.hp = 50.f;

        heading_ = actTitle(act);
        subheading_ = "The Eve of Dungeon Descent — Palace of Aethoria & The Obsidian Sanctum";
        sceneImage_ = "images/prequel/candlelit_throne.jpg";
        body_ =
            "The banners of victory hung across the capital for seven days. Princess Sophie smiled "
            "beside her father on the balcony, applauded the knights who returned from Falkreath, "
            "and wore silk while her heart turned to ash.\n\n"
            "In the quiet hours after midnight, she stopped weeping and began planning. Malvel "
            "had the fire beneath the mountain. She had the keys to the kingdom — and the face "
            "every hero in Aethoria would march into hell to save.";
    }
}

void PrequelRun::loadChapter(int index) {
    chapterIndex_ = index;
    phase_ = PrequelPhase::Scene;
    choices_.clear();
    pendingAfterOutcomeCombat_ = false;
    pendingAfterOutcomeParley_ = false;

    switch (index) {
    // =========================================================================
    // ACT I — ELDRED (Chapters 0..4)
    // =========================================================================
    case 0:
        heading_ = "Chapter 1: The King's Levy";
        subheading_ = "Falkreath Keep — Tax Collectors of Aethoria";
        sceneImage_ = "images/prequel/falkreath_river.jpg";
        body_ =
            "Royal riders bearing Aethoria's golden seal halt inside Falkreath's courtyard. "
            "Their parchment demands half the autumn harvest, every bolt of wool, and two hundred "
            "young farmers to march in a border war Falkreath never voted for.\n\n"
            "The shopkeeper's chronicle will one day record this afternoon as the final straw. "
            "Your villagers watch from the stone steps, waiting for your answer.";
        choices_ = {
            {"Hide half the grain for your villages and parley to stall the royal envoy"},
            {"Pay the silver from your own vault so no Falkreath farmer is conscripted"},
            {"Tear up the conscription writ before the envoy's eyes — Falkreath bends no further"}
        };
        break;

    case 1:
        heading_ = "Chapter 2: Moonlight on the River";
        subheading_ = "The Old Stone Well — Forbidden Meeting";
        sceneImage_ = "images/prequel/falkreath_river.jpg";
        body_ =
            "After midnight, willow branches brush the silver water beside Falkreath's stone well. "
            "Princess Sophie slips off her dark riding hood, her hands cold from the river mist.\n\n"
            "\"My father's council already speaks of marching on your walls, Eldred,\" she whispers. "
            "Then a twig snaps in the reeds. Stepping into the lantern light with wide, terrified eyes "
            "is the royal palace butler — he followed Sophie from the capital and heard every word.";
        choices_ = {
            {"Appeal to the butler's conscience and swear him to silence for Sophie's sake"},
            {"Warn Sophie that her father's spies already know, and press Falkreath's signet into her palm"},
            {"Confine the butler quietly until dawn so no word reaches the King tonight"}
        };
        break;

    case 2:
        heading_ = "Chapter 3: The Hidden Son";
        subheading_ = "Beneath the Mountain Cavern — Young Malvel";
        sceneImage_ = "images/prequel/obsidian_forge.jpg";
        body_ =
            "In a sulfur-scented cavern beneath the northern ridge, your secret son Malvel stands "
            "before veins of glowing magma, obsidian runes humming around his palms. No lord in "
            "Aethoria knows he exists — you kept his name off every royal ledger to protect him.\n\n"
            "\"The Luminari call this heresy,\" Malvel says, his eyes reflecting orange fire. "
            "\"Yet when Aethoria's spears reach our river, honor won't stop them. Fire will.\"";
        choices_ = {
            {"Urge restraint: \"Protect Falkreath's living people, Malvel — never become what we hate.\""},
            {"Entrust him with Falkreath's hidden subterranean maps in case the keep falls"},
            {"Embrace his strength: \"If the King brings ruin to our gate, answer him in kind.\""}
        };
        break;

    case 3:
        heading_ = "Chapter 4: The Northern Patrol";
        subheading_ = "Border Woods — Encounter with Halt the Ranger";
        sceneImage_ = "images/prequel/falkreath_field.jpg";
        body_ =
            "Scouting the northern pines before dawn, you stumble upon a lone Aethorian ranger "
            "pinned behind fallen timber by masked royal assassins. His longbow never wavers.\n\n"
            "\"Name's Halt,\" the ranger mutters, tossing you a blood-stained dispatch bearing the "
            "seals of corrupt palace nobles. \"They forged treason letters to justify burning Falkreath "
            "— and now they're framing me to bury the evidence. Watch the ridge. More riders incoming!\"";
        choices_ = {
            {"Stand shoulder-to-shoulder with Halt and break the royal vanguard scouts in battle"},
            {"Cover Halt's escape north with the forged dispatch before charging the scouts alone"}
        };
        break;

    case 4:
        heading_ = "Chapter 5: The Field of Falkreath";
        subheading_ = "Act I Finale — The King's Vanguard";
        sceneImage_ = "images/prequel/iron_captain.jpg";
        body_ =
            "By dusk, the wheat fields of Falkreath burn crimson. The King's army has broken the outer "
            "bridge — and through the smoke marches the Iron Captain, Aethoria's deadliest war-leader, "
            "with royal executioners at his back.\n\n"
            "Behind you, wagons carrying Falkreath's families and young Malvel race for the mountain pass. "
            "You plant your boots in the scorched mud and raise Falkreath's greatsword. "
            "You only need to hold the line long enough for them to disappear into the dark.";
        choices_ = {
            {"Face the Iron Captain in single combat to hold the pass"}
        };
        break;

    // =========================================================================
    // ACT II — MALVEL (Chapters 5..9)
    // =========================================================================
    case 5:
        heading_ = "Chapter 6: Oath in the Ashes";
        subheading_ = "The Ruined Keep of House Falkreath";
        sceneImage_ = "images/prequel/falkreath_field.jpg";
        body_ =
            "Rain hisses across blackened timber where Falkreath's banners once stood. "
            "You kneel in the mud where your father fell and pry Eldred's cracked signet from the ash.\n\n"
            "Aethoria's heralds already sing of a glorious conquest. None of them know that "
            "Falkreath's last heir survived — or that the Ancient Library of Wan holds the "
            "sealed manuscript of the Obsidian Focusing Seal.";
        choices_ = {
            {"Channel pure obsidian wrath — swear to reduce the King's throne to cinders"},
            {"Swear a calculated oath: patience, stealth, and precision will bring down Aethoria"},
            {"Gather Falkreath's surviving smiths to forge your dungeon sanctum first"}
        };
        break;

    case 6:
        heading_ = "Chapter 7: Raid on the Library of Wan";
        subheading_ = "Sanctuary of the Luminari";
        sceneImage_ = "images/prequel/library_of_wan.jpg";
        body_ =
            "Golden rune circles hum beneath the soaring vaults of the Ancient Library of Wan. "
            "Upon the central lectern lies the black-bound Codex of the Obsidian Seal. "
            "In the shadows of the gallery, a young Luminari scholar named Elian freezes in horror as "
            "your purple cloak steps through the shattered ward.\n\n"
            "From the inner sanctum rises Arch-Mage Solon, staff blazing with ivory light: "
            "\"You dare bring forbidden magma into the halls of the Luminari, son of Falkreath?\"";
        choices_ = {
            {"Disrupt the Luminari ward circle with a swift arcane counter-argument before dueling Solon"},
            {"Shatter the outer ward by raw obsidian fire and confront Arch-Mage Solon head-on"}
        };
        break;

    case 7:
        heading_ = "Chapter 8: Sir Henry's Stand";
        subheading_ = "The Snow-Dusted Mountain Pass";
        sceneImage_ = "images/prequel/sir_henry.jpg";
        body_ =
            "With the stolen codex tucked inside your robes, snow lashes the narrow stone bridge "
            "leading back toward Mount Aethor. Blocking the archway stands a silver-haired veteran "
            "of the King's Guard, blue lion shield raised steady against the storm.\n\n"
            "\"I am Sir Henry,\" the knight calls out over the wind. \"My squire Cassio rides an hour "
            "behind me. Turn back now, sorcerer — no dark tome passes this bridge while I draw breath.\"";
        choices_ = {
            {"Acknowledge the knight's honor, then summon your obsidian shade to break his defense"},
            {"Mock the King's Guard for serving a tyrant and strike without mercy"}
        };
        break;

    case 8:
        heading_ = "Chapter 9: The Hall of Chains";
        subheading_ = "Hunting Eldred's Killer";
        sceneImage_ = "images/prequel/iron_captain.jpg";
        body_ =
            "Three weeks later, in a rain-swept border garrison, you corner the man who drove his "
            "steel through Eldred's throat on Falkreath Field: the Iron Captain.\n\n"
            "He recognizes your father's eyes at once and draws his broadsword with a grim bark of "
            "laughter. \"So Falkreath left a pup behind after all! Finish it then, boy — give me a "
            "soldier's death!\" You smile coldly. Death is far too merciful. You need him alive in chains.";
        choices_ = {
            {"Subdue the Iron Captain in battle and drag him alive to the Hall of Chains"}
        };
        break;

    case 9:
        heading_ = "Chapter 10: The Obsidian Forge";
        subheading_ = "Act II Finale — The Visitor in the Volcano";
        sceneImage_ = "images/prequel/obsidian_forge.jpg";
        body_ =
            "Deep beneath the mountain, rivers of magma illuminate the towering obsidian pillar "
            "etched with runes from the stolen Luminari codex. The Focusing Seal pulses like a living "
            "heart — one command, and Aethoria's capital will burn.\n\n"
            "Footsteps echo along the basalt bridge. A hooded woman steps into the orange glow and "
            "lowers her veil. Golden hair. Tear-hardened eyes. Princess Sophie of Aethoria.\n\n"
            "\"My father took Eldred from both of us, Malvel,\" she says softly. \"Give me the seal. "
            "Together, we will make them watch their own kingdom crumble.\"";
        choices_ = {
            {"Kneel before Sophie's resolve: \"You will lead. I will build the trap.\""},
            {"Warn her of the cost: \"Once we light this seal, neither of us walks back into the sun.\""}
        };
        break;

    // =========================================================================
    // ACT III — PRINCESS SOPHIE (Chapters 10..14)
    // =========================================================================
    case 10:
        heading_ = "Chapter 11: The King's Triumph & The Well";
        subheading_ = "The Royal Banquet & The Riverbank";
        sceneImage_ = "images/prequel/falkreath_river.jpg";
        body_ =
            "Across the candlelit banquet hall, your father raises his goblet to the defeat of "
            "'the traitor Eldred.' Every noble cheers. Every eye turns to see whether the princess "
            "will smile.\n\n"
            "In your sleeve rests the damp, hurried letter you wrote by the river — the very parchment "
            "an adventurer will one day pull from the respite well. First, you must survive your "
            "father's scrutiny at the high table without betraying a single tremor.";
        choices_ = {
            {"Hold your silk smile and enter the Throne Room Parley with the King"}
        };
        break;

    case 11:
        heading_ = "Chapter 12: A Shadow in the Palace";
        subheading_ = "Midnight in the Royal Gallery";
        sceneImage_ = "images/prequel/candlelit_throne.jpg";
        body_ =
            "Slipping down the palace gallery to meet Malvel's courier, you freeze as a lithe shadow "
            "drops silently from the rafters beside the royal gemstone display. A young thief from the "
            "capital slums — Asher — meets your gaze, stunned motionless by the moonlight on your face.\n\n"
            "You could call the guards in a heartbeat. Yet looking at the desperate devotion sudden in "
            "his eyes, a colder thought forms: heroes come in many shapes, even from the slums.";
        choices_ = {
            {"Let Asher slip away into the night unharmed — leave the kingdom's future heroes untouched"},
            {"Drop a royal sapphire ribbon at his feet so rumor of the 'captive princess' haunts him"}
        };
        break;

    case 12:
        heading_ = "Chapter 13: Silencing the Witness";
        subheading_ = "The Sands & Crypts of Floor 2";
        sceneImage_ = "images/prequel/candlelit_throne.jpg";
        body_ =
            "Descending into the outer dungeon vaults to inspect Malvel's defenses, you hear a trembling "
            "voice behind a stone archway: the royal butler. He clutches a bundle of your intercepted "
            "letters to Malvel.\n\n"
            "\"Your Highness, please!\" he weeps. \"I kept your secret about Lord Eldred — I swear it! "
            "Don't let the sorcerer burn innocent people in the capital!\"";
        choices_ = {
            {"Leave him trapped in the shifting vault with a single canteen — fate can decide if a hero frees him"},
            {"Coldly seal the stone gate behind him: \"You knew too much. Stay with the ghosts.\""}
        };
        break;

    case 13:
        heading_ = "Chapter 14: The Candlelit Throne";
        subheading_ = "The Pact Recorded by the Scrying Orb";
        sceneImage_ = "images/prequel/candlelit_throne.jpg";
        body_ =
            "Tall candles flicker around the dark obsidian throne. Malvel stands before you holding "
            "a crimson scrying orb that records every syllable — the exact vision an adventurer will "
            "one day witness on Floor 2.\n\n"
            "\"Are you sure they'll believe this?\" Malvel asks, his voice low.\n"
            "\"They'll believe anything,\" you reply, a calm, devastating smile crossing your lips. "
            "\"This kingdom will tear itself apart looking for me. And when they do... we'll strike.\"\n\n"
            "Beside your throne lies your open leather diary. Before the abduction begins, you set "
            "quill to paper one last time.";
        choices_ = {
            {"Write the honest doubts gnawing at your soul: \"Can I live with the cost of my vengeance?\""},
            {"Write only cold certainty: \"The weapon is ready. One command, and I will rule the ashes.\""}
        };
        break;

    case 14:
        heading_ = "Chapter 15: The Pedestal of Four Weapons";
        subheading_ = "Act III Finale — The Night of the Abduction";
        sceneImage_ = "images/prequel/royal_captain.jpg";
        body_ =
            "Midnight tolls over the palace towers. Smoke curls through the torchlit marble corridor "
            "as Malvel's illusion fire splits the courtyard. Blocking the secret passage to the mountain "
            "stands the Captain of the Palace Guard, halberd leveled.\n\n"
            "Behind him, young knight Cassio lies stunned on the tiles — alive, as planned, so he will "
            "wake and swear to rescue you. Clear the Royal Captain from the corridor, seal the outer "
            "dungeon gate, and set the four weapons upon the stone pedestal.";
        choices_ = {
            {"Duel the Captain of the Palace Guard to spring the final stage of the trap"}
        };
        break;

    default:
        finalizeCampaign();
        break;
    }
}

void PrequelRun::advance() {
    if (phase_ == PrequelPhase::ActIntro) {
        loadChapter(static_cast<int>(act_) * 5);
        return;
    }
    if (phase_ == PrequelPhase::Outcome) {
        if (pendingAfterOutcomeParley_) {
            pendingAfterOutcomeParley_ = false;
            startParley(queuedParley_);
            return;
        }
        if (pendingAfterOutcomeCombat_) {
            pendingAfterOutcomeCombat_ = false;
            startCombat(queuedEnemy_.name, queuedEnemy_.title, queuedEnemy_.image,
                        queuedEnemy_.maxHp, queuedEnemy_.counterDamage, queuedEnemy_.maxStrikes,
                        queuedEnemy_.boss, queuedVictoryText_, queuedNextChapter_);
            return;
        }
        if (pendingNextChapter_ >= 15) {
            finalizeCampaign();
        } else if (pendingNextChapter_ == 5) {
            phase_ = PrequelPhase::ActComplete;
            heading_ = "ACT I COMPLETE — THE FALL OF FALKREATH";
            subheading_ = "Eldred's Last Stand";
            sceneImage_ = "images/prequel/falkreath_field.jpg";
            body_ =
                "Though you broke the Iron Captain's guard and held the bridge until the last wagon "
                "cleared the ridge, Aethoria's encircling lances closed from every side.\n\n"
                "Bleeding and surrounded, Eldred fell to one knee as the Iron Captain drove his blade "
                "through the throat of House Falkreath's lord — a memory that will haunt the captain "
                "inside a dungeon cell for years to come.\n\n"
                "Far above in the smoke of the mountain pass, Eldred's hidden son watched his father fall "
                "and whispered an oath in the dark.";
        } else if (pendingNextChapter_ == 10) {
            phase_ = PrequelPhase::ActComplete;
            heading_ = "ACT II COMPLETE — THE OBSIDIAN PACT";
            subheading_ = "The Dungeon Is Built";
            sceneImage_ = "images/prequel/obsidian_forge.jpg";
            body_ =
                "The Luminari vault is plundered. Sir Henry lies beneath the mountain snow. "
                "Eldred's killer rots alive behind the iron bars of the Hall of Chains.\n\n"
                "Beneath the volcano, the Focusing Seal burns at full strength — awaiting only the "
                "final architect who can lure Aethoria's finest heroes straight through its doors.";
        } else {
            loadChapter(pendingNextChapter_);
        }
        return;
    }
    if (phase_ == PrequelPhase::ActComplete) {
        if (act_ == PrequelAct::Act1_Eldred) setupAct(PrequelAct::Act2_Malvel);
        else if (act_ == PrequelAct::Act2_Malvel) setupAct(PrequelAct::Act3_Sophie);
        else finalizeCampaign();
        return;
    }
}

void PrequelRun::choose(std::size_t index) {
    if (phase_ != PrequelPhase::Scene || index >= choices_.size()) return;

    switch (chapterIndex_) {
    // --- ACT I ---
    case 0: {
        if (index == 0) {
            ++legacy_;
            character_.cunning += 2.f;
            body_ = "You signal the bailiffs to move half the granary downriver under cover of mist, "
                    "then step forward to match wits with the King's tax envoy before he searches the barns.";
        } else if (index == 1) {
            ++legacy_;
            character_.resolve += 2.f;
            body_ = "You empty House Falkreath's ancestral coffers onto the courtyard table so not a single "
                    "son of Falkreath marches in Aethoria's war. The royal envoy narrows his eyes and presses you.";
        } else {
            ++fury_;
            character_.power += 1.5f;
            body_ = "The torn parchment flutters across the flagstones. The envoy's guards reach for their hilts "
                    "as the envoy demands an answer to carry back to the King.";
        }
        queuedParley_ = PrequelParley{
            "Royal Tax Envoy",
            "\"Defy the Crown's levy, Lord Eldred, and the King will brand House Falkreath traitors by sunrise. "
            "By what right do you withhold grain and men from Aethoria?\"",
            {
                "\"By the Charter of the Riverlands — Falkreath feeds Aethoria in peace, never in unjust conquest.\"",
                "\"Take your threats back to your master before I toss you into the river.\"",
                "\"Perhaps the harvest simply rotted this season; check again in winter.\""
            },
            0, 3, 18.f, 18.f,
            "The envoy falters before the assembled townsfolk — the Charter is royal law, and his guards dare not "
            "draw steel before witnesses. He rides away empty-handed, buying Falkreath precious weeks (+2 Resolve, +2 Power).",
            "The envoy smirks, scribbling treason charges on his wax tablet before galloping back to the capital. "
            "War will reach Falkreath sooner than you hoped (-4 HP from mounting exhaustion, +1 Power)."
        };
        pendingAfterOutcomeParley_ = true;
        pendingNextChapter_ = 1;
        phase_ = PrequelPhase::Outcome;
        break;
    }

    case 1: {
        if (index == 0) {
            ++legacy_;
            ++remorse_;
            character_.resolve += 2.f;
            character_.hp = std::min(character_.maxHp, character_.hp + 6.f);
            body_ = "The trembling butler bows low and swears on his late mother's memory never to betray "
                    "Princess Sophie's secret to the King. Sophie squeezes your hand in relief beside the well.\n\n"
                    "Years later, that same butler will risk everything to warn a stranger in the dungeon.";
        } else if (index == 1) {
            ++remorse_;
            character_.cunning += 2.f;
            body_ = "You press Falkreath's warm silver signet into Sophie's fingers. \"Whatever comes, "
                    "remember what we fought for — not hatred, but a kinder realm.\" The butler retreats into the mist.";
        } else {
            ++fury_;
            character_.power += 1.5f;
            body_ = "Your guards escort the frightened butler to a locked guest chamber until sunrise. "
                    "No whisper reaches the palace tonight — though Sophie watches the closed door with a shadow in her eyes.";
        }
        pendingNextChapter_ = 2;
        phase_ = PrequelPhase::Outcome;
        break;
    }

    case 2: {
        if (index == 0) {
            ++legacy_;
            ++remorse_;
            character_.resolve += 2.f;
            body_ = "Malvel dims the molten sparks around his fingers, jaw tight. \"You are a better man than "
                    "the King deserves, Father.\" He pockets your words — though embers still smolder beneath the stone.";
        } else if (index == 1) {
            ++legacy_;
            character_.cunning += 2.f;
            character_.agility += 2.f;
            body_ = "You unroll the ancient subterranean charts of Mount Aethor's caverns. "
                    "\"If the keep falls, lead our people beneath the ridge where Aethoria's cavalry cannot follow.\" "
                    "Malvel memorizes every tunnel — the very tunnels that will one day become the Dungeon.";
        } else {
            ++fury_;
            character_.power += 2.f;
            body_ = "Malvel's eyes flare bright as furnace coal. \"Then let the King learn what sleeps beneath "
                    "his precious mountains.\" The obsidian runes hum louder in the dark.";
        }
        pendingNextChapter_ = 3;
        phase_ = PrequelPhase::Outcome;
        break;
    }

    case 3: {
        if (index == 0) {
            ++legacy_;
            character_.power += 1.5f;
            body_ = "You step beside Halt as his bowstring sings through the pines. \"Watch your left flank, "
                    "Lord Eldred!\" Together you meet the corrupt nobles' Vanguard Scouts in the clearing.";
        } else {
            ++legacy_;
            character_.agility += 2.f;
            body_ = "\"Ride north, Ranger! Keep that dispatch alive!\" Halt salutes once and vanishes into the "
                    "snowy timber — surviving to clear his name years later — as the Vanguard Scouts surround you.";
        }
        queuedEnemy_ = PrequelEnemy{};
        queuedEnemy_.name = "Royal Vanguard Scouts";
        queuedEnemy_.title = "Corrupt Cabal's Outriders";
        queuedEnemy_.image = "images/prequel/falkreath_field.jpg";
        queuedEnemy_.maxHp = 58.f;
        queuedEnemy_.counterDamage = 6.f;
        queuedEnemy_.maxStrikes = 6;
        queuedEnemy_.boss = false;
        queuedVictoryText_ =
            "The vanguard riders scatter into the mist, leaving behind Aethoria's battle plans for Falkreath Field. "
            "Halt's escape into the northern wilds is assured — though his name is already being branded traitor in the capital. "
            "(+6 HP recovered, +1.5 Power)";
        queuedNextChapter_ = 4;
        pendingAfterOutcomeCombat_ = true;
        phase_ = PrequelPhase::Outcome;
        break;
    }

    case 4: {
        startCombat("The Iron Captain", "Aethoria's Battle-Hardened Vanguard Commander",
                    "images/prequel/iron_captain.jpg",
                    86.f, 7.5f, 7, true,
                    "With a thunderous clash of steel, you shatter the Iron Captain's helmet visor and drive him "
                    "back across the scorched bridge! The wagons bearing Falkreath's survivors and young Malvel "
                    "clear the mountain ridge in safety — your duty as Lord of Falkreath is fulfilled.",
                    5);
        break;
    }

    // --- ACT II ---
    case 5: {
        if (index == 0) {
            ++fury_;
            character_.power += 2.f;
            body_ = "Obsidian fire scorches the rain into steam around your fists. The era of mercy died "
                    "with Eldred on Falkreath Field. Now Aethoria will learn to fear the dark.";
        } else if (index == 1) {
            ++legacy_;
            character_.cunning += 2.f;
            character_.agility += 2.f;
            body_ = "Anger without precision gets men killed. You slip Eldred's signet onto your finger "
                    "and plan your infiltration of the Ancient Library of Wan down to the second.";
        } else {
            ++legacy_;
            character_.maxHp += 6.f;
            character_.hp = character_.maxHp;
            body_ = "Beneath Mount Aethor's basalt arches, surviving smiths of House Falkreath temper your "
                    "staff with volcanic glass. Their loyalty steels your resolve (+6 Max HP).";
        }
        pendingNextChapter_ = 6;
        phase_ = PrequelPhase::Outcome;
        break;
    }

    case 6: {
        if (index == 0) {
            character_.cunning += 1.5f;
            body_ = "You step beneath the hovering blue rune circle and chant a counter-harmonic from the "
                    "forbidden texts, challenging Arch-Mage Solon's ward before steel or flame is drawn.";
            queuedParley_ = PrequelParley{
                "Arch-Mage Solon of the Luminari",
                "\"The Luminari sealed the Obsidian Codex to protect the realm from volcanic annihilation! "
                "Speak the counter-seal's true weakness, intruder, or be bound in golden chains!\"",
                {
                    "\"Your ward feeds on fear — invert the inner triad with molten resonance and the circle shatters!\"",
                    "\"Overload the outer candles with frost magic!\"",
                    "\"Burn the bookshelves until the smoke blinds the Luminari!\""
                },
                0, 3, 18.f, 18.f,
                "The Luminari ward fractures in a shower of blue sparks! Young scholar Elian dives behind a pillar "
                "as Arch-Mage Solon's barrier weakens before the duel begins (+2 Power, +2 Resolve).",
                "Solon's ward lashes back with radiant lightning (-5 HP), forcing you into combat with his barrier intact."
            };
            queuedEnemy_ = PrequelEnemy{};
            queuedEnemy_.name = "Arch-Mage Solon";
            queuedEnemy_.title = "Master of the Luminari";
            queuedEnemy_.image = "images/prequel/archmage_solon.jpg";
            queuedEnemy_.maxHp = 84.f;
            queuedEnemy_.counterDamage = 7.5f;
            queuedEnemy_.maxStrikes = 7;
            queuedEnemy_.boss = true;
            queuedVictoryText_ =
                "Arch-Mage Solon's golden staff clatters across the marble tiles. You seize the black-bound "
                "Codex of the Obsidian Seal from the lectern while young Elian stares from the gallery — "
                "swearing the Luminari will hunt you to the ends of Aethoria. (+8 HP recovered, +2 Power)";
            queuedNextChapter_ = 7;
            pendingAfterOutcomeParley_ = true;
            pendingAfterOutcomeCombat_ = true;
            phase_ = PrequelPhase::Outcome;
        } else {
            ++fury_;
            character_.power += 2.f;
            startCombat("Arch-Mage Solon", "Master of the Luminari",
                        "images/prequel/archmage_solon.jpg",
                        90.f, 8.f, 7, true,
                        "Golden wards shatter under waves of obsidian flame! You claim the Codex of the Obsidian Seal "
                        "as young Elian drags the wounded Arch-Mage to cover — cementing your blood feud with the Luminari. "
                        "(+8 HP recovered, +2 Power)",
                        7);
        }
        break;
    }

    case 7: {
        if (index == 0) {
            ++remorse_;
            character_.resolve += 2.f;
            body_ = "\"Step aside, Sir Henry,\" you murmur as snow swirls across the stone arch. \"My quarrel "
                    "is with the King who butchered Falkreath, not with a decent knight.\" Sir Henry raises his shield: "
                    "\"My oath is my life, sorcerer. Draw!\"";
        } else {
            ++fury_;
            character_.power += 2.f;
            body_ = "\"The King's Guard stood silent while Falkreath burned!\" Obsidian shadows twist behind "
                    "you into a towering phantom as Sir Henry plants his boots on the icy bridge.";
        }
        queuedEnemy_ = PrequelEnemy{};
        queuedEnemy_.name = "Sir Henry";
        queuedEnemy_.title = "Knight of the King's Guard — Cassio's Mentor";
        queuedEnemy_.image = "images/prequel/sir_henry.jpg";
        queuedEnemy_.maxHp = 94.f;
        queuedEnemy_.counterDamage = 8.f;
        queuedEnemy_.maxStrikes = 8;
        queuedEnemy_.boss = true;
        queuedVictoryText_ =
            "Sir Henry's lion shield splits in two under your final surge of magma. The silver-haired knight "
            "falls quietly onto the snow-dusted stone, whispering a prayer for his young squire Cassio.\n\n"
            "An hour later, Cassio will find his mentor's cold dragon-steel sword on this bridge — and swear "
            "to follow Malvel into the darkest dungeon on earth. (+8 HP recovered, +1.5 Power)";
        queuedNextChapter_ = 8;
        pendingAfterOutcomeCombat_ = true;
        phase_ = PrequelPhase::Outcome;
        break;
    }

    case 8: {
        startCombat("The Iron Captain", "Slayer of Lord Eldred",
                    "images/prequel/iron_captain.jpg",
                    102.f, 8.5f, 8, true,
                    "Disarmed and bound in iron shackles, Eldred's killer glares up through bloodied teeth. "
                    "\"Kill me, Falkreath!\" You shake your head and drag him down into the freezing dark of the "
                    "Hall of Chains: \"Live long enough to watch what your King's greed built.\" (+8 HP recovered)",
                    9);
        break;
    }

    case 9: {
        if (index == 0) {
            ++fury_;
            body_ = "You bow your head before the glowing obsidian obelisk. \"Your father built his throne "
                    "on Falkreath's bones, Sophie. Command me, and we will give Aethoria the reckoning it earned.\"";
        } else {
            ++remorse_;
            body_ = "Sophie stares into the churning lava below the Focusing Seal. \"I already lost the sun "
                    "the day my father murdered Eldred,\" she whispers. \"Now finish the seal.\"";
        }
        pendingNextChapter_ = 10;
        phase_ = PrequelPhase::Outcome;
        break;
    }

    // --- ACT III ---
    case 10: {
        startParley(PrequelParley{
            "The King of Aethoria",
            "\"You have been unusually quiet since Falkreath's fall, Sophie. Rumors say you met their traitor "
            "lord by the river before the battle. Look your father in the eye and answer me!\"",
            {
                "\"I walked by the river to pray for Aethoria's soldiers, Father — never doubt my loyalty to this crown.\"",
                "\"Eldred was twice the ruler you will ever be!\"",
                "\"Ask the royal butler where I was — he followed me everywhere!\""
            },
            0, 3, 18.f, 18.f,
            "Your smile never flickers. The King laughs warmly, satisfied, and raises his cup to his 'dutiful daughter.' "
            "That very night you slip down to the riverbank, weep one final time over your letter, and hide it deep in the respite well (+2 Cunning, +2 Power).",
            "The King's eyes narrow with cold suspicion, doubling the guards outside your tower (-5 HP from stress). "
            "Still, you slip past them at midnight and bury your tear-stained letter inside the respite well."
        });
        pendingNextChapter_ = 11;
        break;
    }

    case 11: {
        if (index == 0) {
            ++remorse_;
            character_.cunning += 2.f;
            body_ = "You raise one quiet finger to your lips. Asher blinks in disbelief, pockets nothing, "
                    "and slips back into the shadows — carrying a sudden, fierce longing to become someone worthy "
                    "of the princess who spared him.";
        } else {
            ++fury_;
            character_.power += 2.f;
            body_ = "You let a sapphire ribbon fall onto the marble floor before turning away without a word. "
                    "Asher catches it like a sacred relic — another pawn bound to descend when the 'abduction' begins.";
        }
        pendingNextChapter_ = 12;
        phase_ = PrequelPhase::Outcome;
        break;
    }

    case 12: {
        if (index == 0) {
            ++remorse_;
            ++legacy_;
            character_.resolve += 2.f;
            character_.hp = std::min(character_.maxHp, character_.hp + 6.f);
            body_ = "Your hand trembles on the iron lever as you toss a canteen and small lamp into the "
                    "chamber before locking the gate. \"Survive, if you can,\" you whisper. "
                    "If a worthy hero enters these halls, the butler will live to speak the truth.";
        } else {
            ++fury_;
            character_.power += 2.f;
            body_ = "You turn the heavy iron key without looking back as the butler's pleas fade behind the "
                    "shifting sands and crypt mist. No secret can be allowed to reach the palace early.";
        }
        pendingNextChapter_ = 13;
        phase_ = PrequelPhase::Outcome;
        break;
    }

    case 13: {
        if (index == 0) {
            ++remorse_;
            character_.resolve += 2.f;
            character_.hp = std::min(character_.maxHp, character_.hp + 6.f);
            body_ = "Ink darkens the leather-bound page: \"I tell myself this is for Falkreath, but doubts "
                    "creep in... I wonder if I can live with the cost of my vengeance.\" "
                    "You leave the diary in the lava library — perhaps hoping someone will read it in time.";
        } else {
            ++fury_;
            character_.power += 2.5f;
            body_ = "Your handwriting slashes across the parchment: \"It is finished. One command, and "
                    "nothing will survive. I will be the one to rule over the ashes.\" "
                    "Malvel bows deeply as the crimson orb fades cold.";
        }
        pendingNextChapter_ = 14;
        phase_ = PrequelPhase::Outcome;
        break;
    }

    case 14: {
        startCombat("Captain of the Palace Guard", "Shield of the Royal Corridor",
                    "images/prequel/royal_captain.jpg",
                    106.f, 8.5f, 8, true,
                    "With a flash of silver steel and molten light, you disarm the Palace Captain and slip past "
                    "the stunned form of young knight Cassio — leaving him unharmed to carry the alarm to the King.\n\n"
                    "Moments later, inside the damp entrance chamber beneath the mountains, you and Malvel lay "
                    "four gleaming weapons upon the mossy stone pedestal: a runic sword, a throwing knife, "
                    "a mystical wand, and a curved hunting longbow.",
                    15);
        break;
    }

    default:
        break;
    }
}

void PrequelRun::startParley(const PrequelParley& p) {
    parley_ = p;
    const float bonusSec = std::floor(character_.resolve / 2.f);
    parley_.timeLimit = p.timeLimit + bonusSec;
    parley_.timeLeft = parley_.timeLimit;
    parley_.hiddenIndex = 3;
    // High Cunning (>= 8) reveals & eliminates one weak bluff automatically!
    if (character_.cunning >= 8.f) {
        parley_.hiddenIndex = (parley_.bestIndex + 2) % 3;
    }
    phase_ = PrequelPhase::Parley;
    heading_ = "TACTICAL PARLEY — " + parley_.speaker;
    subheading_ = "Choose your response before time expires";
    body_ = parley_.prompt;
}

bool PrequelRun::parleyOptionAvailable(std::size_t index) const {
    return index < 3 && index != parley_.hiddenIndex;
}

void PrequelRun::tickParley(float dt) {
    if (phase_ != PrequelPhase::Parley) return;
    parley_.timeLeft = std::max(0.f, parley_.timeLeft - std::max(0.f, dt));
    if (parley_.timeLeft <= 0.f) {
        answerParley(99); // timeout
    }
}

void PrequelRun::answerParley(std::size_t index) {
    if (phase_ != PrequelPhase::Parley) return;
    const bool ok = (index == parley_.bestIndex);
    if (ok) {
        ++parleysWon_;
        ++legacy_;
        character_.power += 2.f;
        character_.resolve += 2.f;
        body_ = parley_.successText;
        heading_ = "PARLEY SUCCEEDED";
        subheading_ = parley_.speaker;
    } else {
        character_.hp = std::max(6.f, character_.hp - 5.f);
        body_ = (index >= 3 ? "Hesitation cost you the moment! " : "") + parley_.failText;
        heading_ = "PARLEY FALTERED";
        subheading_ = parley_.speaker;
    }
    phase_ = PrequelPhase::Outcome;
}

void PrequelRun::startCombat(const std::string& name, const std::string& title, const std::string& img,
                             float hp, float counterDmg, int baseStrikes, bool boss,
                             std::string victoryText, int nextAfterWin) {
    enemy_ = PrequelEnemy{};
    enemy_.name = name;
    enemy_.title = title;
    enemy_.image = img;
    enemy_.maxHp = hp;
    enemy_.hp = hp;
    enemy_.counterDamage = counterDmg;
    const int bonusStrikes = static_cast<int>(std::floor(character_.agility / 3.f));
    enemy_.maxStrikes = baseStrikes + bonusStrikes;
    enemy_.strikesLeft = enemy_.maxStrikes;
    enemy_.turn = 0;
    enemy_.boss = boss;
    enemy_.abilityUsedThisBattle = false;
    enemy_.rallyGuardActive = false;

    pendingVictoryText_ = std::move(victoryText);
    pendingNextChapter_ = nextAfterWin;
    sceneImage_ = img;
    heading_ = name;
    subheading_ = title;
    body_ = "Choose your combat tactic. Watch for telegraphed boss intents — Guard Strike dampens lethal "
            "counters, Heavy Strike trades two strikes for burst power, and your signature ability can turn the tide once per battle.";
    lastExchange_ = CombatExchangeResult{};
    updateBossIntent();
    phase_ = PrequelPhase::Combat;
}

void PrequelRun::updateBossIntent() {
    BossIntent intent;
    if (!enemy_.boss) {
        intent.name = "Skirmish Counter";
        intent.detail = "Standard retaliation.";
        enemy_.intent = intent;
        return;
    }
    const int cycle = enemy_.turn % 3;
    if (cycle == 1) {
        intent.telegraphed = true;
        intent.counterMultiplier = 1.65f;
        intent.name = "TELEGRAPHED: Crushing Counter!";
        intent.detail = "1.65x retaliation this turn — use Guard Strike or your signature ability!";
    } else if (cycle == 2) {
        intent.telegraphed = true;
        intent.damageTakenMultiplier = 0.72f;
        intent.counterMultiplier = 0.9f;
        intent.name = "Defensive Stance";
        intent.detail = "Armor/ward raised (0.72x normal damage — signature abilities pierce it).";
    } else {
        intent.telegraphed = false;
        intent.name = "Measured Stance";
        intent.detail = "Standard retaliation.";
    }
    enemy_.intent = intent;
}

bool PrequelRun::combatChoiceAvailable(CombatAction action) const {
    if (phase_ != PrequelPhase::Combat) return false;
    if (action == CombatAction::HeavyStrike) return enemy_.strikesLeft >= 2;
    if (action == CombatAction::ClassAbility) return !enemy_.abilityUsedThisBattle && enemy_.strikesLeft >= 1;
    return enemy_.strikesLeft >= 1;
}

CombatActionPreview PrequelRun::previewCombatAction(CombatAction action) const {
    CombatActionPreview p;
    p.available = combatChoiceAvailable(action);
    const float resolveMitigation = std::max(0.65f, 1.f - character_.resolve * 0.025f);
    const float baseCounter = enemy_.counterDamage * enemy_.intent.counterMultiplier * resolveMitigation;
    const float wardMult = enemy_.intent.damageTakenMultiplier;

    switch (action) {
    case CombatAction::StandardStrike:
        p.title = "Standard Strike";
        p.strikeCost = 1;
        p.minDamage = character_.power * wardMult;
        p.critDamage = p.minDamage * 1.9f;
        p.expectedCounter = baseCounter;
        p.effect = "Reliable strike with standard retaliation.";
        break;
    case CombatAction::HeavyStrike:
        p.title = "Heavy Strike";
        p.strikeCost = 2;
        p.minDamage = character_.power * 1.9f * wardMult;
        p.critDamage = p.minDamage * 1.9f;
        p.expectedCounter = baseCounter * 1.2f;
        p.effect = "Costs 2 strikes, 1.9x damage, 1.2x counter.";
        break;
    case CombatAction::GuardStrike:
        p.title = "Guard Strike";
        p.strikeCost = 1;
        p.minDamage = character_.power * 0.78f * wardMult;
        p.critDamage = p.minDamage * 1.9f;
        p.expectedCounter = baseCounter * 0.35f;
        p.effect = "0.78x damage, cuts retaliation by 65%.";
        break;
    case CombatAction::ClassAbility:
        p.title = character_.abilityName;
        p.strikeCost = 1;
        if (character_.act == PrequelAct::Act1_Eldred) {
            p.minDamage = character_.power * 1.4f;
            p.expectedCounter = baseCounter * 0.4f;
            p.effect = "Rally the Banner: pierces stance, 0.4x counter, +4 HP.";
        } else if (character_.act == PrequelAct::Act2_Malvel) {
            p.minDamage = character_.power * 1.55f;
            p.expectedCounter = 0.f;
            p.effect = "Molten Ward: 1.55x ward-piercing surge, blocks counter.";
        } else {
            p.minDamage = character_.power * 1.45f;
            p.expectedCounter = 0.f;
            p.effect = "Sovereign Command: 1.45x strike, staggers enemy, +5 HP.";
        }
        p.critDamage = p.minDamage * 1.9f;
        break;
    }
    return p;
}

void PrequelRun::attack(CombatAction action) {
    if (!combatChoiceAvailable(action)) return;

    const auto preview = previewCombatAction(action);
    CombatExchangeResult ex;
    ex.valid = true;
    ex.action = action;
    ex.strikesSpent = preview.strikeCost;

    const float critChance = std::min(0.45f, character_.cunning / 32.f);
    const bool crit = nextUnitFloat() < critChance;
    ex.critical = crit;

    float dmg = crit ? preview.critDamage : preview.minDamage;
    dmg = std::round(dmg * 10.f) / 10.f;

    enemy_.strikesLeft = std::max(0, enemy_.strikesLeft - preview.strikeCost);
    enemy_.hp = std::max(0.f, enemy_.hp - dmg);
    ex.damageDealt = dmg;

    float healed = 0.f;
    bool counterNegated = false;
    if (action == CombatAction::ClassAbility) {
        enemy_.abilityUsedThisBattle = true;
        ++abilitiesUsed_;
        if (character_.act == PrequelAct::Act1_Eldred) {
            const float before = character_.hp;
            character_.hp = std::min(character_.maxHp, character_.hp + 4.f);
            healed = character_.hp - before;
        } else if (character_.act == PrequelAct::Act3_Sophie) {
            const float before = character_.hp;
            character_.hp = std::min(character_.maxHp, character_.hp + 5.f);
            healed = character_.hp - before;
        }
        if (preview.expectedCounter <= 0.01f) counterNegated = true;
    }

    float counter = 0.f;
    if (enemy_.hp > 0.f) {
        counter = std::round(preview.expectedCounter * 10.f) / 10.f;
        if (counter > 0.f) {
            character_.hp = std::max(0.f, character_.hp - counter);
            ex.counterattacked = true;
            ex.counterDamage = counter;
        }
        if (action == CombatAction::GuardStrike) ex.guarded = true;
    } else {
        ex.enemyDefeated = true;
    }

    std::ostringstream log;
    log << character_.name << " uses " << preview.title
        << " for " << fmtNum(dmg) << (crit ? " CRIT" : "") << " damage!";
    if (healed > 0.f) {
        log << " Recovered +" << fmtNum(healed) << " HP.";
    }
    if (enemy_.hp <= 0.f) {
        log << "\n" << enemy_.name << " is defeated!";
    } else if (counterNegated) {
        log << "\n" << enemy_.name << "'s retaliation is completely staggered!";
    } else {
        log << "\n" << enemy_.name << " retaliates for " << fmtNum(counter) << " HP.";
    }
    const std::string exchangeLog = log.str();

    if (enemy_.hp <= 0.f) {
        lastExchange_ = ex;
        ++battlesWon_;
        character_.hp = std::min(character_.maxHp, character_.hp + 8.f);
        character_.power += 1.5f;
        body_ = exchangeLog + "\n\n" + pendingVictoryText_;
        heading_ = "VICTORY — " + enemy_.name;
        phase_ = PrequelPhase::Outcome;
        return;
    }

    if (character_.hp <= 0.f || enemy_.strikesLeft <= 0) {
        ex.playerDefeated = true;
        lastExchange_ = ex;
        heading_ = "DEFEATED";
        subheading_ = enemy_.name + " prevailed";
        body_ = exchangeLog + "\n\n" +
                (character_.hp <= 0.f
                     ? "Your strength gives out before the chronicle could reach its end."
                     : "Your strike window closed before you could break the enemy's line.") +
                "\n\nRetry this Act to press onward through Ashes of Falkreath.";
        phase_ = PrequelPhase::Defeat;
        return;
    }

    lastExchange_ = ex;
    ++enemy_.turn;
    updateBossIntent();
    body_ = exchangeLog;
}

void PrequelRun::finalizeCampaign() {
    phase_ = PrequelPhase::Epilogue;
    sceneImage_ = "images/prequel/pedestal_chamber.jpg";
    choices_.clear();

    if (remorse_ >= fury_) {
        coda_ = PrequelCoda::LastDoubt;
        heading_ = codaTitle(coda_);
        subheading_ = "The Pedestal of Four Weapons — Minutes Before Dungeon Descent";
        body_ =
            "Two torches hiss against damp moss-grown stone. Upon the central pedestal gleam four "
            "weapons laid in silent symmetry: a runic dragon-steel sword, a throwing knife, a carved "
            "wooden wand, and a curved hunting longbow.\n\n"
            "Princess Sophie pauses at the threshold of the molten descent, her hand resting lightly "
            "on Eldred's river signet hidden inside her cloak. Beneath her silk smile, a quiet prayer "
            "lingers — that whoever wakes in this chamber will uncover the letter, the book, and the "
            "diary she left behind, and find a justice cleaner than her fire.\n\n"
            "The iron gate seals shut. Above the obsidian throne, the gilded cage swings closed.\n\n"
            "Somewhere in the damp dark, a groggy adventurer slowly opens their eyes...";
    } else {
        coda_ = PrequelCoda::HollowCrown;
        heading_ = codaTitle(coda_);
        subheading_ = "The Pedestal of Four Weapons — Minutes Before Dungeon Descent";
        body_ =
            "Two torches flare against the damp brick walls. Upon the carved stone pedestal rest four "
            "weapons chosen for Aethoria's proudest champions: a runic sword for Sir Henry's squire, "
            "a wand for the Luminari, a longbow for the exiled ranger, and a knife for the palace thief.\n\n"
            "Malvel seals the outer portcullis with a twist of obsidian flame. Beside him, Princess "
            "Sophie does not look back. No mercy remains for the crown that buried House Falkreath — "
            "only the Focusing Seal burning beneath the lava, waiting for one final command.\n\n"
            "She steps willingly into the gilded cage and wears her most convincing smile.\n\n"
            "Somewhere in the damp dark, a groggy adventurer slowly opens their eyes...";
    }

    if (ironChronicleEligible()) {
        body_ += "\n\n[IRON CHRONICLE ACHIEVED]: Completed all three Acts without using a single signature ability in combat!";
    }
}

} // namespace dungeon
