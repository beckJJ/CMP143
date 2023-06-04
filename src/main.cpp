#include <GL3/gl3.h>
#include <GL3/gl3w.h>
#include <GLFW/glfw3.h>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <fstream>
#include <sstream>
#include <map>

#include <math.h>

#include <Windows.h>
#include <sdkddkver.h>
#include <commdlg.h>

#ifndef _WIN32_WINNT
#define _WIN32_WINNT _NT_TARGET_VERSION_VISTA
#endif

#ifndef WINVER
#define WINVER _NT_TARGET_VERSION_VISTA
#endif


// Windows procedures
#define PROC_INFO_MENU 1
#define PROC_EXIT_MENU 2



struct SceneObject {
    const char*  name;        
    void*        first_index; 
    int          num_indices; 
    GLenum       rendering_mode;
};

// global variables
bool g_LeftMouseButtonPressed = false;
float g_ScreenRatio;
double g_LastCursorPosX, g_LastCursorPosY;
std::map<const char*, SceneObject> g_VirtualScene;

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

GLuint BuildTriangles();

// windows.h functions
LRESULT CALLBACK WindowProcedure(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);
void AddMenus(HWND hWnd, HMENU hMenu);

// camera functions
glm::mat4 camera(float Translate, glm::vec2 const & Rotate);


int main( int argc, char** argv )
{
	// initialize win32 window
	WNDCLASSW wc = { 0 }; // define window class
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProcedure;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = (HINSTANCE)GetModuleHandle(NULL);
    wc.hIcon = NULL;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
    wc.lpszMenuName = NULL;
    wc.lpszClassName = L"WindowClass";
	
	// register class
	if (!RegisterClassW(&wc)) {
		return 0;
	}
	
	// create main window
	HWND hWnd = CreateWindowW(L"WindowClass", L"CMP143",
						WS_OVERLAPPEDWINDOW | WS_VISIBLE,
						100, 100,
						260, 420,
						NULL, NULL, NULL, NULL);				
	if (!hWnd) {
		return 0;
	}
	
	// create menu and controls
	HMENU hMenu = { 0 };
	AddMenus(hWnd, hMenu);
	//AddControls(hWnd);
	MSG msg = { 0 };
	UpdateWindow(hWnd);


	// initialize openGL
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
	
    glfwMakeContextCurrent(window);
	
	// load shaders
    gl3wInit();
	GLuint vertex_shader_id = LoadShader_Vertex("../triangles.vert");
	GLuint fragment_shader_id = LoadShader_Fragment("../triangles.frag");
	GLuint program_id = CreateGpuProgram(vertex_shader_id, fragment_shader_id);


	GLuint vertex_array_object_id = BuildTriangles();



    while (!glfwWindowShouldClose(window)) {
		
		glm::mat4 viewMatrix;
		glm::mat4 projectionMatrix;

		// Set up camera parameters
		glm::vec3 cameraPosition = glm::vec3(0.0f, 0.0f, 3.0f);
		glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
		glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

		// Create the view matrix
		viewMatrix = glm::lookAt(cameraPosition, cameraTarget, cameraUp);
		float FOV = 45.0f;
		float aspectRatio = 800.0f / 600.0f;
		float nearPlane = 0.1f;
		float farPlane = 100.0f;
		projectionMatrix = glm::perspective(glm::radians(FOV), aspectRatio, nearPlane, farPlane);
		glm::mat4 viewProjectionMatrix = projectionMatrix * viewMatrix;
		
		GLint viewProjectionMatrixLoc = glGetUniformLocation(program_id, "viewProjectionMatrix");
		glUseProgram(program_id);
		glUniformMatrix4fv(viewProjectionMatrixLoc, 1, GL_FALSE, glm::value_ptr(viewProjectionMatrix));
		
        static const float background[] = { 1.0f, 1.0f, 1.0f, 0.0f };

        glClearBufferfv(GL_COLOR, 0, background);

        glBindVertexArray(vertex_array_object_id);
		
		glDrawElements(
			g_VirtualScene["triangulo"].rendering_mode,
			g_VirtualScene["triangulo"].num_indices,
			GL_UNSIGNED_INT,
			(void*)g_VirtualScene["triangulo"].first_index
		);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);

    glfwTerminate();
	return 0;
}

GLuint BuildTriangles()
{
	GLfloat  model_coefficients[] = {
	//     X       Y        Z      W
		-0.90f, -0.90f ,  0.0f , 1.0f,
		 0.85f, -0.90f ,  0.0f , 1.0f,
		-0.90f,  0.85f ,  0.0f , 1.0f // Triangle 1
    };
	GLuint VBO_model_coefficients_id;
	glGenBuffers(1, &VBO_model_coefficients_id);
	

	GLuint vertex_array_object_id;
	glGenVertexArrays(1, &vertex_array_object_id);
	glBindVertexArray(vertex_array_object_id);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_model_coefficients_id);
	glBufferData(GL_ARRAY_BUFFER, sizeof(model_coefficients), NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(model_coefficients), model_coefficients);
	
	GLuint location = 0;
	GLint number_of_dimensions = 4;
	glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(location);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	
    GLfloat color_coefficients[] = {
    //  R     G     B     A
        1.0f, 0.0f, 0.0f, 1.0f,
        0.0f, 1.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f, 1.0f
    };
    GLuint VBO_color_coefficients_id;
    glGenBuffers(1, &VBO_color_coefficients_id);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_color_coefficients_id);
    glBufferData(GL_ARRAY_BUFFER, sizeof(color_coefficients), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(color_coefficients), color_coefficients);
    location = 1;
    number_of_dimensions = 4;
    glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(location);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
	
	
	GLuint indices[] = {
		0, 1, 2 // triângulo 1
	};
	
	SceneObject triangulo;
	triangulo.name = "Triangulo";
	triangulo.first_index = (void*)0;
	triangulo.num_indices = 3;
	triangulo.rendering_mode = GL_TRIANGLES;
	
	g_VirtualScene["triangulo"] = triangulo;
	
	GLuint indices_id;
	glGenBuffers(1, &indices_id);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(indices), indices);

    glBindVertexArray(0);
    return vertex_array_object_id;
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

LRESULT CALLBACK WindowProcedure(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	int val;
	switch (msg) {
	  case WM_COMMAND:
		switch (wp) {
		  case PROC_INFO_MENU:
			MessageBox(NULL, "CMP143", "About", MB_OK);
			break;
		  case PROC_EXIT_MENU:
		    val = MessageBoxW(NULL, L"Exit?", L"", MB_YESNO | MB_ICONEXCLAMATION);
			if (val == IDYES) {
				exit(0);
			}
			break;
		}
		break;
	  case WM_DESTROY:
		exit(0);
		break;
	  default:
	    return DefWindowProcW(hWnd, msg, wp, lp);
	}
}

void AddMenus(HWND hWnd, HMENU hMenu)
{
	hMenu = CreateMenu();
	HMENU hFileMenu = CreateMenu();
	
	LPCSTR exit = "Exit";
	LPCSTR file = "File";
	LPCSTR info = "About";
	
	AppendMenu(hFileMenu, MF_STRING, PROC_EXIT_MENU, exit);
	
	AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hFileMenu, file);
	AppendMenu(hMenu, MF_STRING, PROC_INFO_MENU, info);
	
	SetMenu(hWnd, hMenu);
}

glm::mat4 camera(float Translate, glm::vec2 const & Rotate)
{
	glm::mat4 Projection = glm::perspective(glm::radians(45.0f), 4.0f / 3.0f, 0.1f, 100.f);
	glm::mat4 View = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -Translate));
	View = glm::rotate(View, Rotate.y, glm::vec3(-1.0f, 0.0f, 0.0f));
	View = glm::rotate(View, Rotate.x, glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 Model = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f));
	
	return Projection * View * Model;
}
