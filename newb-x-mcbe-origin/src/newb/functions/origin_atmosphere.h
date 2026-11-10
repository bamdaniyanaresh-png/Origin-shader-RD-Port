#ifndef ORIGIN_ATMOSPHERE_H
#define ORIGIN_ATMOSPHERE_H

// Atmosphere from Origin Shader: Rayleigh, Mie and ozone scattering.
// Units are km, the camera is at (0, EARTH_R + ELEVATION, 0). Directions use world axes.

#include "origin_config.h"
#include "origin_math.h"
#include "origin_tonemap.h"

#define ORG_SUNLIG (nlOriginGammaToLinear(ORG_SUNCOL)*ORG_SUNLIGHT_A)
#define ORG_MOOLIG (nlOriginGammaToLinear(ORG_MOOCOL)*ORG_MOOLIGHT_A)
#define ORG_POILIG (nlOriginGammaToLinear(ORG_POICOL)*ORG_POILIGHT_A)
#define ORG_ENVLIG (nlOriginGammaToLinear(ORG_ENVCOL)*ORG_ENVLIGHT_A)

// phase functions

float orgGetPr(float c) {
  return (1.0/(4.0*ORG_PI))*(8.0/10.0)*(7.0/5.0 + 0.5*c);
}

float orgGetPm4(float c, vec4 pm4, vec4 g4) {
  vec4 g2 = g4*g4;
  vec4 tmp = (3.0/(8.0*ORG_PI))*((1.0 - g2)*(1.0 + c*c))/((2.0 + g2)*orgPow3_2v4(1.0 + g2 - 2.0*g4*c));
  return dot(tmp, pm4);
}

// optical depth (Chapman approximation)

highp float orgChapman(highp float X, highp float h, highp float cosZenith) {
  highp float c = sqrt(X + h);
  highp float cExpH = c*exp(-h);

  if (cosZenith >= 0.0) {
    return cExpH/(c*cosZenith + 1.0);
  }

  // sun below the horizon, exp argument is clamped (full shadow when it is huge)
  highp float x0 = sqrt(max(1.0 - cosZenith*cosZenith, 0.0))*(X + h);
  highp float c0 = sqrt(x0);
  return 2.0*c0*exp(min(X - x0, 70.0)) - cExpH/(1.0 - c*cosZenith);
}

highp float orgOpticalDepth(highp float h, highp float H, highp float earthRadius, highp float cosZenith) {
  return H*orgChapman(earthRadius/H, h/H, cosZenith);
}

highp float orgComputeOpticalDepth(highp vec3 pos, highp vec3 dir, highp float H) {
  highp float r = length(pos);
  return orgOpticalDepth(r - ORG_EARTH_R, H, ORG_EARTH_R, dot(pos, dir)/r);
}

highp vec2 orgAtmads(highp vec3 sampos, highp vec3 sunvec) {
  highp float r = length(sampos);
  highp float cz = dot(sampos, sunvec)/r;
  highp float h = r - ORG_EARTH_R;
  return vec2(orgOpticalDepth(h, ORG_HR, ORG_EARTH_R, cz), orgOpticalDepth(h, ORG_HM, ORG_EARTH_R, cz));
}

// same result as orgAtmads when radius r and the densities exp(-h/H) of the point are already known
// c = sqrt(r/HM) is c = sqrt(r/HR) times a constant, so only one sqrt is needed
highp vec2 orgAtmadsKnown(highp float r, highp float cz, highp vec2 den) {
  const float XR = ORG_EARTH_R/ORG_HR;
  const float XM = ORG_EARTH_R/ORG_HM;
  const float cscale = sqrt(ORG_HR/ORG_HM);

  highp float cR = sqrt(r/ORG_HR);
  highp float cM = cR*cscale;
  highp float expR = cR*den.x;
  highp float expM = cM*den.y;

  if (cz >= 0.0) {
    return vec2(ORG_HR*(expR/(cR*cz + 1.0)), ORG_HM*(expM/(cM*cz + 1.0)));
  }

  highp float s = sqrt(max(1.0 - cz*cz, 0.0));
  highp float x0R = s*(r/ORG_HR);
  highp float x0M = s*(r/ORG_HM);
  highp float tR = 2.0*sqrt(x0R)*exp(min(XR - x0R, 70.0)) - expR/(1.0 - cR*cz);
  highp float tM = 2.0*sqrt(x0M)*exp(min(XM - x0M, 70.0)) - expM/(1.0 - cM*cz);
  return vec2(ORG_HR*tR, ORG_HM*tM);
}

// sky color along a ray

highp vec3 orgPhysicsSky(highp vec3 skylig, highp vec3 marpos, highp vec3 marvec, highp vec3 orisunlig, highp vec3 sunvec, float steps) {
  const float R2 = ORG_EARTH_r*ORG_EARTH_r;
  const float R2_99995 = (ORG_EARTH_r*0.99995)*(ORG_EARTH_r*0.99995);

  highp float PoV = dot(marpos, -marvec);
  highp float PoL = dot(marpos, -sunvec);

  highp vec2 admsum = vec2_splat(0.0);

  highp vec3 t0 = vec3_splat(0.0);
  highp vec3 t1;
  t1.z = dot(marpos, marpos);
  t1.xy = exp(sqrt(t1.z)*ORG_NEG_INV_H + ORG_R_OVER_H);

  highp vec3 marsumr = vec3_splat(0.0);
  highp vec3 expodr0 = vec3_splat(0.0);
  highp vec3 expodr1 = vec3_splat(0.0);

  highp vec3 marsumm = vec3_splat(0.0);
  highp vec3 expodm0 = vec3_splat(0.0);
  highp vec3 expodm1 = vec3_splat(0.0);

  #if NL_ORIGIN_PLANET_ATMOSPHERE
  if (orgRaycastSphereTest(R2, t1.z, PoL))
  #endif
  {
    highp vec2 adorigin = orgAtmads(marpos, sunvec);
    highp vec3 expodorigin = exp((-ORG_RO)*adorigin.x + (-ORG_LM)*adorigin.y);

    expodr1 = expodorigin*t1.x;
    expodm1 = expodorigin*t1.y;
  }

  highp float t = PoV*PoV + R2;
  bool r2R2 = t1.z > R2;
  bool nohitflag = ((r2R2 && PoV < 0.0) || t < t1.z);

  highp float mpx;
  highp float em;
  highp float dp;

  if (nohitflag) {
    highp vec3 maxpos = marpos + max(0.0, PoV)*marvec;
    mpx = exp(length(maxpos)*(-1.0/ORG_HR) + (ORG_EARTH_R/ORG_HR));
    em = mpx - t1.x;
    dp = (-2.0*mpx + t1.x)*(0.99999/steps);
  } else {
    highp float q = sqrt(max(t - t1.z, 0.0));
    highp vec3 maxpos = marpos + ((r2R2 ? -q : q) + PoV)*marvec;
    mpx = exp(length(maxpos)*(-1.0/ORG_HR) + (ORG_EARTH_R/ORG_HR));
    em = mpx - t1.x;
    dp = -em*(0.99999/steps);
  }

  for (float j = 1.0; j <= ORG_MARNUM; j++) {
    if (j > steps) {
      break;
    }
    t0 = t1;

    t1.x = -abs(em + dp*j) + mpx;
    highp float h = log(t1.x)*(-ORG_HR);
    highp float r = h + ORG_EARTH_R;
    t1.y = exp(h*(-1.0/ORG_HM));
    t1.z = r*r;

    PoV = dot(marpos, -marvec);
    highp float l = orgRaycastSphereLength(t1.z, t0.z, PoV);

    marpos += l*marvec;

    admsum += (t0.xy + t1.xy)*0.5*l;

    PoL = dot(marpos, -sunvec);

    #if NL_ORIGIN_PLANET_ATMOSPHERE
    if (orgRaycastSphereTest(R2_99995, t1.z, PoL))
    #endif
    {
      highp vec2 adssum = orgAtmadsKnown(r, dot(marpos, sunvec)/r, t1.xy);

      highp vec2 ad = adssum + admsum;
      highp vec3 od = ORG_RO*ad.x + ORG_LM*ad.y;
      highp vec3 expod = exp(-od);

      expodr0 = expodr1;
      expodr1 = expod*t1.x;
      marsumr += (expodr0 + expodr1)*l;

      expodm0 = expodm1;
      expodm1 = expod*t1.y;
      marsumm += (expodm0 + expodm1)*l;
    }
  }
  marsumr *= 0.5;
  marsumm *= 0.5;

  highp float c = dot(sunvec, marvec);
  highp float pr = orgGetPr(c);
  highp float pm = orgGetPm4(c, ORG_PM_AIR, ORG_G_AIR);

  highp vec3 insca = orisunlig*(pr*ORG_LR*marsumr + pm*ORG_LM*marsumm);
  #if ORG_EARTH_SHADOW
    skylig = nohitflag ? skylig : vec3_splat(0.0);
  #endif
  highp vec3 outsca = skylig*exp((-ORG_RO)*admsum.x + (-ORG_LM)*admsum.y);

  return max(vec3_splat(0.0), outsca + insca);
}

// aerial perspective between two points

void orgAtmsca(out highp vec3 outsca, out highp vec3 insca, highp vec3 stapos, highp vec3 marpos, highp vec3 sunvec, vec4 pm4, vec4 g4, float steps) {
  highp vec3 delpos = stapos - marpos;
  highp vec3 marvec = normalize(delpos);

  highp float r2 = dot(marpos, marpos);

  highp vec2 admsum = vec2_splat(0.0);
  highp vec2 adm0;
  highp vec2 adm1 = exp(sqrt(r2)*ORG_NEG_INV_H + ORG_R_OVER_H);

  highp vec3 marsumr = vec3_splat(0.0);
  highp vec3 expodr0;
  highp vec3 expodr1 = vec3_splat(0.0);

  highp vec3 marsumm = vec3_splat(0.0);
  highp vec3 expodm0;
  highp vec3 expodm1 = vec3_splat(0.0);

  highp vec2 adorigin = orgAtmads(marpos, sunvec);
  highp vec3 expodorigin = exp((-ORG_RO)*adorigin.x + (-ORG_LM)*adorigin.y);

  expodr1 = expodorigin*adm1.x;
  expodm1 = expodorigin*adm1.y;

  highp float l = length(delpos)/steps;

  for (float j = 1.0; j <= ORG_MARNUM; j++) {
    if (j > steps) {
      break;
    }
    marpos += l*marvec;

    r2 = dot(marpos, marpos);
    highp float rr = sqrt(r2);

    adm0 = adm1;
    adm1 = exp(rr*ORG_NEG_INV_H + ORG_R_OVER_H);
    admsum += (adm0 + adm1)*0.5*l;

    highp vec2 adssum = orgAtmadsKnown(rr, dot(marpos, sunvec)/rr, adm1);

    highp vec2 ad = adssum + admsum;
    highp vec3 od = ORG_RO*ad.x + ORG_LM*ad.y;
    highp vec3 expod = exp(-od);

    expodr0 = expodr1;
    expodr1 = expod*adm1.x;
    marsumr += (expodr0 + expodr1)*l;

    expodm0 = expodm1;
    expodm1 = expod*adm1.y;
    marsumm += (expodm0 + expodm1)*l;
  }

  marsumr *= 0.5;
  marsumm *= 0.5;

  highp float c = dot(sunvec, marvec);
  highp float pr = orgGetPr(c);
  highp float pm = orgGetPm4(c, pm4, g4);

  insca = max(vec3_splat(0.0), pr*ORG_LR*marsumr + pm*ORG_LM*marsumm);
  outsca = max(vec3_splat(0.0), exp((-ORG_RO)*admsum.x + (-ORG_LM)*admsum.y));
}

// light that reaches a point after passing through the atmosphere
highp vec3 orgGetSunLig(highp vec3 marpos, highp vec3 orisunlig, highp vec3 sunvec) {
  highp vec2 adorigin = orgAtmads(marpos, sunvec);
  return max(vec3_splat(0.0), orisunlig*exp((-ORG_RO)*adorigin.x + (-ORG_LM)*adorigin.y));
}

// sun / moon disc
highp vec3 orgPhysicSun(highp vec3 skylig, highp vec3 orisunlig, highp vec3 sunvec, highp vec3 skyvec, float r) {
  const vec3 a = vec3(0.397, 0.503, 0.652);
  float s = 2.0*ORG_PI*(1.0 - cos(r));

  float cosTheta = dot(skyvec, sunvec);
  if (cosTheta < cos(r)) {
    return skylig;
  }
  float theta = acos(clamp(cosTheta, -1.0, 1.0));

  if (theta < r) {
    vec3 lig = orisunlig/s;
    float mu = sqrt(max(1.0 - orgPow2(theta/r), 0.0));
    vec3 factor = pow(vec3_splat(mu), a);
    return skylig + min(vec3_splat(10000.0), lig*factor);
  }
  return skylig;
}

// sun direction from the time of day (TimeOfDay.x: 0 noon, 0.25 sunset, 0.5 midnight, 0.75 sunrise)
// real sun direction is (-sin(2*pi*t), cos(2*pi*t), 0), at night the sun light becomes the moon
void orgSunLocal(float tod, out highp vec3 orisunlig, out highp vec3 sunvec) {
  float a = 2.0*ORG_PI*tod;
  sunvec = vec3(-sin(a), cos(a), 0.0);
  bool day = sunvec.y > ORG_SUN_RISE_Y;
  sunvec = day ? sunvec : -sunvec;
  orisunlig = day ? ORG_SUNLIG : ORG_MOOLIG;

  // latitude tilt
  float cl = cos(ORG_LATITUDE);
  float sl = sin(ORG_LATITUDE);
  float y = sunvec.y;
  float z = sunvec.z;
  sunvec.y = cl*y - sl*z;
  sunvec.z = sl*y + cl*z;
  sunvec.x += 0.00001; // keeps the cloud shading from turning black at noon
  sunvec = normalize(sunvec);
}

void orgSetSunLig(float tod, highp vec3 campos, out highp vec3 orisunlig, out highp vec3 sunlig, out highp vec3 sunvec) {
  orgSunLocal(tod, orisunlig, sunvec);
  sunlig = orgGetSunLig(campos, orisunlig, sunvec);
}

#endif
