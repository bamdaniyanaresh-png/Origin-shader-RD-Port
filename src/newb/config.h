#ifndef NL_CONFIG_H
#define NL_CONFIG_H

/*
  ORIGIN SHADER - MAIN CONFIG
  Origin Shader by linlin (GPL-3.0), ported to Minecraft Bedrock RenderDragon.

  TOGGLES
  - If [toggle] is mentioned, the option can be commented out to disable it (add '//').
    eg: #define NL_ORIGIN_EXAMPLE    -> ON
        //#define NL_ORIGIN_EXAMPLE  -> OFF

  TYPES
  - Integer. The available choices are written next to the option.

  Remember to rebuild the shader after making changes.
*/

/* Quality */
#define NL_ORIGIN_SAMPLING_LEVEL 3      // 1:low, 2:medium, 3:high, 4:placebo (lower = faster on phones)

/* Look */
#define NL_ORIGIN_LIGHT_STYLE 1         // 1:default, 2:cold, 3:warm
#define NL_ORIGIN_PLANET_ATMOSPHERE 0   // 1:space mode (camera 20km up), 0:normal
#define NL_ORIGIN_CLOUD_TYPE 3          // 1:none, 2:fast box clouds, 3:fancy jelly clouds
#define NL_ORIGIN_WATER_TYPE 2          // 1:flat water, 2:wavy water normals

/* Color */
#define NL_ORIGIN_TONEMAP 1             // 0:none, 1:ACES, 2:AgX, 3:PBR Neutral, 4:Oklab
#define NL_ORIGIN_GAMMA 2.2             // Origin uses 2.2
#define NL_ORIGIN_LUM 1.0               // global brightness

/* Redstone tools */
#define NL_ORIGIN_LIGHT_MAP 0           // 0:off, 1:show light level borders
#define NL_ORIGIN_CHUNK_BOUND 0         // 0:off, 1:show chunk borders

/* Entities / items / enchant glint */
#define NL_ORIGIN_GLINT 20.0            // enchant glint strength

/* Sky texture strength for the End sky (Origin: MILKY_WAY_LIGHT) */
#define NL_ORIGIN_END_SKY_LIGHT 1.0

/* Quality, set by the subpacks (Low, Medium, Ultra) */
//#define NL_ORIGIN_NO_REFLECTION       // water, glass and block reflections off (Low)
//#define NL_ORIGIN_FULL_STEPS          // full steps for fog and sky (Ultra)

#ifdef ORIGIN_LOW
  #define NL_ORIGIN_NO_REFLECTION
#endif

#ifdef ORIGIN_ULTRA
  #define NL_ORIGIN_FULL_STEPS
#endif

#endif
