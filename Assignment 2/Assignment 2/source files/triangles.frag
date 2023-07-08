#version 450 core

in vec4 fragColor;
in vec4 worldPosition;
in vec4 modelPosition;
in vec4 fragNormal;

uniform int fragmentShaderType;

#define NO_SHADER 0
#define PHONG     1


uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;
uniform vec4 LightDir;

out vec4 fColor;

subroutine vec4 fragmentShader();

subroutine (fragmentShader) vec4 phongShader() {
    vec4 origin = vec4(0.0, 0.0, 0.0, 1.0);
    vec4 cameraPosition = inverse(viewMatrix) * origin;
    
    vec3 Kd = vec3(1.0, 1.0, 1.0);
    vec3 Ks = vec3(1.0, 1.0, 1.0);
    vec3 Ka = vec3(0.2, 0.2, 0.2);
    vec3 Ia = vec3(0.2, 0.2, 0.2);
    float q = 32.0;

    vec4 fragNormal = normalize(fragNormal);
    vec4 lightDirection = normalize(vec4(1.0,1.0,0.0,0.0));
    vec4 viewDirection = normalize(cameraPosition - worldPosition);
    vec4 reflectionDirection = -lightDirection + 2 * fragNormal * dot(fragNormal, lightDirection);

    vec3 ambientTerm = Ka * Ia;
    vec3 lambertDiffuseTerm = Kd * max(0.0, dot(fragNormal, lightDirection));
    vec3 phongSpecularTerm = Ks * pow(max(0.0, dot(reflectionDirection, viewDirection)), q);
    
    vec4 outputColor = vec4(0.0);
    outputColor.rgb = (ambientTerm+lambertDiffuseTerm+phongSpecularTerm) * fragColor.rgb;
    outputColor.rgb = pow(outputColor.rgb, vec3(1.0,1.0,1.0)/2.2);
    outputColor.a = 1.0;
    return outputColor;
}

subroutine (fragmentShader) vec4 noFragmentShader() {
    return fragColor;
}

void main()
{
    switch(fragmentShaderType) {
      case NO_SHADER: {
        fColor = noFragmentShader();
        break;
      }
      case PHONG: {
        fColor = phongShader();
        break;
      }
      default: {
        fColor = noFragmentShader();
        break;
      }
    }
    
}
