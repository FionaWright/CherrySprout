# Flag Debug Mode Idea

Have new argument in FLAG_PROCESS

FLAG_PROCESS(MyFeature, CanBeLoose)

Being "loose" (better name?) means that it can be freely toggled on and off without needing recompilation (Assuming pre-proc is defined true)

Not loose = Snug

For example Accumulation is loose, but alias tables are not

When DEBUG_LOOSE_MODE is defined:

Have new Debug CBV value "LooseFeatureFlags" which now also must be checked for loose flags. Snug flags act as normal

All FLAG_ENABLED directives must be replaced by FLAG_ENABLED_PP which regardless of loose mode will never check loose flags

New GUI button to "Lock in loose flags" which does FlagEnabled |= LooseFlagEnabled, disables loose mode and recompiles

Loose flags in the GUI will be a new checkbox adjacent to the existing ones