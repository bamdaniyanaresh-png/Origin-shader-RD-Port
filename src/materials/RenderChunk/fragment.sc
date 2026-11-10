$input v_color0, v_color1, v_envlig, v_outsca, v_texcoord0, v_lightmapUV, v_position, v_addlig, v_skylum

#include <bgfx_shader.sh>
#include <newb/main.sh>

uniform vec4 TimeOfDay;
uniform vec4 ViewPositionAndTime;
uniform vec4 CameraPosition;

SAMPLER2D_AUTOREG(s_MatTexture);
SAMPLER2D_AUTOREG(s_SeasonsTexture);
SAMPLER2D_AUTOREG(s_LightMapTexture);

void main() {
  #if defined(DEPTH_ONLY_OPAQUE) || defined(DEPTH_ONLY) || defined(INSTANCING)
    gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);
    return;
  #endif

  vec4 diffuse = texture2D(s_MatTexture, v_texcoord0);

  #ifdef ALPHA_TEST
    if (diffuse.a < 0.6) {
      discard;
    }
  #endif

  vec4 varcol = v_color0;
  #if defined(SEASONS) && (defined(OPAQUE) || defined(ALPHA_TEST))
    varcol.rgb = mix(vec3(1.0, 1.0, 1.0), texture2D(s_SeasonsTexture, v_color1.xy).rgb * 2.0, v_color1.z);
  #endif

  highp vec3 wp = v_position;
  highp vec3 dpx = dFdx(wp);
  highp vec3 dpy = dFdy(wp);
  highp vec3 N = normalize(cross(dpx, dpy));
  if (dot(N, wp) > 0.0) {
    N = -N; // face the camera
  }
  vec2 dux = dFdx(v_texcoord0);
  vec2 duy = dFdy(v_texcoord0);

  highp vec3 gpos = wp + CameraPosition.xyz;
  vec2 ouv1 = orgUv1(v_lightmapUV);

  // sun light at the ground comes from the vertex shader
  vec3 grosunlig = vec3(v_outsca.w, v_addlig.w, v_skylum.w);

  vec4 oc = orgTerrainShade(
    diffuse, v_color1, varcol, ouv1, wp, gpos, N, dpx, dpy, dux, duy,
    v_envlig.rgb, v_outsca.rgb, v_addlig.rgb, grosunlig, v_skylum.rgb,
    v_envlig.w > 0.5, TimeOfDay.x, ViewPositionAndTime.w
  );

  #if NL_ORIGIN_CHUNK_BOUND || NL_ORIGIN_LIGHT_MAP
    vec3 blopos = fract(gpos);
  #endif
  #if NL_ORIGIN_CHUNK_BOUND
    oc.rgb = orgChunkBound(oc.rgb, mod(gpos, 16.0), N, blopos);
  #endif
  #if NL_ORIGIN_LIGHT_MAP == 1
    oc.rgb = orgLightBound(oc.rgb, ouv1, N, blopos);
  #endif

  gl_FragColor = oc;
}
