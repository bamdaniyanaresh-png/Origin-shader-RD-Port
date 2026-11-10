$input a_indices, a_color0, a_normal, a_position, a_texcoord0
#ifdef INSTANCING
  $input i_data1, i_data2, i_data3
#endif
$output v_color0, v_fog, v_light, v_texcoord0, v_addlig

#include <bgfx_shader.sh>
#include <MinecraftRenderer.Materials/DynamicUtil.dragonh>
#include <MinecraftRenderer.Materials/TAAUtil.dragonh>
#include <newb/main.sh>

uniform vec4 FogControl;
uniform vec4 FogColor;
uniform vec4 TileLightColor;
uniform vec4 OverlayColor;
uniform mat4 Bones[8];
uniform vec4 UVAnimation;
uniform vec4 TimeOfDay;
uniform vec4 ViewPositionAndTime;
uniform vec4 CameraPosition;
uniform vec4 RenderDistance;


void main() {
  mat4 World = u_model[0];

  World = mul(World, Bones[int(a_indices)]);

  vec2 texcoord0 = a_texcoord0;
  texcoord0 = applyUvAnimation(texcoord0, UVAnimation);

  vec3 worldPosition;
  #if defined(INSTANCING)
    mat4 model = mtxFromCols(i_data1, i_data2, i_data3, vec4(0.0, 0.0, 0.0, 1.0));
    worldPosition = instMul(model, vec4(a_position, 1.0)).xyz;
  #else
    worldPosition = mul(World, vec4(a_position, 1.0)).xyz;
  #endif

  vec4 position = jitterVertexPosition(worldPosition);

  #if !(defined(DEPTH_ONLY_OPAQUE) || defined(DEPTH_ONLY) || defined(INSTANCING))
    vec3 orgN = normalize(mul(World, a_normal)).xyz;
    vec3 orgLight;
    vec3 orgOutsca;
    vec3 orgAddlig;
    orgEntityVertex(TimeOfDay.x, worldPosition.xyz, orgN, TileLightColor, orgLight, orgOutsca, orgAddlig);

    v_texcoord0 = texcoord0;
    v_color0 = a_color0;
    v_fog = vec4(orgOutsca, 1.0);
    v_addlig = vec4(orgAddlig, 0.0);
    v_light = vec4(orgLight, 1.0);
  #endif
  gl_Position = position;
}