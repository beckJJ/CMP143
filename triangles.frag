#version 450 core

in vec4 fragColor;
in vec4 position_world;
in vec4 position_model;
in vec4 normal;


uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;
uniform vec4 LightDir;

out vec4 fColor;

void main()
{
    fColor = fragColor;
}
