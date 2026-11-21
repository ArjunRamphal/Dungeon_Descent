# Local leaderboards

Each combat difficulty has its own **leaderboard**: Easy, Medium, Hard and Impossible. You can browse all four boards anytime from **High scores** on the main menu. After any of the four endings, use **View your journey** or `Esc` to see the final journey results and enter your initials on the matching board. After an early game over, the results screen also displays the leaderboards in read-only mode so you can compare your run against saved scores, though only completed endings can submit a score.

Type exactly **three letters (A–Z)**, use **Backspace** to edit, and press **Enter** or click **Save score**. Lowercase input becomes uppercase. Letters such as `V` enter initials while the name field is active. `F11` still changes fullscreen mode. **Skip entry** / `Esc` leaves the board visible without submitting. After saving or skipping, the difficulty tabs let you browse all four boards, and **Main menu** becomes available.

The saved score is the same overall score displayed in the journey results: the sum of the six hero stats, truncating each stat to its integer value. No difficulty bonus, time bonus or replacement formula is added. A run is saved to its actual locked combat difficulty. Hero and ending are shown beside the initials and score.

Scores rank highest first. Ties keep the earlier submitted run first. Repeated initials are allowed for different runs. All completed runs submitted by the player are retained and displayed **10 records per page**; use the **`<`** and **`>`** buttons (or Left/Right, Page Up/Down, or the mouse wheel) to page through all records. When browsing, use the **Hero filter tabs** (`All Heroes`, `Warrior`, `Ranger`, `Mage`, `Rogue`) to filter entries by class, and click **Replay seed** on any seeded entry to jump straight to biome selection with that run's hero, difficulty, and seed pre-filled. Each run can be submitted once; reopening its results or pressing Save repeatedly does not duplicate it. When a run is saved, the board automatically opens to the page containing your new rank and highlights your entry.

## Storage

| Platform | Default file |
| --- | --- |
| Windows | `%LOCALAPPDATA%\DungeonDescent\leaderboards.tsv` |
| Linux | `$XDG_DATA_HOME/DungeonDescent/leaderboards.tsv`, or `~/.local/share/DungeonDescent/leaderboards.tsv` |

The tables belong to the local operating-system account and require no network connection. The same file is used by Debug, Release and Standalone builds. Updating, moving or rebuilding the EXE keeps existing scores. Leaderboards are stored separately from the versioned asset cache, so clearing extracted assets does not clear the scores. Copy the TSV file while the game is closed to back it up or move it to another account/computer.

For a separate profile or a writable portable location:

```powershell
.\dist\Dungeon_Descent.exe --data-dir "D:\DungeonDescentData"
```

The `DUNGEON_DESCENT_DATA_DIR` environment variable also overrides the directory; `--data-dir` takes precedence. Unit/render tests use their own temporary or build directories and do not submit scores to the player's normal profile.

The versioned v2 TSV stores initials, score, difficulty, hero, ending, run seed, playtime, and an ordering ID, while remaining fully backward-compatible with v1 leaderboard files. Reads and saves share an operating-system file lock. A save reloads the latest tables and atomically replaces the file, preserving submissions made by another game instance. Invalid or truncated files are reported without overwriting them. A failed save keeps the initials available to retry or skip. If a manually edited or damaged file cannot be read, keep a backup and restore a valid copy, or move it aside to start new boards.

## Verification

The SFML-independent tests cover all difficulties/endings, fresh-profile loading, v1 backward compatibility and v2 seed/playtime persistence, Unicode paths, initials validation, multi-page retention, ties, repeated initials, zero scores, concurrent writers and preservation on invalid data or storage failure. Render/input tests earn endings through the actual run rules, enter initials, save the exact final score, reject incomplete names, filter by hero, replay a saved seed, browse all boards, reopen results without duplication, reload in a fresh game instance and display a save error. The original score calculation and all three-floor rules remain covered by the existing suites.
