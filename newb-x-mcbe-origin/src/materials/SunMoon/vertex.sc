$input a_position, a_texcoord0
$output v_texcoord0

#include <bgfx_shader.sh>

// Origin Shader draws its own physical sun and moon inside the sky shader,
// so the vanilla sun/moon quads are not rendered.
void main() {
  v_texcoord0 = a_texcoord0;
  gl_Position = vec4(0.0, 0.0, 0.0, 0.0);
}
