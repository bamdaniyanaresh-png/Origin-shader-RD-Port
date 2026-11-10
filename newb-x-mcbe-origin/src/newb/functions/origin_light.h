#ifndef ORIGIN_LIGHT_H
#define ORIGIN_LIGHT_H

// World lighting from Origin Shader.
// The light map has to go through orgUv1 first (level/15 to level/16).

#include "origin_cloud.h"

// PBR constants (ALPHA_PBR_STYLE 1)
#define ORG_ROUGHNESS 1.0
#define ORG_ROUGHNESS_0 0.05
#define ORG_FLUORESCENCE 0.5
#define ORG_HARDCODE_METAL 0.8
#define ORG_F0 vec3(0.040, 0.040, 0.040)
#define ORG_F0_LEAVES vec3(0.020, 0.020, 0.020)
#define ORG_F0_WATER vec3(0.040, 0.040, 0.040)
#define ORG_F0_GLASS vec3(0.035, 0.035, 0.035)

// convert RenderDragon light map value (level/15) into Origin scale (level/16)
vec2 orgUv1(vec2 uv1) {
  return uv1*(15.0/16.0);
}

// vertex colour -> tint + ambient occlusion

float orgGetLeafAO(vec3 c) {
  return dot(c, vec3(0.260278, 1.29965, -0.0816814));
}

float orgGetGrassAO(vec3 c) {
  return dot(c, vec3(0.185295, 1.11177, 0.207755));
}

// rgb = biome tint, a = ambient occlusion / face shading
vec4 orgGetVarCol(vec4 c) {
  if (orgEqu3(c.rgb)) {
    c.a = c.r;
    c.rgb = vec3_splat(1.0);
  } else {
    if (c.a < 0.001) {
      c.a = orgGetLeafAO(c.rgb);
    } else {
      c.a = orgGetGrassAO(c.rgb);
    }
    c.rgb = c.rgb/c.a;
  }
  return c;
}

// light curves

float orgShadow(vec2 uv1) {
  float shadow;
  #if ORG_FAST_SOFTSHADOW
    shadow = saturate(uv1.y*(1.0/ORG_SOFTSHADOW/ORG_SUN_R) + (1.0 - 0.875*(1.0/ORG_SOFTSHADOW/ORG_SUN_R)));
    shadow = (-2.0*shadow + 3.0)*shadow*shadow;
  #else
    bool t_a = uv1.y >= 0.875;
    bool t_b = uv1.y <= 0.875 - 1.0/ORG_SUN_R;
    if (t_a || t_b) {
      shadow = t_a ? 1.0 : 0.0;
    } else {
      shadow = saturate(uv1.y*(1.0/ORG_SOFTSHADOW/ORG_SUN_R) + (1.0 - 0.875*(1.0/ORG_SOFTSHADOW/ORG_SUN_R)))*2.0 - 1.0;
      shadow = (sqrt(max(1.0 - shadow*shadow, 0.0))*shadow + asin(clamp(shadow, -1.0, 1.0)))*(1.0/ORG_PI) + 0.5;
    }
  #endif
  return shadow;
}

float orgUx2l(float l) {
  return 1.0/(l*l);
}

float orgUv1x2lig(float uv1x) {
  float l = saturate((14.0/16.0) - uv1x)*16.0 + 0.5;
  return max(0.0, orgUx2l(l) - orgUx2l(15.0)*(1.0 - uv1x));
}

float orgUy2l(float l) {
  return exp(-0.2*l);
}

float orgUv1y2lig(float uv1y) {
  float l = saturate((15.0/16.0) - uv1y)*16.0;
  return max(0.0, orgUy2l(l) - orgUy2l(15.0)*(1.0 - uv1y));
}

vec3 orgDiffuseLambert(vec3 baseColor) {
  return (1.0/ORG_PI)*baseColor;
}

// torch / block light
vec3 orgGetPoiLig(vec2 uv1) {
  return nlOriginGammaToLinear(ORG_POICOL)*ORG_POILIGHT_A*orgUv1x2lig(uv1.x);
}

// ambient light of a vertex: sky + torch + bounced sun
highp vec3 orgGetEnvLig(vec2 uv1, highp vec3 sunlig, highp vec3 orisunlig, highp vec3 marpos, highp vec3 sunvec, float ao) {
  highp vec3 skylum = orgPhysicsSky(vec3_splat(0.0), marpos, vec3(0.0, 0.8, 0.6), orisunlig, sunvec, ORG_MARNUM);
  vec3 blolum = sunlig*orgDiffuseLambert(ORG_GRAY)*orgUv1y2lig(uv1.y);
  vec3 poilum = orgGetPoiLig(uv1)*orgDiffuseLambert(ORG_GRAY);
  return (ORG_ENVLIG + poilum + mix(blolum, skylum, orgUv1y2lig(uv1.y)*0.65))*ao;
}

highp vec3 orgGetEarPos(highp vec3 campos, highp vec3 worpos) {
  return campos + worpos*(1.0/ORG_FOG_A);
}

// aerial perspective fog from camera to a vertex
// 3 to 4 steps match 8 steps within 1/255
void orgSetFog(out highp vec3 outsca, out highp vec3 insca, highp vec3 stapos, highp vec3 marpos, highp vec3 sunvec, vec2 uv1) {
  #ifdef NL_ORIGIN_FULL_STEPS
    float steps = ORG_MARNUM;
  #else
    float steps = min(ORG_MARNUM, length(stapos - marpos) < 6.0 ? 3.0 : 4.0);
  #endif
  orgAtmsca(outsca, insca, stapos, marpos, sunvec, ORG_PM_FOG, ORG_G_FOG, steps);
  insca *= uv1.y;
}

// BRDF

vec3 orgFFresnel(vec3 specularColor, float VoH) {
  vec3 sq = sqrt(clamp(specularColor, 0.0, 0.99));
  vec3 n = (1.0 + sq)/(1.0 - sq);
  vec3 g = sqrt(n*n + VoH*VoH - 1.0);
  return 0.5*orgPow2v3((g - VoH)/(g + VoH))*(1.0 + orgPow2v3(((g + VoH)*VoH - 1.0)/((g - VoH)*VoH + 1.0)));
}

vec3 orgFSchlick(vec3 specularColor, float VoH) {
  float Fc = orgPow5(1.0 - VoH);
  return saturate(50.0*specularColor.g)*Fc + (1.0 - Fc)*specularColor;
}

vec3 orgFresnelBRDF(vec3 specularColor, float VoH) {
  #if ORG_FAST_BSDF
    return orgFSchlick(specularColor, VoH);
  #else
    return orgFFresnel(specularColor, VoH);
  #endif
}

vec3 orgScatteringBSDF(vec3 baseColor) {
  return (1.0/(4.0*ORG_PI))*baseColor;
}

vec3 orgSpecularBRDF(vec3 specularColor, float roughness, float NoV, float NoL, float NoH, float VoH) {
  float r2 = roughness*roughness;
  float r4 = r2*r2;

  float d = (NoH*r4 - NoH)*NoH + 1.0;
  float D = r4/(ORG_PI*d*d);

  #if ORG_FAST_BSDF
    float Vis = 0.25;
    vec3 F = orgFSchlick(specularColor, VoH);
  #else
    vec2 tmp_vis = vec2(NoV, NoL);
    tmp_vis = tmp_vis + sqrt(tmp_vis*(tmp_vis - tmp_vis*r4) + r4);
    float Vis = 1.0/(tmp_vis.x*tmp_vis.y);
    vec3 F = orgFFresnel(specularColor, VoH);
  #endif

  return D*Vis*F;
}

vec3 orgGetBRDF(vec3 baseColor, vec3 specularColor, float roughness, float NoV, float NoL, float NoH, float VoH) {
  return (1.0/ORG_PI)*baseColor + orgSpecularBRDF(specularColor, roughness, NoV, NoL, NoH, VoH);
}

// alpha PBR (texture alpha encodes material flags, 255 = plain block)

float orgGetBlockCode(float a) {
  float c = floor(a*255.0 + 0.1);
  return c < 0.5 ? 255.0 : c;
}

// true when the bit is set
bool orgBitSet(float code, float bit) {
  return mod(floor(code/bit), 2.0) > 0.5;
}

vec3 orgGetMetalF0(float code) {
  float idx = mod(floor(code/4.0), 4.0);
  if (idx < 0.5) return vec3(1.000, 0.782, 0.344);   // gold
  if (idx < 1.5) return vec3(0.972, 0.960, 0.915);   // silver
  if (idx < 2.5) return vec3(0.955, 0.638, 0.538);   // copper
  return vec3(0.562, 0.565, 0.578);                  // iron
}

// water

highp vec2 orgWatPos(highp vec2 gxz, highp float t) {
  return gxz + 0.125*orgValpar2D(gxz*4.0 + t);
}

// wavy water normal in world space. T,B,N = tangent frame
highp vec3 orgGetWatNor(highp vec3 T, highp vec3 B, highp vec3 N, highp vec2 gxz, highp float t) {
  highp vec3 v = normalize(vec3(0.02*orgValpar2D(2.0*orgWatPos(gxz, t) + t), 1.0));
  return T*v.x + B*v.y + N*v.z;
}

// sky reflection (Origin: getskyref)
highp vec3 orgGetSkyRef(vec3 f0, float NoV, vec2 uv1, highp vec3 campos, highp vec3 V, highp vec3 N, highp vec3 L, highp vec3 orisunlig, highp vec3 closunlig, highp vec3 closkylum, highp float t) {
  #ifdef NL_ORIGIN_NO_REFLECTION
    return vec3_splat(0.0);
  #else
    return orgFresnelBRDF(f0, NoV)*orgUv1y2lig(uv1.y)*orgSkyRender(vec3_splat(0.0), campos, reflect(-V, N), orisunlig, closunlig, closkylum, L, t, 1.0);
  #endif
}

// redstone tools

vec3 orgChunkBound(vec3 oc, vec3 cp, vec3 N, vec3 bp) {
  vec3 cc = vec3_splat(0.0);
  bool cbx = abs(8.0 - cp.x) > (8.0 - 1.5/16.0);
  bool cby = abs(8.0 - cp.y) > (8.0 - 1.5/16.0);
  bool cbz = abs(8.0 - cp.z) > (8.0 - 1.5/16.0);
  bool bbx = abs(0.5 - bp.x) > (0.5 - 0.5/16.0);
  bool bby = abs(0.5 - bp.y) > (0.5 - 0.5/16.0);
  bool bbz = abs(0.5 - bp.z) > (0.5 - 0.5/16.0);
  if ((cbx || cbz) && ((bbx && bby) || (bby && bbz) || (bbz && bbx))) {
    bool flag = !(N.y > 0.9) && ((cbx && cby) || (cby && cbz) || (cbz && cbx));
    vec4 col = vec4(1.0, 1.0, 1.0, 0.2);                       // other edges
    if (flag && cbx && cbz) col = vec4(0.0, 1.0, 0.0, 0.5);   // y axis
    else if (flag && !cbx && cbz) col = vec4(1.0, 0.0, 0.0, 0.5); // x axis
    else if (flag && cbx && !cbz) col = vec4(0.0, 0.0, 1.0, 0.5); // z axis
    cc = mix(cc, col.rgb, col.a);
  }
  return oc + cc;
}

vec3 orgLightBound(vec3 outcol, vec2 uv1, vec3 N, vec3 blopos) {
  bool lba = (uv1.x > 0.4375 && uv1.x < 0.498) && (N.y > 0.9) && (abs(blopos.x - 0.5) > 0.46875 || abs(blopos.z - 0.5) > 0.46875);
  return lba ? mix(outcol, vec3(1.0, 0.5, 0.0), 0.5) : outcol;
}

#endif
