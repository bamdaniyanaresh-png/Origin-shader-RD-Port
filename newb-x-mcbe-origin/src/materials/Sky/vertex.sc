#ifndef INSTANCING
  $input a_color0, a_position
  $output v_worldPos, v_skylum, v_clolig, v_orisun, v_sundir
#endif

#include <bgfx_shader.sh>

#ifndef INSTANCING
  #include <newb/main.sh>

  uniform vec4 TimeOfDay;
#endif

void main() {
  #ifndef INSTANCING
    // background quad
    vec4 pos = vec4(a_position.xzy, 1.0);
    pos.xy = 2.0*clamp(pos.xy, -0.5, 0.5);

    v_worldPos = mul(u_invViewProj, pos).xyz;

    // sun and cloud light are the same for every pixel, so they are done here
    highp vec3 campos = vec3(0.0, ORG_EARTH_R + ORG_ELEVATION, 0.0);
    highp vec3 orisunlig;
    highp vec3 grosunlig;
    highp vec3 L;
    orgSetSunLig(TimeOfDay.x, campos, orisunlig, grosunlig, L);

    highp vec3 closunlig = grosunlig;
    highp vec3 closkylum = vec3_splat(0.0);
    #if ORG_CLOUDS
      highp vec3 clocampos = vec3(0.0, ORG_EARTH_R + ORG_ELEVATION + ORG_CLOUD_Y, 0.0);
      closunlig = orgGetSunLig(clocampos, orisunlig, L);
      closkylum = orgPhysicsSky(vec3_splat(0.0), clocampos, vec3(0.0, 0.8, 0.6), orisunlig, L, ORG_MARNUM);
    #endif

    v_skylum = vec4(closkylum, 0.0);
    v_clolig = vec4(closunlig, 0.0);
    v_orisun = vec4(orisunlig, 0.0);
    v_sundir = vec4(L, 0.0);

    gl_Position = pos;
  #else
    gl_Position = vec4(0.0, 0.0, 0.0, 0.0);
  #endif
}
