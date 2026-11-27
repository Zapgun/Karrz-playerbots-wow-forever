# Source patches (ForeverCore)

Code changes kept as `git apply`-able diffs so they can be re-applied when a new
ForeverCore version lands. Apply in filename order from the ForeverCore repo
root:

    cd F:/_dev/ForeverCore
    git apply ../Karrz-playerbots-wow-forever/patches/0001_*.patch

or `git am < file` if you want them as commits. `sql-changes/` holds the
database-side equivalents.

| File | Change |
|------|--------|
| 0001_spell_tracker_stacking.patch | Forever tracker stacking: creature trackers (aura 44 + Track Hidden 151) stay one-at-a-time; resource finders (aura 45: Find Herbs/Minerals/Fish) get their own exclusive group; Find Treasure (aura 45, misc 6 — dwarf racial) stacks on top of both. Result: 1 Track + 1 Find + Find Treasure can be active at once. |

(0002_bot_rpg_names.patch is gone — the change merged upstream as ForeverCore PR #394 and ships in the module; the name data applies itself as the bot migrations 0063/0064.)
