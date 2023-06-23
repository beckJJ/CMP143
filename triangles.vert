#version 450 core

layout( location = 0 ) in vec4 model_coefficients;
layout( location = 1 ) in vec4 color_coefficients;
layout( location = 2 ) in vec4 normal_coefficients;

uniform vec4 colorVector;
uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

uniform vec4 lightPos;

uniform bool useClose2GL;

out vec4 fragColor;
out vec4 position_world;
out vec4 position_model;
out vec4 fragNormal;

void main()
{
	if (useClose2GL) {
		gl_Position = model_coefficients;
        fragColor = colorVector;
	} else {
		gl_Position = projectionMatrix * viewMatrix * modelMatrix * model_coefficients;

        vec4 worldPosition = modelMatrix * model_coefficients;
        vec4 viewPosition = viewMatrix * worldPosition;
        fragNormal = inverse(transpose(modelMatrix)) * normal_coefficients;
        fragNormal.w = 0.0;
        
        vec4 origin = vec4(0.0, 0.0, 0.0, 1.0);
        vec4 cameraPosition = inverse(viewMatrix) * origin;
        
        vec3 Kd = vec3(1.0, 1.0, 1.0);
        vec3 Ks = vec3(1.0, 1.0, 1.0);
        vec3 Ka = vec3(0.2, 0.2, 0.2);
        vec3 Ia = vec3(0.2,0.2,0.2);
        float q = 32.0;

        vec4 normal = normalize(fragNormal);
        vec4 lightDirection = normalize(vec4(1.0,1.0,0.0,0.0));
        vec4 viewDirection = normalize(cameraPosition - worldPosition);
        vec4 reflectionDirection = -lightDirection + 2 * normal * dot(normal, lightDirection);

        vec3 ambientTerm = Ka * Ia;
        vec3 lambertDiffuseTerm = Kd * max(0.0, dot(normal, lightDirection));
        vec3 phongSpecularTerm = Ks * pow(max(0.0, dot(reflectionDirection, viewDirection)), q);
        fragColor.rgb = (ambientTerm+lambertDiffuseTerm+phongSpecularTerm) * colorVector.rgb;
        fragColor.rgb = pow(fragColor.rgb, vec3(1.0,1.0,1.0)/2.2);
        fragColor.a = 1.0;
    }
}
