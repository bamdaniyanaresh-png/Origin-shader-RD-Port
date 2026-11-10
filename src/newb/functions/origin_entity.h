#ifndef ORIGIN_ENTITY_H
#define ORIGIN_ENTITY_H

// Lighting for entities, held items, weather and the End sky.
// Uses the same sun, sky and fog as the terrain.

#include "origin_light.h"

// entity vertex
// worldPos : camera relative position
// N        : world space normal
// tile     : TileLightColor (r = block light, b = sky light, level/15)
void orgEntityVertex(
  float tod, highp vec3 worldPos, highp vec3 N, vec4 tile,
  out vec3 lightFactor, out vec3 outsca, out vec3 addlig
) {
  highp vec3 campos = vec3(0.0, ORG_EARTH_R + ORG_ELEVATION, 0.0);
  highp vec3 orisunlig;
  highp vec3 grosunlig;
  highp vec3 L;
  orgSetSunLig(tod, campos, orisunlig, grosunlig, L);

  vec2 uv1 = orgUv1(vec2(tile.r, tile.b));

  highp vec3 o;
  highp vec3 i;
  orgSetFog(o, i, orgGetEarPos(campos, worldPos), campos, L, uv1);
  outsca = o;
  addlig = orisunlig*i;

  vec3 envlig = orgGetEnvLig(uv1, grosunlig, orisunlig, campos, L, 1.0);
  vec3 poilig = orgGetPoiLig(uv1);

  float NoL = saturate(dot(N, L));
  vec3 sunlig = grosunlig*orgShadow(uv1);

  // Lambert diffuse: (1/pi) * (sun*NoL + torch) + ambient
  lightFactor = (1.0/ORG_PI)*(sunlig*NoL + poilig) + envlig;
}

// entity fragment
// albedo: gamma encoded colour from the texture. returns display colour.
vec3 orgEntityFinish(vec3 albedoLinearLit, vec3 outsca, vec3 addlig) {
  return nlOriginLig2Col(albedoLinearLit*outsca + addlig);
}

// enchant glint, added to the light factor (Origin has no glint, this keeps vanilla behaviour)
vec4 orgGlint(vec4 light, vec4 layerUV, sampler2D glintTexture, vec4 glintColor, vec4 tileLightColor, vec4 albedo) {
  float d = fract(dot(albedo.rgb, vec3_splat(4.0)));

  vec4 tex1 = texture2D(glintTexture, fract(layerUV.xy + 0.1*d)).rgbr;
  vec4 tex2 = texture2D(glintTexture, fract(layerUV.zw + 0.1*d)).rgbr;

  vec4 glint = (tex1*tex1 + tex2*tex2)*tileLightColor*glintColor;

  light.rgb = light.rgb*(1.0 - 0.4*glint.a) + NL_ORIGIN_GLINT*glint.rgb;
  return light;
}

// weather
// light that falls on rain / snow: sky luminance + a bit of direct light
void orgWeatherLight(float tod, out vec3 lig) {
  highp vec3 campos = vec3(0.0, ORG_EARTH_R + ORG_ELEVATION, 0.0);
  highp vec3 orisunlig;
  highp vec3 grosunlig;
  highp vec3 L;
  orgSetSunLig(tod, campos, orisunlig, grosunlig, L);
  highp vec3 skylum = orgPhysicsSky(vec3_splat(0.0), campos, vec3(0.0, 0.8, 0.6), orisunlig, L, ORG_MARNUM);
  lig = skylum + grosunlig*(0.3/ORG_PI);
}

#endif
