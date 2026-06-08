#ifndef H_DEBUG_ID_H
#define H_DEBUG_ID_H

#ifndef __cplusplus
#   define CREATE_ID(id, idx, string) static const uint id = idx;
#else
#   define CREATE_ID(id, idx, string) string,

#   include <vector>
static constexpr auto s_debugIdList = {
#endif

// Indices MUST be in sequential order!!
CREATE_ID(NO_METAL_GLASS,       0, "Transmissive materials cannot be metal")
CREATE_ID(NO_EMISSIVE_GLASS,    1, "Transmissive materials cannot be emissive")

#ifdef __cplusplus
};
#endif

#endif
