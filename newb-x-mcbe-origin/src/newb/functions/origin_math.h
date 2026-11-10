#ifndef ORIGIN_MATH_H
#define ORIGIN_MATH_H

// Math helpers from Origin Shader.

#include "origin_config.h"

float orgPow3_2(float x) { return x*sqrt(x); }
vec4 orgPow3_2v4(vec4 x) { return x*sqrt(x); }
float orgPow2(float x) { return x*x; }
vec3 orgPow2v3(vec3 x) { return x*x; }
float orgPow3(float x) { return x*x*x; }
float orgPow4(float x) { x *= x; return x*x; }
float orgPow5(float x) { float x2 = x*x; return x2*x2*x; }

// true if r,g,b are (almost) equal
bool orgEqu3(vec3 v) {
  return abs(v.x - v.y) < 0.000002 && abs(v.y - v.z) < 0.000002;
}

// geometry

vec4 orgRaymarchAABB(highp vec3 o, highp vec3 v, highp vec3 a, highp vec3 b) {
  highp vec3 ta = (a - o)/v;
  highp vec3 tb = (b - o)/v;
  highp vec4 t2;
  t2.xyz = max(ta, tb);
  t2.w = min(min(t2.x, t2.y), t2.z);
  t2.xyz = sign(v)*step(t2.xyz, t2.www);
  // a sample that sits 1-2 float steps outside the box gives a huge negative length when
  // the ray is almost parallel to a face (sun on the horizon). The face choice above stays
  // correct, only the length must not go below 0.
  t2.w = max(t2.w, 0.0);
  return t2;
}

highp float orgRaycastSphere(highp float R2, highp float r2, highp float PoD) {
  highp float t = PoD*PoD + R2;
  highp float q = sqrt(max(t - r2, 0.0));
  return ((r2 > R2 && PoD < 0.0) || t < r2) ? -1.0 : (R2 > r2 ? q : -q) + PoD;
}

highp float orgRaycastSphereLength(highp float R2, highp float r2, highp float PoD) {
  highp float q = sqrt(max(PoD*PoD + R2 - r2, 0.0));
  return (R2 > r2 ? q : -q) + PoD;
}

bool orgRaycastSphereTest(highp float R2, highp float r2, highp float PoD) {
  return ((r2 > R2 && PoD < 0.0) || PoD*PoD + R2 < r2);
}

// hash / noise

highp float orgHash21(highp vec2 p) {
  highp vec2 r2 = (1.0/4093.0)*p + vec2(1.145141919, 0.0);
  highp float r = fract(dot(r2, r2)*4093.0);
  r = fract(r*r*4093.0);
  return r;
}

// value noise derivative (used by water normals)
highp vec2 orgValpar2D(highp vec2 p) {
  highp vec4 pi = vec4(floor(p), ceil(p));
  highp vec2 pf = fract(p);

  highp vec4 t = (1.0/4093.0)*pi + vec4(0.25, 0.0, 0.25, 0.0);
  t *= t;

  highp vec4 d = t.zxxz + t.wwyy;
  d = fract(d*4093.0);
  d = fract(d*d*4093.0);

  #if ORG_FAST_VALUE
    highp vec2 a = (-2.0*pf + 3.0)*pf*pf;
    highp vec2 b = (-6.0*pf + 6.0)*pf;
  #else
    highp vec2 a = ((6.0*pf - 15.0)*pf + 10.0)*pf*pf*pf;
    highp vec2 b = ((30.0*pf - 60.0)*pf + 30.0)*pf*pf;
  #endif

  return ((d.wy - d.zz) + (d.z + d.x - d.w - d.y)*a.yx)*b.xy;
}

#endif
