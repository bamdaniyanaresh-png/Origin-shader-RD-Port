#ifndef ORIGIN_TERRAIN_H
#define ORIGIN_TERRAIN_H

// terrain lighting, ported from terrain_base.vertex and terrain_base.fragment of Origin Shader

#include "origin_light.h"

// vertex part
// uv1 is in Origin scale (orgUv1), ao comes from orgGetVarCol
// envlig    ambient light (already multiplied by ao)
// outsca    light that survives the air between camera and vertex
// addlig    light scattered into the view ray
// grosunlig sun light at the ground, passed on to the fragment shader
// closkylum sky light seen from the cloud layer (water and glass reflections)
void orgTerrainVertex(
  float tod, highp vec3 worldPos, vec2 uv1, float ao,
  out vec3 envlig, out vec3 outsca, out vec3 addlig, out vec3 grosunlig, out vec3 closkylum
) {
  highp vec3 campos = vec3(0.0, ORG_EARTH_R + ORG_ELEVATION, 0.0);
  highp vec3 orisunlig;
  highp vec3 sunlig;
  highp vec3 L;
  orgSetSunLig(tod, campos, orisunlig, sunlig, L);
  grosunlig = sunlig;

  highp vec3 o;
  highp vec3 i;
  orgSetFog(o, i, orgGetEarPos(campos, worldPos), campos, L, uv1);
  outsca = o;
  addlig = orisunlig*i;

  envlig = orgGetEnvLig(uv1, sunlig, orisunlig, campos, L, ao);

  #if ORG_CLOUDS && defined(TRANSPARENT) && !defined(NL_ORIGIN_NO_REFLECTION)
    highp vec3 clocampos = vec3(0.0, ORG_EARTH_R + ORG_ELEVATION + ORG_CLOUD_Y, 0.0);
    closkylum = orgPhysicsSky(vec3_splat(0.0), clocampos, vec3(0.0, 0.8, 0.6), orisunlig, L, ORG_MARNUM);
  #else
    closkylum = vec3_splat(0.0);
  #endif
}

// sun light that reaches the cloud layer
highp vec3 orgCloudSunLig(highp vec3 orisunlig, highp vec3 L, highp vec3 grosunlig) {
  #if ORG_CLOUDS
    return orgGetSunLig(vec3(0.0, ORG_EARTH_R + ORG_ELEVATION + ORG_CLOUD_Y, 0.0), orisunlig, L);
  #else
    return grosunlig;
  #endif
}

// diffuse and specular of sun, torch and ambient light on a plain surface
vec3 orgShadeSurface(
  vec3 baseColor, vec3 f0, float roughness, highp vec3 N, highp vec3 V, highp vec3 L,
  vec3 sunlig, vec3 poilig, vec3 envlig
) {
  highp vec3 H = normalize(V + L);
  float NoV = saturate(dot(N, V));
  float NoL = saturate(dot(N, L));
  float NoH = saturate(dot(N, H));
  float VoH = dot(V, H);

  vec3 sunBRDF = orgGetBRDF(baseColor, f0, roughness, NoV, NoL, NoH, VoH);
  vec3 poiBRDF = orgDiffuseLambert(baseColor);

  vec3 sunref = sunlig*sunBRDF*NoL;
  vec3 poiref = poilig*poiBRDF;
  vec3 envref = envlig*baseColor;
  return sunref + poiref + envref;
}

// fragment part
// texcol_  : texture colour, alpha test is done by the caller
// col      : vertex colour
// varcol   : tint (rgb) and ambient occlusion (a)
// uv1      : light map in Origin scale
// worpos   : camera relative world position, gpos is the global one
// N        : geometric normal facing the camera
// dpx, dpy, dux, duy : screen space derivatives of worpos and uv0
// envlig, outsca, addlig, grosunlig, closkylum : from orgTerrainVertex
vec4 orgTerrainShade(
  vec4 texcol_, vec4 col, vec4 varcol, vec2 uv1,
  highp vec3 worpos, highp vec3 gpos, highp vec3 N,
  highp vec3 dpx, highp vec3 dpy, vec2 dux, vec2 duy,
  vec3 envlig, highp vec3 outsca, highp vec3 addlig, highp vec3 grosunlig, highp vec3 closkylum,
  bool isWater, float tod, highp float t
) {
  vec4 outcol = vec4(ORG_GRAY, 1.0);

  highp vec3 campos = vec3(0.0, ORG_EARTH_R + ORG_ELEVATION, 0.0);
  highp vec3 orisunlig;
  highp vec3 L;
  orgSunLocal(tod, orisunlig, L);

  highp vec3 V = normalize(-worpos);
  vec3 poilig = orgGetPoiLig(uv1);
  vec3 tint = texcol_.rgb*varcol.rgb;

  #if defined(SEASONS)

    // season variants are shaded like leaves
    vec3 baseColor = nlOriginGammaToLinear(tint);
    vec3 sunlig = grosunlig*orgShadow(uv1);
    vec3 ligsum = orgShadeSurface(baseColor, ORG_F0_LEAVES, ORG_ROUGHNESS, N, V, L, sunlig, poilig, envlig);
    outcol.rgb = nlOriginLig2Col(ligsum*outsca + addlig);

  #elif defined(TRANSPARENT)

    highp vec3 closunlig = orgCloudSunLig(orisunlig, L, grosunlig);

    if (isWater) {
      // water
      highp vec3 Nw = N;
      #if NL_ORIGIN_WATER_TYPE == 2
        highp vec3 T = normalize(dpx*duy.y - dux.y*dpy);
        highp vec3 B = normalize(dpx*duy.x - dux.x*dpy);
        highp vec3 Nt = cross(T, B);
        Nt = dot(Nt, N) > 0.0 ? Nt : -Nt;
        Nw = orgGetWatNor(T, B, Nt, gpos.xz, t);
      #endif

      highp vec3 H = normalize(V + L);
      float NoV = saturate(dot(Nw, V));
      float NoL = saturate(dot(Nw, L));
      float NoH = saturate(dot(Nw, H));
      float VoH = dot(V, H);

      vec3 sunlig = grosunlig*orgShadow(uv1);
      vec3 baseColor = nlOriginGammaToLinear(0.6*normalize(col.rgb));

      vec3 sunspe = orgSpecularBRDF(ORG_F0_WATER, ORG_ROUGHNESS_0, NoV, NoL, NoH, VoH);

      vec3 skyref = orgGetSkyRef(ORG_F0_WATER, NoV, uv1, campos, V, Nw, L, orisunlig, closunlig, closkylum, t);
      vec3 envref = envlig*baseColor;
      vec3 sunsca = sunlig*orgScatteringBSDF(baseColor);
      vec3 poisca = poilig*orgScatteringBSDF(baseColor);

      #ifdef NL_ORIGIN_NO_REFLECTION
        // no reflection, the water keeps the transparency of the vertex colour
        outcol.a = col.a;
      #else
        // alpha controls the strength of the reflection
        outcol.a = ORG_F0_WATER.g + nlOriginSrgbLuma(nlOriginLig2Col(skyref + sunsca + poisca));
      #endif
      skyref /= nlOriginGammaToLinear(vec3_splat(outcol.a));

      vec3 ligsum = sunspe + envref + skyref + sunsca + poisca;
      outcol.rgb = nlOriginLig2Col(ligsum*outsca + addlig);
    } else {
      // glass, ice and other transparent blocks
      highp vec3 H = normalize(V + L);
      float NoV = saturate(dot(N, V));
      float NoL = saturate(dot(N, L));
      float NoH = saturate(dot(N, H));
      float VoH = dot(V, H);

      vec3 sunlig = grosunlig*orgShadow(uv1);
      vec3 baseColor = nlOriginGammaToLinear(tint);

      vec3 sunspe = orgSpecularBRDF(ORG_F0_GLASS, ORG_ROUGHNESS_0, NoV, NoL, NoH, VoH);

      vec3 skyref = orgGetSkyRef(ORG_F0_GLASS, NoV, uv1, campos, V, N, L, orisunlig, closunlig, closkylum, t);
      vec3 envref = envlig*baseColor;
      vec3 sunsca = sunlig*orgScatteringBSDF(baseColor);
      vec3 poisca = poilig*orgScatteringBSDF(baseColor);

      outcol.a = texcol_.a + nlOriginSrgbLuma(nlOriginLig2Col(skyref));
      skyref /= nlOriginGammaToLinear(vec3_splat(outcol.a));

      vec3 ligsum = sunspe + envref + skyref + sunsca + poisca;
      outcol.rgb = nlOriginLig2Col(ligsum*outsca + addlig);
    }

  #else

    #if defined(ALPHA_TEST)
      if (col.b == 0.0) {
        // redstone wire is not lit
        outcol.rgb = texcol_.rgb*col.rgb;
        return outcol;
      }
    #endif

    vec3 baseColor = nlOriginGammaToLinear(tint);

    if (col.a < 0.001) {
      // leaves
      vec3 sunlig = grosunlig*orgShadow(uv1);
      vec3 ligsum = orgShadeSurface(baseColor, ORG_F0_LEAVES, ORG_ROUGHNESS, N, V, L, sunlig, poilig, envlig);
      outcol.rgb = nlOriginLig2Col(ligsum*outsca + addlig);
    } else {
      #if defined(ALPHA_TEST)
        // tall grass, flowers, crops
        vec3 sunlig = uv1.y >= 0.875 ? grosunlig : vec3_splat(0.0);
        vec3 ligsum = orgShadeSurface(baseColor, ORG_F0_LEAVES, ORG_ROUGHNESS, N, V, L, sunlig, poilig, envlig);
        outcol.rgb = nlOriginLig2Col(ligsum*outsca + addlig);
      #else
        // opaque blocks
        highp vec3 H = normalize(V + L);
        float NoV = saturate(dot(N, V));
        float NoL = saturate(dot(N, L));
        float NoH = saturate(dot(N, H));
        float VoH = dot(V, H);

        vec3 sunlig = grosunlig*orgShadow(uv1);

        // material flags are stored in the texture alpha
        float blockcode = orgGetBlockCode(texcol_.a);
        bool ismetal = !orgBitSet(blockcode, 128.0);
        bool issmooth = !orgBitSet(blockcode, 64.0);
        bool isfluorescence = !orgBitSet(blockcode, 32.0);

        vec3 ligsum = vec3_splat(0.0);
        if (isfluorescence) {
          ligsum += baseColor*ORG_FLUORESCENCE;
        }

        if (ismetal) {
          vec3 f0m = mix(baseColor, orgGetMetalF0(blockcode), ORG_HARDCODE_METAL);
          vec3 sunspe = orgSpecularBRDF(f0m, ORG_ROUGHNESS_0, NoV, NoL, NoH, VoH);
          vec3 sunref = sunlig*sunspe*NoL;
          vec3 poiref = poilig*orgDiffuseLambert(ORG_GRAY)*f0m;
          vec3 envref;
          #ifdef NL_ORIGIN_NO_REFLECTION
            if (false) {
          #else
            if (issmooth) {
          #endif
            highp vec3 closunlig = orgCloudSunLig(orisunlig, L, grosunlig);
            #if ORG_CLOUDS
              highp vec3 cloky = orgPhysicsSky(vec3_splat(0.0), vec3(0.0, ORG_EARTH_R + ORG_ELEVATION + ORG_CLOUD_Y, 0.0), vec3(0.0, 0.8, 0.6), orisunlig, L, ORG_MARNUM);
            #else
              highp vec3 cloky = vec3_splat(0.0);
            #endif
            envref = orgGetSkyRef(f0m, NoV, uv1, campos, V, N, L, orisunlig, closunlig, cloky, t);
          } else {
            envref = envlig*f0m;
          }
          ligsum += sunref + poiref + envref;
        } else {
          float rough = issmooth ? ORG_ROUGHNESS_0 : ORG_ROUGHNESS;
          vec3 sunBRDF = orgGetBRDF(baseColor, ORG_F0, rough, NoV, NoL, NoH, VoH);
          vec3 poiBRDF = orgDiffuseLambert(baseColor);
          vec3 sunref = sunlig*sunBRDF*NoL;
          vec3 poiref = poilig*poiBRDF;
          vec3 envref = envlig*baseColor;
          #ifdef NL_ORIGIN_NO_REFLECTION
            if (false) {
          #else
            if (issmooth) {
          #endif
            highp vec3 closunlig = orgCloudSunLig(orisunlig, L, grosunlig);
            #if ORG_CLOUDS
              highp vec3 cloky = orgPhysicsSky(vec3_splat(0.0), vec3(0.0, ORG_EARTH_R + ORG_ELEVATION + ORG_CLOUD_Y, 0.0), vec3(0.0, 0.8, 0.6), orisunlig, L, ORG_MARNUM);
            #else
              highp vec3 cloky = vec3_splat(0.0);
            #endif
            envref += orgGetSkyRef(ORG_F0, NoV, uv1, campos, V, N, L, orisunlig, closunlig, cloky, t);
          }
          ligsum += poiref + sunref + envref;
        }

        outcol.rgb = nlOriginLig2Col(ligsum*outsca + addlig);
      #endif
    }

  #endif

  return outcol;
}

#endif
