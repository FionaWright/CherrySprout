# Flags Rename Refactor

## Current

FeatureFlags - Core features (Jitter, Accumulation, ReSTIR, etc)

DebugFlags - Debug features (PathDumper, OutputColor, FurnaceTest, etc)

CbvFlagsMode - Mode that adds extra flags to CBV, runtime checks, ability to avoid recompilation at cost of compiling shader initially with everything enabled 
"--cbvFlagMode" - Cmd line arg

CbvFeatureFlags/CbvDebugFlags - C++ flags used for CbvFlagsMode

ListFeature.h/ListDebug.h - List of features. Each tied to CanBeCbvValue bool

Processing.h - Given defined macros, compiles list as type index/enum/strings/CanBeCbv-bool

Flags.h - Runs processing for each type depending on C++/HLSL

FEATURE_COUNT/DEBUG_COUNT - Count of flags

s_defaultFeatureFlags{CbvMode}/s_defaultDebugFlags{CbvMode} - Static list of default flags

GetPathTracerFeatureFlag()    
GetPathTracerDebugFlag()    
SetPathTracerFeatureFlag()    
SetPathTracerDebugFlag()    
    - Operates on a flags state and flag value

FlagFeatureEnabled()  
FlagDebugEnabled()  
    - Operates on config comptime flags state, cbv flags and flag value

FEATURE_ENABLED_PP()/DEBUG_ENABLED_PP() - Checks if feature enabled, for directives only

DEBUG_CBV_FLAGS_MODE_ENABLED - Define for cbv flag mode checking

FEATURE_ENABLED()/DEBUG_ENABLED() - Checks if feature enabled, comptime/runtime depending on CbvFlagsMode 

FEATURE_FLAGS / DEBUG_FLAGS
FEATURE_VALUE_* / DEBUG_VALUE_*

## Renamed

FeatFlagsCore - Core features (Jitter, Accumulation, ReSTIR, etc)

FeatFlagsDbg - Debug features (PathDumper, OutputColor, FurnaceTest, etc)

FeatFlagsQuickSwitchMode - Mode that adds extra flags to CBV, runtime checks, ability to avoid recompilation at cost of compiling shader initially with everything enabled 
"--ffQuickSwitch" - Cmd line arg

FeatFlagsQSCore/FeatFlagsQSDbg - C++ flags used for FeatFlagsQuickSwitchMode

ListCore.h/ListDebug.h - List of features. Each tied to IsQuickSwitchable bool

FeatFlagsProcessing.h - Given defined macros, compiles list as type index/enum/strings/IsQuickSwitchable-bool

FeatFlags.h - Runs processing for each type depending on C++/HLSL

FEAT_COUNT_CORE/FEAT_COUNT_DBG - Count of flags

s_defaultFeatFlagsCore{QS}/s_defaultFeatFlagsDbg{QS} - Static list of default flags

GetFeatFlagCore()   
GetFeatFlagDbg()   
SetFeatFlagCore()   
SetFeatFlagDbg()  
    - Operates on a flags state and flag value

FeatFlagEnabledCore()   
FeatFlagEnabledDbg()  
    - Operates on config comptime flags state, QS flags and flag value

FEAT_CORE_D()/FEAT_DBG_D() - Checks if feature enabled, for directives only

FF_QUICK_SWITCH_ENABLED - Define for QS flag mode checking

FEAT_CORE()/FEAT_DBG() - Checks if feature enabled, comptime/runtime depending on FeatFlagsQuickSwitchMode 

FEAT_FLAGS_COMP_CORE / FEAT_FLAGS_COMP_DBG
FEAT_FLAG_VALUE_CORE_* / FEAT_FLAG_VALUE_DBG_*