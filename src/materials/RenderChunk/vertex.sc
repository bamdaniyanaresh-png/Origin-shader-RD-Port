$input a_color0, a_position, a_texcoord0, a_texcoord1
#ifdef INSTANCING
  $input i_data0, i_data1, i_data2, i_data3
#endif
$output v_color0, v_color1, v_envlig, v_outsca, v_texcoord0, v_lightmapUV, v_position, v_addlig, v_skylum

#include <bgfx_shader.sh>
#include <newb/main.sh>

uniform vec4 ViewPositionAndTime;
uniform vec4 TimeOfDay;

void main() {
  #ifdef INSTANCING
    mat4 model = mtxFromCols(i_data0, i_data1, i_data2, i_data3);
  #else
    mat4 model = u_model[0];
  #endif

  vec3 worldPos = mul(model, vec4(a_position, 1.0)).xyz;

  #if !(defined(DEPTH_ONLY_OPAQUE) || defined(DEPTH_ONLY) || defined(INSTANCING))

  #ifdef RENDER_AS_BILLBOARDS
    worldPos += vec3(0.5, 0.5, 0.5);

    vec3 modelCamPos = ViewPositionAndTime.xyz - worldPos;
    float camDis = length(modelCamPos);
    vec3 viewDir = modelCamPos / camDis;

    vec3 boardPlane = normalize(vec3(-viewDir.z, 0.0, viewDir.x));
    worldPos -= (((viewDir.zxy * boardPlane.yzx) - (viewDir.yzx * boardPlane.zxy)) *
                 (a_color0.z - 0.5)) +
                 (boardPlane * (a_color0.x - 0.5));
    vec4 color = vec4(1.0, 1.0, 1.0, 1.0);
  #else
    vec4 color = a_color0;
  #endif

  // bit 16 for dithering??
  // bit 15-1 for texture mapping
  vec2 uv0 = 2.0*a_texcoord0.xy;
  uv0 = fract(uv0) + ((floor(uv0)-0.5)/16384.0);

  // bits 8-5 block light, bits 4-1 sky light (level/15)
  vec2 uv1 = fract(a_texcoord1.y*vec2(256.0, 4096.0));

  // light map in Origin scale (level/16)
  vec2 ouv1 = orgUv1(uv1);

  // tint + ambient occlusion
  #ifdef SEASONS
    vec4 varcol = vec4(1.0, 1.0, 1.0, color.a);
  #else
    vec4 varcol = orgGetVarCol(color);
  #endif

  vec3 envlig;
  vec3 outsca;
  vec3 addlig;
  vec3 grosunlig;
  vec3 closkylum;
  orgTerrainVertex(TimeOfDay.x, worldPos, ouv1, varcol.a, envlig, outsca, addlig, grosunlig, closkylum);

  // water has a blue vertex colour with transparency
  float isWater = 0.0;
  #if defined(TRANSPARENT) && !(defined(RENDER_AS_BILLBOARDS) || defined(SEASONS))
    if (a_color0.b > 0.3 && a_color0.a < 0.95) {
      isWater = 1.0;
    }
  #endif

  v_color0 = varcol;
  v_color1 = color;
  v_envlig = vec4(envlig, isWater);
  v_outsca = vec4(outsca, grosunlig.r);
  v_texcoord0 = uv0;
  v_lightmapUV = uv1;
  v_position = worldPos;
  v_addlig = vec4(addlig, grosunlig.g);
  v_skylum = vec4(closkylum, grosunlig.b);

  #endif

  gl_Position = mul(u_viewProj, vec4(worldPos, 1.0));
}
