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
uniform int vertexShaderType;

#define NO_SHADER   0
#define GOURAUD_AD  1
#define GOURAUD_ADS 2

out vec4 fragColor;
out vec4 worldPosition;
out vec4 modelPosition;
out vec4 fragNormal;

subroutine vec4 vertexShader();

subroutine (vertexShader) vec4 gouraudADSshader() {
    vec4 origin = vec4(0.0, 0.0, 0.0, 1.0);
    vec4 cameraPosition = inverse(viewMatrix) * origin;
    
    vec3 Kd = vec3(1.0, 1.0, 1.0);
    vec3 Ks = vec3(1.0, 1.0, 1.0);
    vec3 Ka = vec3(0.2, 0.2, 0.2);
    vec3 Ia = vec3(0.2, 0.2, 0.2);
    float q = 32.0;

    vec4 lightDirection = normalize(vec4(1.0,1.0,0.0,0.0));
    vec4 viewDirection = normalize(cameraPosition - worldPosition);
    vec4 reflectionDirection = -lightDirection + 2 * fragNormal * dot(fragNormal, lightDirection);

    vec3 ambientTerm = Ka * Ia;
    vec3 lambertDiffuseTerm = Kd * max(0.0, dot(fragNormal, lightDirection));
    vec3 phongSpecularTerm = Ks * pow(max(0.0, dot(reflectionDirection, viewDirection)), q);
    
    vec4 outputColor = vec4(0.0);
    outputColor.rgb = (ambientTerm+lambertDiffuseTerm+phongSpecularTerm) * colorVector.rgb;
    outputColor.rgb = pow(outputColor.rgb, vec3(1.0,1.0,1.0)/2.2);
    outputColor.a = 1.0;
    return outputColor;
}

subroutine (vertexShader) vec4 gouraudADshader() {   
    vec3 Kd = vec3(1.0, 1.0, 1.0);
    vec3 Ka = vec3(0.2, 0.2, 0.2);
    vec3 Ia = vec3(0.2,0.2,0.2);

    vec4 lightDirection = normalize(vec4(1.0,1.0,0.0,0.0));

    vec3 ambientTerm = Ka * Ia;
    vec3 lambertDiffuseTerm = Kd * max(0.0, dot(fragNormal, lightDirection));
    
    vec4 outputColor = vec4(0.0);
    outputColor.rgb = (ambientTerm+lambertDiffuseTerm) * colorVector.rgb;
    outputColor.rgb = pow(outputColor.rgb, vec3(1.0,1.0,1.0)/2.2);
    outputColor.a = 1.0;
    return outputColor;
}

subroutine (vertexShader) vec4 noVertexShader() {
    return colorVector;
}

subroutine vec4 modelCoords();

subroutine (modelCoords) vec4 close2GLCoords() {
    return model_coefficients;
}

subroutine (modelCoords) vec4 openGLCoords() {
    worldPosition = modelMatrix * model_coefficients;
    fragNormal = inverse(transpose(modelMatrix)) * normal_coefficients;
    fragNormal.w = 0.0;
    fragNormal = normalize(normal_coefficients);
    
    return projectionMatrix * viewMatrix * modelMatrix * model_coefficients;
}

void main()
{
	if (useClose2GL) {
		gl_Position = close2GLCoords();
        fragColor = noVertexShader();
	} else {
		gl_Position = openGLCoords();
        switch(vertexShaderType) {
          case NO_SHADER: 
            fragColor = noVertexShader();
            break;
          case GOURAUD_AD:
            fragColor = gouraudADshader();
            break;
          case GOURAUD_ADS:
            fragColor = gouraudADSshader();
            break;
          default:
            fragColor = noVertexShader();
            break;
        }
    }
}
