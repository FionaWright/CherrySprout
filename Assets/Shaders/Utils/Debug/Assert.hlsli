#ifndef H_ASSERT_H
#define H_ASSERT_H

#include "Utils/Debug/DebugID.h"

#if !DEBUG_ENABLED(Asserts)

#    define DBG_ASSERT(expr, dbgID)
#    define DBG_ASSERT_VALID(expr, dbgID)

#else

// TODO: If expr is false then increment debug buffer at dbgID
#define DBG_ASSERT(expr, dbgID)          \
{                                        \
}                                        \

// TODO: If value is inf / nan then increment debug buffer at dbgID
#define DBG_ASSERT_VALID(value, dbgID)          \
{                                        \
}                                        \

#endif

#endif
