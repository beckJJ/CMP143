#version 330 core

layout( location = 0 ) in vec3 vPosition;
layout( location = 1 ) in vec4 color_coefficients;

uniform mat4 viewProjectionMatrix;

out vec4 cor_interpolada_pelo_rasterizador;

void
main()
{
    gl_Position = viewProjectionMatrix * vec4(vPosition, 1.0);
	cor_interpolada_pelo_rasterizador = color_coefficients;
}
