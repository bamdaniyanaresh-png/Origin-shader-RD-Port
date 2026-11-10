#ifndef ORIGIN_CONFIG_H
#define ORIGIN_CONFIG_H

// Constants from Origin Shader (Users/Set.txt), all names start with ORG_.
// The options are in config.h.

#ifndef NL_ORIGIN_LIGHT_STYLE
  #define NL_ORIGIN_LIGHT_STYLE 1
#endif
#ifndef NL_ORIGIN_SAMPLING_LEVEL
  #define NL_ORIGIN_SAMPLING_LEVEL 3
#endif
#ifndef NL_ORIGIN_PLANET_ATMOSPHERE
  #define NL_ORIGIN_PLANET_ATMOSPHERE 0
#endif
#ifndef NL_ORIGIN_CLOUD_TYPE
  #define NL_ORIGIN_CLOUD_TYPE 3
#endif
#ifndef NL_ORIGIN_TONEMAP
  #define NL_ORIGIN_TONEMAP 1
#endif
#ifndef NL_ORIGIN_GAMMA
  #define NL_ORIGIN_GAMMA 2.2
#endif
#ifndef NL_ORIGIN_LUM
  #define NL_ORIGIN_LUM 1.0
#endif
#ifndef NL_ORIGIN_WATER_TYPE
  #define NL_ORIGIN_WATER_TYPE 2
#endif
#ifndef NL_ORIGIN_LIGHT_MAP
  #define NL_ORIGIN_LIGHT_MAP 0
#endif
#ifndef NL_ORIGIN_CHUNK_BOUND
  #define NL_ORIGIN_CHUNK_BOUND 0
#endif

#define ORG_PI 3.141592653589793
#define ORG_GRAY vec3(0.18, 0.18, 0.18)

// planet / elevation
#if NL_ORIGIN_PLANET_ATMOSPHERE
  #define ORG_ELEVATION 20.0
#else
  #define ORG_ELEVATION 1.5
#endif

// sampling level
#if NL_ORIGIN_PLANET_ATMOSPHERE && NL_ORIGIN_SAMPLING_LEVEL < 3
  #define ORG_SAMPLING_LEVEL 3
#else
  #define ORG_SAMPLING_LEVEL NL_ORIGIN_SAMPLING_LEVEL
#endif

#if ORG_SAMPLING_LEVEL == 1
  #define ORG_MARNUM 4.0
  #define ORG_CLOINTNUM 8.0
  #define ORG_CLOSAMNUM 2.0
  #define ORG_CLOMARNUM 4.0
  #define ORG_FAST_SOFTSHADOW 1
  #define ORG_FAST_BSDF 1
  #define ORG_FAST_VALUE 1
#elif ORG_SAMPLING_LEVEL == 2
  #define ORG_MARNUM 6.0
  #define ORG_CLOINTNUM 12.0
  #define ORG_CLOSAMNUM 3.0
  #define ORG_CLOMARNUM 6.0
  #define ORG_FAST_SOFTSHADOW 1
  #define ORG_FAST_BSDF 0
  #define ORG_FAST_VALUE 1
#elif ORG_SAMPLING_LEVEL == 3
  #define ORG_MARNUM 8.0
  #define ORG_CLOINTNUM 16.0
  #define ORG_CLOSAMNUM 4.0
  #define ORG_CLOMARNUM 8.0
  #define ORG_FAST_SOFTSHADOW 1
  #define ORG_FAST_BSDF 0
  #define ORG_FAST_VALUE 0
#else
  #define ORG_MARNUM 16.0
  #define ORG_CLOINTNUM 32.0
  #define ORG_CLOSAMNUM 8.0
  #define ORG_CLOMARNUM 16.0
  #define ORG_FAST_SOFTSHADOW 0
  #define ORG_FAST_BSDF 0
  #define ORG_FAST_VALUE 0
#endif

// light style
#define ORG_SUNCOL vec3(1.0, 1.0, 1.0)
#define ORG_SUNLIGHT_A 12.0
#define ORG_MOOCOL vec3(1.0, 1.0, 1.0)
#define ORG_MOOLIGHT_A 0.2
#define ORG_POILIGHT_A 12.0
#define ORG_ENVCOL vec3(1.0, 1.0, 1.0)
#define ORG_ENVLIGHT_A 0.02
#define ORG_LATITUDE 0.2618
#define ORG_SUN_R 0.009512
#define ORG_SOFTSHADOW 3.0
#define ORG_FOG_A 16.0
#define ORG_O3 (0.010*vec3(3.426, 8.298, 0.356)*0.06)

#if NL_ORIGIN_LIGHT_STYLE == 2
  #define ORG_POICOL vec3(1.0, 1.0, 1.0)
  #define ORG_LR (0.005*vec3(0.580, 1.350, 3.310))
  #define ORG_LM (0.005*vec3(2.000, 2.000, 2.000))
#elif NL_ORIGIN_LIGHT_STYLE == 3
  #define ORG_POICOL vec3(1.0, 0.3814, 0.0588)
  #define ORG_LR (0.015*vec3(0.580, 1.350, 3.310))
  #define ORG_LM (0.015*vec3(2.000, 2.000, 2.000))
#else
  #define ORG_POICOL vec3(1.0, 0.5155, 0.2087)
  #define ORG_LR (0.010*vec3(0.580, 1.350, 3.310))
  #define ORG_LM (0.010*vec3(2.000, 2.000, 2.000))
#endif

#define ORG_PM_AIR vec4(0.5, 0.25, 0.15, 0.1)
#define ORG_G_AIR vec4(0.0, 0.5, 0.8, 0.95)
#define ORG_PM_FOG vec4(0.5, 0.25, 0.15, 0.1)
#define ORG_G_FOG vec4(0.0, 0.5, 0.8, 0.95)
#define ORG_RO (ORG_LR + ORG_O3)
#define ORG_HR 8.0
#define ORG_HM 1.2
#define ORG_EARTH_R 6371.0
#define ORG_EARTH_r (ORG_EARTH_R - 0.5)
#define ORG_SUN_RISE_Y (-0.04)
#define ORG_EARTH_SHADOW 1

// clouds (CLOUD_TYPE 2 = fast, 3 = fancy)
// NL_ORIGIN_CLOUD_TYPE 1 = no clouds.
#if NL_ORIGIN_CLOUD_TYPE == 2 || NL_ORIGIN_CLOUD_TYPE == 3
  #define ORG_CLOUDS 1
#else
  #define ORG_CLOUDS 0
#endif

// clouds
#if ORG_CLOUDS
  #define ORG_CLOUD_COVERAGE 0.18
  #define ORG_CLOUD_SPEED vec2(0.02, 0.05)
  #define ORG_CLOUD_W 1.0
  #define ORG_CLOUD_H (1.0/3.0)
  #define ORG_CLOUD_Y 3.0
  #define ORG_CLOSIZ vec3(ORG_CLOUD_W, ORG_CLOUD_H, ORG_CLOUD_W)
  #if NL_ORIGIN_CLOUD_TYPE == 3
    #define ORG_CLOUD_LM 4.0
    #define ORG_PM_CLOUD vec4(0.5, 0.1, 0.3, 0.1)
    #define ORG_G_CLOUD vec4(0.0, 0.8, 0.95, 0.98)
  #endif
#endif

// scattering helpers
#define ORG_NEG_INV_H vec2(-1.0/ORG_HR, -1.0/ORG_HM)
#define ORG_R_OVER_H vec2(ORG_EARTH_R/ORG_HR, ORG_EARTH_R/ORG_HM)

#endif
