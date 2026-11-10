#ifndef INSTANCING
  $input v_worldPos, v_skylum, v_clolig, v_orisun, v_sundir
#endif

#include <bgfx_shader.sh>

#ifndef INSTANCING
  #include <newb/main.sh>

  uniform vec4 ViewPositionAndTime;
#endif

void main() {
  #ifndef INSTANCING
    highp vec3 viewDir = normalize(v_worldPos);

    highp vec3 campos = vec3(0.0, ORG_EARTH_R + ORG_ELEVATION, 0.0);
    highp vec3 orisunlig = v_orisun.rgb;
    highp vec3 L = normalize(v_sundir.xyz);

    // below the horizon the terrain covers the sky, half the steps are enough
    #ifdef NL_ORIGIN_FULL_STEPS
      float q = 1.0;
    #else
      float q = viewDir.y < -0.02 ? 0.5 : 1.0;
    #endif

    // sun / moon disc, air scattering and clouds
    highp vec3 skylig = orgPhysicSun(vec3_splat(0.0), orisunlig, L, viewDir, ORG_SUN_R);
    skylig = orgSkyRender(skylig, campos, viewDir, orisunlig, v_clolig.rgb, v_skylum.rgb, L, ViewPositionAndTime.w, q);

    gl_FragColor = vec4(nlOriginLig2Col(skylig), 1.0);
  #else
    gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);
  #endif
}
