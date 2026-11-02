#ifndef H_SPACES_H
#define H_SPACES_H

#ifdef __cplusplus
#define SPACE(i) i
#else
#define SPACE(i) space##i
#endif

#define SPACE_B_DEFAULT            SPACE(0)
#define SPACE_B_DEBUG              SPACE(1)
#define SPACE_B_RESTIR             SPACE(2)

#define SPACE_T_DEFAULT            SPACE(0)
#define SPACE_T_SCENE_TEXTURES     SPACE(1)
#define SPACE_T_LSD                SPACE(2)
#define SPACE_T_DEBUG              SPACE(3)

#define SPACE_U_DEFAULT            SPACE(0)
#define SPACE_U_DEBUG              SPACE(1)
#define SPACE_U_RESTIR             SPACE(2)
#define SPACE_U_GDPT               SPACE(3)

#endif