#ifndef ORIGIN_TONEMAP_H
#define ORIGIN_TONEMAP_H

// Tonemapping from Origin Shader.
// Matrix products are written as dot products so every backend gives the same result.

#include "origin_config.h"

// color helpers (Origin: gamma2line / line2gamma / srgb2y / lrgb2y)

vec3 nlOriginGammaToLinear(vec3 x) {
  return pow(max(x, vec3_splat(0.0)), vec3_splat(NL_ORIGIN_GAMMA));
}

vec3 nlOriginLinearToGamma(vec3 x) {
  return pow(max(x, vec3_splat(0.0)), vec3_splat(1.0/NL_ORIGIN_GAMMA));
}

float nlOriginSrgbLuma(vec3 c) {
  return dot(c, vec3(0.299, 0.587, 0.114));
}

float nlOriginLinearLuma(vec3 c) {
  return dot(c, vec3(0.2126, 0.7152, 0.0722));
}

// 1 : ACES (Stephen Hill RRT+ODT fit)

vec3 nlOriginRRTAndODTFit(vec3 v) {
  vec3 a = v*(v + vec3_splat(0.0245786)) - vec3_splat(0.000090537);
  vec3 b = v*(0.983729*v + vec3_splat(0.4329510)) + vec3_splat(0.238081);
  return a/b;
}

vec3 nlOriginTonemapACES(vec3 c) {
  // input matrix (sRGB -> AP1 RRT sat)
  vec3 v = vec3(dot(c, vec3(0.59719, 0.35458, 0.04823)),
                dot(c, vec3(0.07600, 0.90834, 0.01566)),
                dot(c, vec3(0.02840, 0.13383, 0.83777)));

  v = nlOriginRRTAndODTFit(v);

  // output matrix (ODT sat -> XYZ -> D60 to D65 -> sRGB)
  c = vec3(dot(v, vec3( 1.60475, -0.53108, -0.07367)),
           dot(v, vec3(-0.10208,  1.10813, -0.00605)),
           dot(v, vec3(-0.00327, -0.07276,  1.07602)));

  return clamp(c, 0.0, 1.0);
}

// 2 : AgX

vec3 nlOriginAgxCurve(vec3 v) {
  const float threshold = 0.6060606060606061;
  const float a_up = 69.86278913545539;
  const float a_down = 59.507875;
  const float b_up = 3.25;      // 13/4
  const float b_down = 3.0;
  const float c_up = -0.3076923076923077; // -4/13
  const float c_down = -0.3333333333333333; // -1/3

  vec3 mask = step(v, vec3_splat(threshold));
  vec3 a = a_up + (a_down - a_up)*mask;
  vec3 b = b_up + (b_down - b_up)*mask;
  vec3 c = c_up + (c_down - c_up)*mask;
  return 0.5 + ((-2.0*threshold) + 2.0*v)*pow(1.0 + a*pow(abs(v - vec3_splat(threshold)), b), c);
}

vec3 nlOriginTonemapAgX(vec3 ci) {
  const float min_ev = -12.473931188332413;
  const float max_ev = 4.026068811667588;
  const float dynamic_range = max_ev - min_ev;

  // input transform (inset)
  ci = vec3(dot(ci, vec3(0.8424010709504686, 0.07843650156180276, 0.0791624274877287)),
            dot(ci, vec3(0.04240107095046854, 0.8784365015618028, 0.0791624274877287)),
            dot(ci, vec3(0.04240107095046854, 0.07843650156180276, 0.8791624274877287)));

  // sigmoid in log2 space (log2(0) guarded)
  vec3 ct = clamp(log2(max(ci, vec3_splat(0.0000001)))*(1.0/dynamic_range) - (min_ev/dynamic_range), 0.0, 1.0);
  vec3 co = nlOriginAgxCurve(ct);

  // inverse input transform (outset)
  return vec3(dot(co, vec3( 1.1969986613119143, -0.09804562695225345, -0.09895303435966087)),
              dot(co, vec3(-0.053001338688085674, 1.1519543730477466, -0.09895303435966087)),
              dot(co, vec3(-0.053001338688085674, -0.09804562695225345, 1.151046965640339)));
}

// 3 : Khronos PBR Neutral

vec3 nlOriginTonemapNeutral(vec3 color) {
  const float startCompression = 0.76; // 0.8 - 0.04
  const float desaturation = 0.15;

  float x = min(color.r, min(color.g, color.b));
  float offset = x < 0.08 ? x - 6.25*x*x : 0.04;
  color -= vec3_splat(offset);

  float peak = max(color.r, max(color.g, color.b));
  if (peak < startCompression) {
    return color;
  }

  const float d = 1.0 - startCompression;
  float newPeak = 1.0 - d*d/(peak + d - startCompression);
  color *= newPeak/peak;

  float g = 1.0 - 1.0/(desaturation*(peak - newPeak) + 1.0);
  return mix(color, vec3_splat(newPeak), g);
}

// 4 : Oklab soft clip

vec3 nlOriginLinearToOklab(vec3 c) {
  vec3 lms = vec3(0.4122214708, 0.2119034982, 0.0883024619)*c.r
           + vec3(0.5363325363, 0.6806995451, 0.2817188376)*c.g
           + vec3(0.0514459929, 0.1073969566, 0.6299787005)*c.b;
  vec3 l = pow(max(lms, vec3_splat(0.0)), vec3_splat(1.0/3.0));
  return vec3(0.2104542553, 1.9779984951, 0.0259040371)*l.x
       + vec3(0.7936177850, -2.4285922050, 0.7827717662)*l.y
       + vec3(-0.0040720468, 0.4505937099, -0.8086757660)*l.z;
}

vec3 nlOriginOklabToLinear(vec3 c) {
  vec3 l = vec3_splat(c.x)
         + vec3(0.3963377774, -0.1055613458, -0.0894841775)*c.y
         + vec3(0.2158037573, -0.0638541728, -1.2914855480)*c.z;
  vec3 lms = l*l*l;
  return vec3(4.0767416621, -1.2684380046, -0.0041960863)*lms.x
       + vec3(-3.3077115913, 2.6097574011, -0.7034186147)*lms.y
       + vec3(0.2309699292, -0.3413193965, 1.7076147010)*lms.z;
}

float nlOriginSoftClip(float x) {
  const float s = -0.875;
  return (1.0 + x - sqrt(1.0 - 2.0*s*x + x*x))/(1.0 + s);
}

vec3 nlOriginTonemapOklab(vec3 color) {
  vec3 oklabWhite = vec3(1.0, 0.0, 0.0);

  float maxRgb = max(max(color.r, color.g), max(color.b, 0.0000001));

  vec3 oklab = nlOriginLinearToOklab(color);
  vec3 anchor = nlOriginLinearToOklab(color/maxRgb);

  if (oklab.x > anchor.x) {
    float k = nlOriginSoftClip(oklab.x - anchor.x);
    oklab = mix(anchor, oklabWhite, k);
  }

  return nlOriginOklabToLinear(oklab);
}

// entry point
// Origin lig2col(): tonemap(light * LUM) then gamma encode.
// AgX output is already display encoded, so no gamma there (same as Origin).

vec3 nlOriginLig2Col(vec3 lig) {
  lig *= NL_ORIGIN_LUM;

  #if NL_ORIGIN_TONEMAP == 1
    return nlOriginLinearToGamma(nlOriginTonemapACES(lig));
  #elif NL_ORIGIN_TONEMAP == 2
    return max(nlOriginTonemapAgX(lig), vec3_splat(0.0));
  #elif NL_ORIGIN_TONEMAP == 3
    return nlOriginLinearToGamma(nlOriginTonemapNeutral(lig));
  #elif NL_ORIGIN_TONEMAP == 4
    return nlOriginLinearToGamma(nlOriginTonemapOklab(lig));
  #else
    return nlOriginLinearToGamma(lig);
  #endif
}

#endif
