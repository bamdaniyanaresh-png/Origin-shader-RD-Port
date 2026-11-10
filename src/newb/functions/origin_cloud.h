#ifndef ORIGIN_CLOUD_H
#define ORIGIN_CLOUD_H

// Clouds and the full sky ray from Origin Shader.
// NL_ORIGIN_CLOUD_TYPE 2 is the box cloud, 3 is the jelly cloud.

#include "origin_atmosphere.h"

#if ORG_CLOUDS

bool orgIsCloud(highp vec3 p0) {
  return orgHash21(p0.xz) < ORG_CLOUD_COVERAGE;
}

// optical length of cloud between sample point and the sun
highp float orgCloSamSun(highp vec3 sampos, highp vec3 aabbsam, highp vec3 sunvec, float samnum) {
  highp float samsum = 0.0;
  for (float k = 0.0; k < ORG_CLOSAMNUM && k < samnum && abs(aabbsam.y - ORG_CLOUD_Y) < 0.001; k++) {
    highp vec4 intsun = orgRaymarchAABB(sampos, sunvec, aabbsam, aabbsam + ORG_CLOSIZ);
    samsum += orgIsCloud(aabbsam) ? intsun.w : 0.0;
    sampos += intsun.w*sunvec;
    aabbsam += intsun.xyz*ORG_CLOSIZ;
  }
  return samsum;
}

#if NL_ORIGIN_CLOUD_TYPE == 2

highp vec3 orgCloudRenderFast(highp vec3 skylig, highp vec3 campos, highp vec3 marvec, highp vec3 sunlig, highp float gametime) {
  highp vec3 marpos = campos;
  marpos.xz += gametime*ORG_CLOUD_SPEED;
  highp vec3 aabbmar = floor(marpos*(1.0/ORG_CLOSIZ))*ORG_CLOSIZ;

  highp float marsum = 0.0;

  for (float i = 0.0; i < 8.0 && abs(aabbmar.y - ORG_CLOUD_Y) < 0.001; i++) {
    highp vec4 intclo = orgRaymarchAABB(marpos, marvec, aabbmar, aabbmar + ORG_CLOSIZ);
    if (orgIsCloud(aabbmar)) {
      marsum += intclo.w;
    }
    marpos += intclo.w*marvec;
    aabbmar += intclo.xyz*ORG_CLOSIZ;
  }

  return skylig + (1.0/(4.0*ORG_PI))*marsum*sunlig;
}

#elif NL_ORIGIN_CLOUD_TYPE == 3

highp vec3 orgCloudRender(highp vec3 skylig, highp vec3 campos, highp vec3 marvec, highp vec3 sunlig, highp vec3 sunvec, highp vec3 skylum, highp float gametime, float q) {
  float cloint = max(1.0, floor(ORG_CLOINTNUM*q));
  float clomar = max(1.0, floor(ORG_CLOMARNUM*q));
  float closam = max(1.0, floor(ORG_CLOSAMNUM*q));
  highp vec3 marpos = campos;
  marpos.xz += gametime*ORG_CLOUD_SPEED;
  highp vec3 aabbmar = floor(marpos*(1.0/ORG_CLOSIZ))*ORG_CLOSIZ;

  highp float marsum = 0.0;
  highp float expsum = 0.0;

  for (float i = 0.0; i < ORG_CLOINTNUM && i < cloint && abs(aabbmar.y - ORG_CLOUD_Y) < 0.001; i++) {
    highp vec4 intclo = orgRaymarchAABB(marpos, marvec, aabbmar, aabbmar + ORG_CLOSIZ);

    if (orgIsCloud(aabbmar)) {
      highp float l = intclo.w/clomar;
      highp vec3 marstep = l*marvec;

      highp vec3 sampos = marpos;

      highp vec2 t0;
      highp vec2 t1;
      t1.x = orgCloSamSun(sampos, aabbmar, sunvec, closam);
      t1.y = exp(-ORG_CLOUD_LM*(marsum + t1.x));

      highp float s = 0.0;

      for (float j = 0.0; j < ORG_CLOMARNUM && j < clomar; j++) {
        sampos += marstep;

        t0 = t1;

        t1.x = orgCloSamSun(sampos, aabbmar, sunvec, closam);
        highp float t_a = t1.x + l;
        t1.y = exp(-ORG_CLOUD_LM*(marsum + t_a));
        // when the view ray points at the sun (sunrise/sunset) the denominator goes to 0 (0/0).
        // the limit of (e^-a - e^-b)/(b - a) for b -> a is LM*e^-a, so use that instead.
        highp float den = t_a - t0.x;
        s += abs(den) > 0.0001 ? (t0.y - t1.y)/den : ORG_CLOUD_LM*t0.y;

        marsum += l;
      }
      expsum += s*l*(1.0/ORG_CLOUD_LM);
    }

    marpos += intclo.w*marvec;
    aabbmar += intclo.xyz*ORG_CLOSIZ;
  }

  highp float c = dot(sunvec, marvec);
  highp float pm = orgGetPm4(c, ORG_PM_CLOUD, ORG_G_CLOUD);

  highp vec3 insca = sunlig*(pm*ORG_CLOUD_LM*expsum);
  highp vec3 outsca = skylig*exp(-ORG_CLOUD_LM*marsum);

  return insca + outsca + skylum;
}

#endif
#endif // ORG_CLOUDS

// aerial perspective applied to a far point (clouds seen through air)
highp vec3 orgSkyFog(highp vec3 skylig, highp vec3 stapos, highp vec3 marpos, highp vec3 orisunlig, highp vec3 sunvec, float steps) {
  highp vec3 insca;
  highp vec3 outsca;
  orgAtmsca(outsca, insca, stapos, marpos, sunvec, ORG_PM_AIR, ORG_G_AIR, steps);
  return skylig*outsca + orisunlig*insca;
}

// full sky along one ray: air + clouds (Origin: skyrender)
// sunlig  = sun light reaching the cloud layer (orgGetSunLig at cloud camera pos)
// skylum  = sky luminance used as cloud ambient (orgPhysicsSky straight up-ish from cloud camera pos)
highp vec3 orgSkyRender(highp vec3 skylig, highp vec3 campos, highp vec3 skyvec, highp vec3 orisunlig, highp vec3 sunlig, highp vec3 skylum, highp vec3 sunvec, highp float gametime, float q) {
  float steps = max(2.0, floor(ORG_MARNUM*q));
  #if !ORG_CLOUDS
    return orgPhysicsSky(skylig, campos, skyvec, orisunlig, sunvec, steps);
  #else
    const float R2 = ORG_EARTH_r*ORG_EARTH_r;
    const float R2_Y = (ORG_EARTH_R + ORG_CLOUD_Y)*(ORG_EARTH_R + ORG_CLOUD_Y);
    const float R2_YH = (ORG_EARTH_R + ORG_CLOUD_Y + ORG_CLOUD_H)*(ORG_EARTH_R + ORG_CLOUD_Y + ORG_CLOUD_H);

    bool need_render_cloud = true;
    highp vec3 real_cloud_position = campos;
    highp vec3 fake_cloud_position = campos;

    highp float r2 = dot(campos, campos);
    highp float PoV = dot(campos, -skyvec);

    if (campos.y < ORG_EARTH_R + ORG_CLOUD_Y) {
      highp float cloud_distance = orgRaycastSphere(R2_Y, r2, PoV);

      bool no_hit_earth = orgRaycastSphereTest(R2, r2, PoV);
      if ((cloud_distance < -0.5) || (!no_hit_earth)) {
        need_render_cloud = false;
      } else {
        real_cloud_position = campos + skyvec*cloud_distance;
        fake_cloud_position = real_cloud_position;
        fake_cloud_position.y = ORG_CLOUD_Y + 0.00001;
      }
    } else if (campos.y > ORG_EARTH_R + ORG_CLOUD_Y + ORG_CLOUD_H) {
      highp float cloud_distance = orgRaycastSphere(R2_YH, r2, PoV);
      if (cloud_distance < -0.5) {
        need_render_cloud = false;
      } else {
        real_cloud_position = campos + skyvec*cloud_distance;
        fake_cloud_position = real_cloud_position;
        fake_cloud_position.y = ORG_CLOUD_Y + ORG_CLOUD_H - 0.00001;
      }
    }

    if (need_render_cloud) {
      skylig = orgPhysicsSky(skylig, real_cloud_position, skyvec, orisunlig, sunvec, steps);
      #if NL_ORIGIN_CLOUD_TYPE == 2
        skylig = orgCloudRenderFast(skylig, fake_cloud_position, skyvec, sunlig, gametime);
      #else
        skylig = orgCloudRender(skylig, fake_cloud_position, skyvec, sunlig, sunvec, skylum, gametime, q);
      #endif
      skylig = orgSkyFog(skylig, real_cloud_position, campos, orisunlig, sunvec, steps);
    } else {
      skylig = orgPhysicsSky(skylig, campos, skyvec, orisunlig, sunvec, steps);
    }
    return skylig;
  #endif
}

#endif
