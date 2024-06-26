#version 450 core

in vec4 fragColor;
in vec4 worldPosition;
in vec4 modelPosition;
in vec4 fragNormal;
in vec2 texCoords;

uniform int fragmentShaderType;
uniform bool useClose2GL;
uniform bool useTexture;
#define NO_SHADER 0
#define PHONG     1


uniform sampler2D textureC2GL;
uniform sampler2D textureSampler;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;
uniform vec4 LightDir;

out vec4 fColor;

subroutine vec4 fragmentShader(vec4 baseColor);

subroutine (fragmentShader) vec4 phongShader(vec4 baseColor) {
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
    outputColor.rgb = (ambientTerm+lambertDiffuseTerm+phongSpecularTerm) * baseColor.rgb;
    outputColor.rgb = pow(outputColor.rgb, vec3(1.0,1.0,1.0)/2.2);
    outputColor.a = 1.0;
    return outputColor;
}

subroutine (fragmentShader) vec4 noFragmentShader(vec4 baseColor) {
    return baseColor;
}

void main()
{
    if (useClose2GL) {
        fColor = texture(textureC2GL, texCoords);
    } else {
        vec4 baseColor;
        if (useTexture) {
            baseColor = texture(textureSampler, texCoords);
        } else {
            baseColor = fragColor;
        }
        switch(fragmentShaderType) {
          case NO_SHADER: {
            fColor = noFragmentShader(baseColor);
            break;
          }
          case PHONG: {
            fColor = phongShader(baseColor);
            break;
          }
          default: {
            fColor = noFragmentShader(baseColor);
            break;
          }
        }
    }
}
