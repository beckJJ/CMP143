#version 330 core

layout( location = 0 ) in vec4 model_coefficients;
layout( location = 1 ) in vec4 color_coefficients;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

out vec4 cor_interpolada_pelo_rasterizador;

void
main()
{
    gl_Position = projectionMatrix * viewMatrix * modelMatrix * model_coefficients;
	cor_interpolada_pelo_rasterizador = color_coefficients;
}
