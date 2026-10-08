#ifndef H_DEBUG_SCALES_H
#define H_DEBUG_SCALES_H

#if DEBUG_ENABLED_PP(Scales)

#define DBG_SCALE_INTENSITY_GLOBAL(x)     x *= gDebugSettings.ScaleIntensityGlobal;
#define DBG_SCALE_INTENSITY_PUNCTUAL(x)   x *= gDebugSettings.ScaleIntensityPunctual;
#define DBG_SCALE_INTENSITY_POINT(x)      x *= gDebugSettings.ScaleIntensityPoint;
#define DBG_SCALE_INTENSITY_DISTANT(x)    x *= gDebugSettings.ScaleIntensityDistant;
#define DBG_SCALE_INTENSITY_SPOT(x)       x *= gDebugSettings.ScaleIntensitySpot;
#define DBG_SCALE_INTENSITY_ENV_MAP(x)    x *= gDebugSettings.ScaleIntensityEnvMap;
#define DBG_SCALE_INTENSITY_EMISSION(x)   x *= gDebugSettings.ScaleIntensityEmission;
#define DBG_SCALE_POINT_LIGHT_RADIUS(x)   x *= gDebugSettings.ScalePointLightRadius;
#define DBG_SCALE_F(x)                    x *= gDebugSettings.ScaleF;
#define DBG_SCALE_D(x)                    x *= gDebugSettings.ScaleD;
#define DBG_SCALE_G(x)                    x *= gDebugSettings.ScaleG;
#define DBG_SCALE_DIFFUSE(x)              x *= gDebugSettings.ScaleDiffuse;
#define DBG_SCALE_SPECULAR(x)             x *= gDebugSettings.ScaleSpecular;
#define DBG_SCALE_REFLECT(x)              x *= gDebugSettings.ScaleReflect;
#define DBG_SCALE_REFRACT(x)              x *= gDebugSettings.ScaleRefract;

#else

#define DBG_SCALE_INTENSITY_GLOBAL(x)
#define DBG_SCALE_INTENSITY_PUNCTUAL(x)
#define DBG_SCALE_INTENSITY_POINT(x)
#define DBG_SCALE_INTENSITY_DISTANT(x)
#define DBG_SCALE_INTENSITY_SPOT(x)
#define DBG_SCALE_INTENSITY_ENV_MAP(x)
#define DBG_SCALE_POINT_LIGHT_RADIUS(x)
#define DBG_SCALE_F(x)
#define DBG_SCALE_D(x)
#define DBG_SCALE_G(x)
#define DBG_SCALE_DIFFUSE(x)
#define DBG_SCALE_SPECULAR(x)
#define DBG_SCALE_REFLECT(x)
#define DBG_SCALE_REFRACT(x)

#endif

#endif