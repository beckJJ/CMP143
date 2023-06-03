Esse é o código de exemplo da disciplina INF01009 / CMP143 - Computação Gráfica. Ele foi baseado fortemente no repositório do OpenGL Red Book Examples (https://github.com/openglredbook/examples).

A maneira mais fácil de compilar esse código é utilizando CMake. O diretório fonte deve ser a pasta CMP143 (que contém o arquivo CMakeLists.txt) e a pasta de build pode ser ("PathAtéAPasta"/CMP143/build). O CMake vai gerar uma maneira de compilar o código em si, dependendo do gerador que vocês selecionarem. Por exemplo: se escolherem o Visual Studio (e recomendo fortemente usarem, se estiverem no Windows) o CMake vai gerar um .sln, se escolherem MinGW ele gerará um Makefile, e por aí vai. No Linux, o gerador padrão é o Make, que também faz o CMake gerar Makefiles.

Depois disso o projeto deve estar pronto para ser compilado na pasta que vocês selecionaram ("/build"). Caso estejam usando o Visual Studio, não se esqueçam de mudar o projeto de startup da solução para CMP143.

Obs: Caso você estiver usando um Mac você provavelmente terá problemas rodando esse código. A Apple descontinuou o OpenGL desde a versão 4.1, então qualquer função mais recente do que isso não funcionará.