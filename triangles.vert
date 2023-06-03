
#version 400 core

layout( location = 0 ) in vec4 vPosition;
layout( location = 1 ) in vec4 color_coefficients;

out vec4 cor_interpolada_pelo_rasterizador;

void
main()
{
    gl_Position = vPosition;
	cor_interpolada_pelo_rasterizador = color_coefficients;
}
