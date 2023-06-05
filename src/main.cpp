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

struct TriangleVertex {
	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec3 color;
};

struct Triangle {
	TriangleVertex v0;
	TriangleVertex v1;
	TriangleVertex v2;
	glm::vec3 face_normal;
};

struct ModelObject {
	char name[256];
	int num_triangles;
	int material_count;
	glm::vec3 *ambient_color;
	glm::vec3 *diffuse_color;
	glm::vec3 *specular_color;
	float *material_shine;
	Triangle *triangles;
};

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
glm::vec2 g_CameraRotation;

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

GLuint BuildTriangles(ModelObject model);
ModelObject ReadModelFile(char *filename);

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

	char filename[] = "../cow_up.in";
	ModelObject model = ReadModelFile(filename);
	GLuint vertex_array_object_id = BuildTriangles(model);

	glm::mat4 viewProjectionMatrix;
	g_CameraRotation = glm::vec2(0.0f,0.0f);
	
	float camera_distance = 100.0f;

	glFrontFace(GL_CCW);
	//glEnable(GL_CULL_FACE);


    while (!glfwWindowShouldClose(window)) {

		glUseProgram(program_id);
        static const float background[] = { 1.0f, 1.0f, 1.0f, 0.0f };

        glClearBufferfv(GL_COLOR, 0, background);

		g_CameraRotation += glm::vec2(-0.01f,0.0f);
		camera_distance += 10.0f;
		viewProjectionMatrix = camera(camera_distance, g_CameraRotation);
		
		GLint viewProjectionMatrixLoc = glGetUniformLocation(program_id, "viewProjectionMatrix");

		glUniformMatrix4fv(viewProjectionMatrixLoc, 1, GL_FALSE, glm::value_ptr(viewProjectionMatrix));
		

        glBindVertexArray(vertex_array_object_id);

		glDrawElements(
			g_VirtualScene["cube"].rendering_mode,
			g_VirtualScene["cube"].num_indices,
			GL_UNSIGNED_INT,
			(void*)g_VirtualScene["cube"].first_index
		);
		
		

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);

    glfwTerminate();
	return 0;
}

ModelObject ReadModelFile(char *filename)
{
	ModelObject model;
	
	FILE *fp = fopen(filename,"r");
	if (!fp) {
		printf("ERROR: unable to open file [%s]!\n", filename);
		exit(0);
	}
	char objname[256] = { 0 };
	fscanf(fp, "Object name = %s\n", &model.name);
	fscanf(fp, "# triangles = %d\n", &model.num_triangles);
	fscanf(fp, "Material count = %d\n", &model.material_count);
	
	model.ambient_color = (glm::vec3*)calloc(model.material_count, sizeof(glm::vec3));
	model.diffuse_color = (glm::vec3*)calloc(model.material_count, sizeof(glm::vec3));
	model.specular_color = (glm::vec3*)calloc(model.material_count, sizeof(glm::vec3));
	model.material_shine = (float*)calloc(model.material_count, sizeof(float));
	
	for (int i = 0; i < model.material_count; i++) {
		fscanf(fp, "ambient color %f %f %f\n", &model.ambient_color[i].x,
											   &model.ambient_color[i].y,
											   &model.ambient_color[i].z);
		fscanf(fp, "diffuse color %f %f %f\n", &model.diffuse_color[i].x,
											   &model.diffuse_color[i].y,
											   &model.diffuse_color[i].z);
		fscanf(fp, "specular color %f %f %f\n", &model.specular_color[i].x,
												&model.specular_color[i].y,
												&model.specular_color[i].z);
		fscanf(fp, "material shine %f\n", &model.material_shine[i]);
	}
	char ch;
	while (ch != '\n') {
		fscanf(fp, "%c", &ch);
	}
	printf("Reading in %s (%d triangles)...\n", filename, model.num_triangles);
	
	model.triangles = (Triangle*)calloc(model.num_triangles, sizeof(Triangle));
	
	for (int i = 0; i < model.num_triangles; i++) {
		int color_index;
		fscanf(fp, "v0 %f %f %f %f %f %f %d\n",
				&model.triangles[i].v0.pos.x,
				&model.triangles[i].v0.pos.y,
				&model.triangles[i].v0.pos.z,
				&model.triangles[i].v0.normal.x,
				&model.triangles[i].v0.normal.y,
				&model.triangles[i].v0.normal.z,
				&color_index);
		model.triangles[i].v0.color.x =
			(unsigned char)(int)(255*(model.diffuse_color[color_index].x));
		model.triangles[i].v0.color.y =
			(unsigned char)(int)(255*(model.diffuse_color[color_index].y));
		model.triangles[i].v0.color.z =
			(unsigned char)(int)(255*(model.diffuse_color[color_index].z));

		fscanf(fp, "v1 %f %f %f %f %f %f %d\n",
				&model.triangles[i].v1.pos.x,
				&model.triangles[i].v1.pos.y,
				&model.triangles[i].v1.pos.z,
				&model.triangles[i].v1.normal.x,
				&model.triangles[i].v1.normal.y,
				&model.triangles[i].v1.normal.z,
				&color_index);
		model.triangles[i].v1.color.x =
			(unsigned char)(int)(255*(model.diffuse_color[color_index].x));
		model.triangles[i].v1.color.y =
			(unsigned char)(int)(255*(model.diffuse_color[color_index].y));
		model.triangles[i].v1.color.z =
			(unsigned char)(int)(255*(model.diffuse_color[color_index].z));
				
		fscanf(fp, "v2 %f %f %f %f %f %f %d\n",
				&model.triangles[i].v2.pos.x,
				&model.triangles[i].v2.pos.y,
				&model.triangles[i].v2.pos.z,
				&model.triangles[i].v2.normal.x,
				&model.triangles[i].v2.normal.y,
				&model.triangles[i].v2.normal.z,
				&color_index);
		model.triangles[i].v2.color.x =
			(unsigned char)(int)(255*(model.diffuse_color[color_index].x));
		model.triangles[i].v2.color.y =
			(unsigned char)(int)(255*(model.diffuse_color[color_index].y));
		model.triangles[i].v2.color.z =
			(unsigned char)(int)(255*(model.diffuse_color[color_index].z));	
				
		fscanf(fp, "face normal %f %f %f\n",
				&model.triangles[i].face_normal.x,
				&model.triangles[i].face_normal.y,
				&model.triangles[i].face_normal.z);
	}

	fclose(fp);
	return model;
}

GLuint BuildTriangles(ModelObject model)
{
	GLfloat model_coefficients[model.num_triangles * 3 * 4];	
	int j = 0;
	for (int i = 0; i < model.num_triangles; i++) {
		Triangle triangle = model.triangles[i];
		// v0
		// X
		model_coefficients[j]   = triangle.v0.pos.x;
		// Y
		model_coefficients[j+1] = triangle.v0.pos.y;
		// Z
		model_coefficients[j+2] = triangle.v0.pos.z;
		// W
		model_coefficients[j+3] = 1.0f;
		j+=4;
		// v1
		// X
		model_coefficients[j]   = triangle.v1.pos.x;
		// Y
		model_coefficients[j+1] = triangle.v1.pos.y;
		// Z
		model_coefficients[j+2] = triangle.v1.pos.z;
		// W
		model_coefficients[j+3] = 1.0f;
		j+=4;
		// v2
		// X
		model_coefficients[j]   = triangle.v2.pos.x;
		// Y
		model_coefficients[j+1] = triangle.v2.pos.y;
		// Z
		model_coefficients[j+2] = triangle.v2.pos.z;
		// W
		model_coefficients[j+3] = 1.0f;
		j+=4;
	}	
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
    // Cores dos vértices do cubo
    //  R     G     B     A
        1.0f, 0.5f, 0.0f, 1.0f, // cor do vértice 0
        1.0f, 0.5f, 0.0f, 1.0f, // cor do vértice 1
        0.0f, 0.5f, 1.0f, 1.0f, // cor do vértice 2
        0.0f, 0.5f, 1.0f, 1.0f, // cor do vértice 3
        1.0f, 0.5f, 0.0f, 1.0f, // cor do vértice 4
        1.0f, 0.5f, 0.0f, 1.0f, // cor do vértice 5
        0.0f, 0.5f, 1.0f, 1.0f, // cor do vértice 6
        0.0f, 0.5f, 1.0f, 1.0f, // cor do vértice 7
        1.0f, 0.5f, 0.0f, 1.0f, // cor do vértice 0
        1.0f, 0.5f, 0.0f, 1.0f, // cor do vértice 1
        0.0f, 0.5f, 1.0f, 1.0f, // cor do vértice 2
        0.0f, 0.5f, 1.0f, 1.0f, // cor do vértice 3
        1.0f, 0.5f, 0.0f, 1.0f, // cor do vértice 4
        1.0f, 0.5f, 0.0f, 1.0f, // cor do vértice 5
        0.0f, 0.5f, 1.0f, 1.0f, // cor do vértice 6
        0.0f, 0.5f, 1.0f, 1.0f, // cor do vértice 7
        1.0f, 0.5f, 0.0f, 1.0f, // cor do vértice 0
        1.0f, 0.5f, 0.0f, 1.0f, // cor do vértice 1
        0.0f, 0.5f, 1.0f, 1.0f, // cor do vértice 2
        0.0f, 0.5f, 1.0f, 1.0f, // cor do vértice 3
        1.0f, 0.5f, 0.0f, 1.0f, // cor do vértice 4
        1.0f, 0.5f, 0.0f, 1.0f, // cor do vértice 5
        0.0f, 0.5f, 1.0f, 1.0f, // cor do vértice 6
        0.0f, 0.5f, 1.0f, 1.0f, // cor do vértice 7
        1.0f, 0.5f, 0.0f, 1.0f, // cor do vértice 0
        1.0f, 0.5f, 0.0f, 1.0f, // cor do vértice 1
        0.0f, 0.5f, 1.0f, 1.0f, // cor do vértice 2
        0.0f, 0.5f, 1.0f, 1.0f, // cor do vértice 3
        1.0f, 0.5f, 0.0f, 1.0f, // cor do vértice 4
        1.0f, 0.5f, 0.0f, 1.0f, // cor do vértice 5
        0.0f, 0.5f, 1.0f, 1.0f, // cor do vértice 6
        0.0f, 0.5f, 1.0f, 1.0f, // cor do vértice 7
        1.0f, 0.5f, 0.0f, 1.0f, // cor do vértice 0
        1.0f, 0.5f, 0.0f, 1.0f, // cor do vértice 1
        0.0f, 0.5f, 1.0f, 1.0f, // cor do vértice 2
        0.0f, 0.5f, 1.0f, 1.0f, // cor do vértice 3
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
	
	
	GLuint indices[model.num_triangles * 3];
	for (int i = 0; i < model.num_triangles * 3; i++) {
		indices[i] = i;
	}

    SceneObject cube_faces;
    cube_faces.name           = "cube";
    cube_faces.first_index    = (void*)0; // Primeiro índice está em indices[0]
    cube_faces.num_indices    =  model.num_triangles * 3;       // Último índice está em indices[35]; total de 36 índices.
    cube_faces.rendering_mode = GL_TRIANGLES; // Índices correspondem ao tipo de rasterização GL_TRIANGLES.
	g_VirtualScene["cube"] = cube_faces;
	
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
