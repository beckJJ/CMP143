#version 450 core

in vec4 cor_interpolada_pelo_rasterizador;
out vec4 fColor;

void main()
{
    fColor = cor_interpolada_pelo_rasterizador;
}
