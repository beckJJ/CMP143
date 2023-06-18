#version 330 core

layout( location = 0 ) in vec4 model_coefficients;
layout( location = 1 ) in vec4 color_coefficients;

uniform vec4 colorVector;
uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

uniform bool useClose2GL;

out vec4 cor_interpolada_pelo_rasterizador;

void
main()
{
	if (useClose2GL) {
		gl_Position = model_coefficients;
	} else {
		gl_Position = projectionMatrix * viewMatrix * modelMatrix * model_coefficients;
	}
	cor_interpolada_pelo_rasterizador = colorVector;
}
