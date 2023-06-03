#include <GL3/gl3.h>
#include <GL3/gl3w.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <fstream>
#include <sstream>

#include <Windows.h>


// global variables
bool g_LeftMouseButtonPressed = false;
float g_ScreenRatio;
double g_LastCursorPosX, g_LastCursorPosY;

// callback functions
void ErrorCallback(int error, const char *description);
void KeyCallback(GLFWwindow *window,
                                   int key, int scancode, int action, int mod);
void MouseButtonCallback(GLFWwindow *window, int button, int action, int mods);
void CursorPosCallback(GLFWwindow *window, double xpos, double ypos);
void ScrollCallback(GLFWwindow *window, double xoffset, double yoffset);
void FramebufferSizeCallback(GLFWwindow *window, int width, int height);

// shader functions
void LoadShader(const char *filename, GLuint shader_id);
GLuint LoadShader_Vertex(const char *filename);
GLuint LoadShader_Fragment(const char *filename);
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id);

int main(int argc, char **argv)
{
	int success = glfwInit();
	if (!success) {
		fprintf(stderr, "ERROR: glfwInit() failed.\n");
		std::exit(EXIT_FAILURE);
	}
	glfwSetErrorCallback(ErrorCallback);
	
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow *window = glfwCreateWindow(800, 600, "CMP143", NULL, NULL);
	if (!window) {
        glfwTerminate();
        fprintf(stderr, "ERROR: glfwCreateWindow() failed.\n");
        std::exit(EXIT_FAILURE);
	}
	
	glfwSetKeyCallback(window, KeyCallback);
	glfwSetMouseButtonCallback(window, MouseButtonCallback);
	glfwSetCursorPosCallback(window, CursorPosCallback);
	glfwSetScrollCallback(window, ScrollCallback);
    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
    
	glfwSetWindowSize(window, 800, 600);
	
	glfwMakeContextCurrent(window);

	// load shaders
	gl3wInit();
	GLuint vertex_shader_id = LoadShader_Vertex("../triangles.vert");
	GLuint fragment_shader_id = LoadShader_Fragment("../triangles.frag");	
	
	GLuint program_id = CreateGpuProgram(vertex_shader_id, fragment_shader_id);
	glUseProgram(program_id);
	
	while (!glfwWindowShouldClose(window)) {
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}

void ErrorCallback(int error, const char *description)
{
	fprintf(stderr, "ERROR: GLFW %s\n", description);
}

void KeyCallback(GLFWwindow *window,
                                    int key, int scancode, int action, int mod)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GL_TRUE);
	}
}

void MouseButtonCallback(GLFWwindow *window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_LeftMouseButtonPressed = true;
    }
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
        g_LeftMouseButtonPressed = false;
    }
}

void CursorPosCallback(GLFWwindow *window, double xpos, double ypos)
{
	if (!g_LeftMouseButtonPressed) {
		return;
	}
}

void ScrollCallback(GLFWwindow *window, double xoffset, double yoffset)
{
	return;
}

void FramebufferSizeCallback(GLFWwindow *window, int width, int height)
{
	glViewport(0, 0, width, height);
    g_ScreenRatio = (float)width / height;
}

void LoadShader(const char *filename, GLuint shader_id)
{
	std::ifstream file;
    try {
        file.exceptions(std::ifstream::failbit);
        file.open(filename);
    } catch ( std::exception& e ) {
        fprintf(stderr, "ERROR: Cannot open file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }
    std::stringstream shader;
    shader << file.rdbuf();
    std::string str = shader.str();
    const GLchar *shader_string = str.c_str();
    const GLint   shader_string_length = static_cast<GLint>( str.length() );

    glShaderSource(shader_id, 1, &shader_string, &shader_string_length);
    glCompileShader(shader_id);

    GLint compiled_ok;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compiled_ok);

    GLint log_length = 0;
    glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length);
    GLchar *log = new GLchar[log_length];
    glGetShaderInfoLog(shader_id, log_length, &log_length, log);
	if (log_length != 0) {
        std::string  output;

        if (!compiled_ok) {
            output += "ERROR: OpenGL compilation of \"";
            output += filename;
            output += "\" failed.\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        } else {
            output += "WARNING: OpenGL compilation of \"";
            output += filename;
            output += "\".\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }
        fprintf(stderr, "%s", output.c_str());
    }
    delete [] log;
}

GLuint LoadShader_Fragment(const char *filename)
{
	GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);
    LoadShader(filename, fragment_shader_id);
    return fragment_shader_id;
}

GLuint LoadShader_Vertex(const char *filename)
{
	GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
	LoadShader(filename, vertex_shader_id);
	return vertex_shader_id;
}

GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id)
{
	GLuint program_id = glCreateProgram();
	
	glAttachShader(program_id, vertex_shader_id);
	glAttachShader(program_id, fragment_shader_id);
	glLinkProgram(program_id);
	
	GLint linked_ok = GL_FALSE;
	glGetProgramiv(program_id, GL_LINK_STATUS, &linked_ok);
	
	if (linked_ok == GL_FALSE) {
		GLint log_length = 0;
		glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &log_length);
		GLchar *log = new GLchar[log_length];
        glGetProgramInfoLog(program_id, log_length, &log_length, log);
        std::string output;
        output += "ERROR: OpenGL linking of program failed.\n";
        output += "== Start of link log\n";
        output += log;
        output += "\n== End of link log\n";
        delete[] log;
        fprintf(stderr, "%s", output.c_str());
	}
	return program_id;
}