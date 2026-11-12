# Local leaderboards

Each combat difficulty has its own **top ten**: Easy, Medium, Hard and Impossible. After any of the four endings, use **View your journey** or `Esc` to see the final journey results and the matching board together. Early game overs and abandoned descents do not qualify because they have not reached an ending.

Type exactly **three letters (A–Z)**, use **Backspace** to edit, and press **Enter** or click **Save score**. Lowercase input becomes uppercase. Letters such as `V` enter initials while the name field is active. `F11` still changes fullscreen mode. **Skip entry** / `Esc` leaves the board visible without submitting. After saving or skipping, the difficulty tabs let you browse all four boards, and **Main menu** becomes available.

The saved score is the same overall score displayed in the journey results: the sum of the six hero stats, truncating each stat to its integer value. No difficulty bonus, time bonus or replacement formula is added. A run is saved to its actual locked combat difficulty. Hero and ending are shown beside the initials and score.

Scores rank highest first. Ties keep the earlier submitted run first. Repeated initials are allowed for different runs. Only ten scores per difficulty are retained. A score outside the top ten receives an explanatory message. Each run can be submitted once; reopening its results or pressing Save repeatedly does not duplicate it. The current entry is highlighted when it places in the displayed board.

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

The versioned TSV contains only initials, score, difficulty, hero, ending and an ordering ID. Reads and saves share an operating-system file lock. A save reloads the latest tables and atomically replaces the file, preserving submissions made by another game instance. Invalid or truncated files are reported without overwriting them. A failed save keeps the initials available to retry or skip. If a manually edited or damaged file cannot be read, keep a backup and restore a valid copy, or move it aside to start new boards.

## Verification

The SFML-independent tests cover all difficulties/endings, fresh-profile loading, persistence, Unicode paths, initials validation, top-ten limits, ties, repeated initials, zero scores, concurrent writers and preservation on invalid data or storage failure. Render/input tests earn endings through the actual run rules, enter initials, save the exact final score, reject incomplete names, browse all boards, reopen results without duplication, reload in a fresh game instance and display a save error. The original score calculation and all three-floor rules remain covered by the existing suites.
