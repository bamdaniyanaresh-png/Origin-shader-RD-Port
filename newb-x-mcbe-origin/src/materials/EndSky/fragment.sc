#ifndef INSTANCING
$input v_texcoord0, v_posTime
#endif

#include <bgfx_shader.sh>

#ifndef INSTANCING
  #include <newb/main.sh>

  SAMPLER2D_AUTOREG(s_SkyTexture);
#endif

void main() {
  #ifndef INSTANCING
    vec4 diffuse = texture2D(s_SkyTexture, v_texcoord0);

    // Origin has no End sky: the sky texture is shown as light (Origin: MILKY_WAY_LIGHT)
    vec3 color = nlOriginLig2Col(nlOriginGammaToLinear(diffuse.rgb)*NL_ORIGIN_END_SKY_LIGHT);

    gl_FragColor = vec4(color, 1.0);
  #else
    gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);
  #endif
}
