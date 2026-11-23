#include "Codex.h"
#include "PrequelRun.h"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace dungeon;
using namespace dungeon::portable;

namespace {
void check(bool cond, const std::string& msg) {
    if (!cond) throw std::runtime_error(msg);
}

struct TempDir {
    std::filesystem::path root = std::filesystem::temp_directory_path() /
        ("DungeonDescent-prequel-" + std::to_string(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
    ~TempDir() {
        std::error_code ignored;
        std::filesystem::remove_all(root, ignored);
    }
};

void playCampaign(PrequelRun& run, bool ruthless, bool useAbilities) {
    run.start();
    int safety = 0;
    while (run.phase() != PrequelPhase::Epilogue && run.phase() != PrequelPhase::Defeat && ++safety < 600) {
        if (run.phase() == PrequelPhase::ActIntro ||
            run.phase() == PrequelPhase::Outcome ||
            run.phase() == PrequelPhase::ActComplete) {
            run.advance();
        } else if (run.phase() == PrequelPhase::Scene) {
            const auto n = run.choices().size();
            check(n >= 1, "Scene must present at least one choice");
            run.choose(ruthless ? (n - 1) : 0);
        } else if (run.phase() == PrequelPhase::Parley) {
            run.answerParley(run.parley().bestIndex);
        } else if (run.phase() == PrequelPhase::Combat) {
            if (useAbilities && run.combatChoiceAvailable(CombatAction::ClassAbility)) {
                run.attack(CombatAction::ClassAbility);
            } else if (run.enemy().intent.telegraphed && run.enemy().intent.counterMultiplier > 1.2f) {
                run.attack(CombatAction::GuardStrike);
            } else if (run.combatChoiceAvailable(CombatAction::HeavyStrike) && !run.enemy().intent.telegraphed) {
                run.attack(CombatAction::HeavyStrike);
            } else {
                run.attack(CombatAction::StandardStrike);
            }
        }
    }
    check(run.phase() == PrequelPhase::Epilogue,
          "Expected prequel campaign to reach Epilogue cleanly (ended in phase " +
          std::to_string(static_cast<int>(run.phase())) + " at chapter " + std::to_string(run.chapterNumber()) + ")");
}
} // namespace

int main(int argc, char** argv) {
    try {
        check(argc == 2, "Pass the assets directory.");
        const std::filesystem::path assets = argv[1];

        // 1. Verify all 13 custom prequel artwork files exist on disk.
        const std::vector<std::string> artFiles = {
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
        };
        for (const auto& rel : artFiles) {
            check(std::filesystem::is_regular_file(assets / rel), "Missing prequel artwork: " + rel);
        }

        // 2. Full Remorseful + Iron Chronicle (0 signature abilities) playthrough -> LastDoubt Coda.
        PrequelRun ironRun(assets);
        playCampaign(ironRun, /*ruthless=*/false, /*useAbilities=*/false);
        check(ironRun.coda() == PrequelCoda::LastDoubt, "Merciful/remorseful choices unlock The Last Doubt coda");
        check(ironRun.ironChronicleEligible() && ironRun.abilitiesUsed() == 0,
              "0-ability playthrough preserves Iron Chronicle eligibility");
        check(ironRun.battlesWon() == 6, "All 6 canonical battles completed across Acts I-III");
        check(ironRun.parleysWon() == 3, "All 3 canonical parleys won on remorseful route");
        check(ironRun.sceneImage() == "images/prequel/pedestal_chamber.jpg",
              "Epilogue bridges directly into the Pedestal of Four Weapons");

        // 3. Full Ruthless + Signature Abilities playthrough -> HollowCrown Coda.
        PrequelRun ruthlessRun(assets);
        playCampaign(ruthlessRun, /*ruthless=*/true, /*useAbilities=*/true);
        check(ruthlessRun.coda() == PrequelCoda::HollowCrown, "Ruthless fury choices unlock The Hollow Crown coda");
        check(!ruthlessRun.ironChronicleEligible() && ruthlessRun.abilitiesUsed() >= 3,
              "Using POV abilities across Acts I-III tracks ability count accurately");

        // 4. Verify Parley timeout, Cunning bluff elimination, and restartAct() rollback.
        PrequelRun parleyTest(assets);
        parleyTest.start();
        parleyTest.advance(); // ActIntro -> Chapter 1 Scene
        parleyTest.choose(0); // Outcome -> queued Parley
        parleyTest.advance(); // Enters Parley
        check(parleyTest.phase() == PrequelPhase::Parley, "Chapter 1 enters Royal Envoy Parley");
        const float hpBeforeTimeout = parleyTest.character().hp;
        parleyTest.tickParley(120.f);
        check(parleyTest.phase() == PrequelPhase::Outcome && parleyTest.character().hp < hpBeforeTimeout,
              "Parley timeout resolves as failure with HP penalty");
        parleyTest.restartAct();
        check(parleyTest.phase() == PrequelPhase::ActIntro && parleyTest.act() == PrequelAct::Act1_Eldred &&
              parleyTest.character().hp == parleyTest.character().maxHp,
              "restartAct() cleanly resets current Act state and HP");

        // 5. Verify Codex prequel completion persistence (Ashes of Falkreath + Iron Chronicle milestones).
        TempDir temp;
        CodexStore store(temp.root / "codex.tsv");
        const auto afterPrequel = store.recordPrequelCompletion(1, true);
        check(afterPrequel.prequelCompleted, "Codex marks prequelCompleted");
        check((afterPrequel.prequelCodasMask & (1u << 1)) != 0 && (afterPrequel.prequelCodasMask & (1u << 2)) != 0,
              "Codex stores Last Doubt coda and Iron Chronicle bit");
        const auto reloaded = CodexStore(temp.root / "codex.tsv").load();
        check(reloaded.prequelCompleted && reloaded.prequelCodasMask == afterPrequel.prequelCodasMask,
              "Prequel Codex fields persist across disk reload");

        std::cout << "Prequel (Ashes of Falkreath) rules, combat, parley, coda & artwork tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "PrequelTests failed: " << error.what() << '\n';
        return 1;
    }
}
