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
#include <float.h>

#include "matrices.h"

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
#define PROC_OPEN_FILE 3
#define PROC_TOGGLE_CW 4
#define PROC_TOGGLE_CCW 4

struct TriangleVertex {
	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec3 color;
};

struct Triangle {
	TriangleVertex v0;
	TriangleVertex v1;
	TriangleVertex v2;
	glm::vec3      face_normal;
};

struct ModelObject {
	char       name[256];
	int        num_triangles;
	int        material_count;
	glm::vec3 *ambient_color;
	glm::vec3 *diffuse_color;
	glm::vec3 *specular_color;
	float     *material_shine;
	Triangle  *triangles;
};

struct SceneObject {
    const char *name;        
    void       *first_index; 
    int         num_indices; 
    GLenum      rendering_mode;
	glm::vec3   min_coord;
	glm::vec3   max_coord;
};

// global variables
bool g_LeftMouseButtonPressed = false;
float g_ScreenRatio;
double g_LastCursorPosX, g_LastCursorPosY;
std::map<const char*, SceneObject> g_VirtualScene;
float g_CameraTheta = 0.0f;
float g_CameraPhi = 0.0f;
float g_CameraDistance = 2.5f;
bool g_ResetCamera = false;
bool g_W_pressed = false;
bool g_A_pressed = false;
bool g_S_pressed = false;
bool g_D_pressed = false;

char g_ModelFilename[FILENAME_MAX];
ModelObject g_Model;
GLuint g_vertex_array_object_id;
HWND w_toggleCW = NULL;
HWND w_toggleCCW = NULL;

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
void AddControls(HWND hWnd);
int openFile(HWND hWnd);

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
	AddControls(hWnd);
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
	glfwSetWindowSize(window, 800, 600);
	g_ScreenRatio = 800.0f/600.0f;
	
    glfwMakeContextCurrent(window);
	
	// load shaders
    gl3wInit();
	GLuint vertex_shader_id = LoadShader_Vertex("../triangles.vert");
	GLuint fragment_shader_id = LoadShader_Fragment("../triangles.frag");
	GLuint program_id = CreateGpuProgram(vertex_shader_id, fragment_shader_id);

	char filename[] = "../cube.in";
	ModelObject model = ReadModelFile(filename);
	GLuint vertex_array_object_id = BuildTriangles(model);

    GLint modelMatrixLocation = glGetUniformLocation(program_id, "modelMatrix");
    GLint viewMatrixLocation = glGetUniformLocation(program_id, "viewMatrix");
    GLint projectionMatrixLocation = glGetUniformLocation(program_id, "projectionMatrix");

	g_vertex_array_object_id = -1;

	glFrontFace(GL_CW);
	glEnable(GL_CULL_FACE);

    while (!glfwWindowShouldClose(window)) {

		glUseProgram(program_id);
        static const float background[] = { 1.0f, 1.0f, 1.0f, 0.0f };

        glClearBufferfv(GL_COLOR, 0, background);

        float r = g_CameraDistance;
        float y = r*sin(g_CameraPhi);
        float z = r*cos(g_CameraPhi)*cos(g_CameraTheta);
        float x = r*cos(g_CameraPhi)*sin(g_CameraTheta);

		static int ini = 0;
        glm::vec4 camera_position_c;
        if (!ini || g_ResetCamera) {
            camera_position_c = glm::vec4(x,y,z,1.0f);
            ini = 1;
			g_ResetCamera = false;
        }
		
		glm::vec4 cameraTarget = glm::vec4(0.0f,0.0f,0.0f,1.0f);
		glm::vec4 cameraView = cameraTarget - glm::vec4(x,y,z,1.0f);
		glm::vec4 cameraUp = glm::vec4(0.0f,1.0f,0.0f,0.0f);
		
        glm::vec4 camera_w = -cameraView/norm(cameraView);
        glm::vec4 camera_u = crossproduct(cameraUp, camera_w);
        glm::vec4 camera_v = crossproduct(camera_w, camera_u);
		
        if (g_W_pressed) {
            camera_position_c += -camera_w * 0.05f;
        }

        if (g_S_pressed) {
            camera_position_c += camera_w * 0.05f;
        }

        if (g_D_pressed) {
            camera_position_c += camera_u * 0.05f;
        }

        if (g_A_pressed) {
            camera_position_c += -camera_u * 0.05f;
        }
		
		glm::mat4 viewMatrix = Matrix_Camera_View(camera_position_c, cameraView, cameraUp);		
		
		float FOV = 3.141592 / 4;
		float aspectRatio = g_ScreenRatio;
		
		float nearPlane = -0.1f;
		float farPlane = -100.0f;
		
		glm::mat4 projectionMatrix = Matrix_Perspective(FOV, g_ScreenRatio, nearPlane, farPlane);
		
		glUniformMatrix4fv(viewMatrixLocation, 1 , GL_FALSE , glm::value_ptr(viewMatrix));
		glUniformMatrix4fv(projectionMatrixLocation, 1 , GL_FALSE , glm::value_ptr(projectionMatrix));

		if (g_vertex_array_object_id != -1) {
			
			float trans_x = (g_VirtualScene["model"].min_coord.x + g_VirtualScene["model"].max_coord.x) / 2;
			float trans_y = (g_VirtualScene["model"].min_coord.y + g_VirtualScene["model"].max_coord.y) / 2;
			float trans_z = (g_VirtualScene["model"].min_coord.z + g_VirtualScene["model"].max_coord.z) / 2;
			float size_x = (g_VirtualScene["model"].max_coord.x - g_VirtualScene["model"].min_coord.x);
			float size_y = (g_VirtualScene["model"].max_coord.y - g_VirtualScene["model"].min_coord.y);
			float size_z = (g_VirtualScene["model"].max_coord.z - g_VirtualScene["model"].min_coord.z);
			float scaling_factor = size_x;
			scaling_factor = (size_y > scaling_factor) ? size_y : scaling_factor;
			scaling_factor = (size_z > scaling_factor) ? size_z : scaling_factor;
			glBindVertexArray(g_vertex_array_object_id);
			
			glm::vec3 objectTranslate = glm::vec3(-trans_x, -trans_y, -trans_z);
			glm::vec3 objectScale = glm::vec3(2.0f / scaling_factor, 2.0f / scaling_factor, 2.0f / scaling_factor);
			glm::mat4 modelMatrix = glm::mat4(1.0f);
			modelMatrix = glm::scale(modelMatrix, objectScale);
			modelMatrix = glm::translate(modelMatrix, objectTranslate);

			glDrawElements(
				g_VirtualScene["model"].rendering_mode,
				g_VirtualScene["model"].num_indices,
				GL_UNSIGNED_INT,
				(void*)g_VirtualScene["model"].first_index
			);		
			glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, glm::value_ptr(modelMatrix));
		}

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
	glm::vec3 min_coord = glm::vec3(FLT_MAX, FLT_MAX, FLT_MAX);
	glm::vec3 max_coord = glm::vec3(FLT_MIN, FLT_MIN, FLT_MIN);
	
	GLfloat model_coefficients[model.num_triangles * 3 * 4];	
	int j = 0;
	for (int i = 0; i < model.num_triangles; i++) {
		Triangle triangle = model.triangles[i];
		// v0
		// X
		model_coefficients[j]   = triangle.v0.pos.x;
		min_coord.x = (model_coefficients[j] < min_coord.x) ? model_coefficients[j] : min_coord.x;
		max_coord.x = (model_coefficients[j] > max_coord.x) ? model_coefficients[j] : max_coord.x;
		// Y
		model_coefficients[j+1] = triangle.v0.pos.y;
		min_coord.y = (model_coefficients[j+1] < min_coord.y) ? model_coefficients[j+1] : min_coord.y;
		max_coord.y = (model_coefficients[j+1] > max_coord.y) ? model_coefficients[j+1] : max_coord.y;
		// Z
		model_coefficients[j+2] = triangle.v0.pos.z;
		min_coord.z = (model_coefficients[j+2] < min_coord.z) ? model_coefficients[j+2] : min_coord.z;
		max_coord.z = (model_coefficients[j+2] > max_coord.z) ? model_coefficients[j+2] : max_coord.z;
		// W
		model_coefficients[j+3] = 1.0f;
		j+=4;
		// v1
		// X
		model_coefficients[j]   = triangle.v1.pos.x;
		min_coord.x = (model_coefficients[j] < min_coord.x) ? model_coefficients[j] : min_coord.x;
		max_coord.x = (model_coefficients[j] > max_coord.x) ? model_coefficients[j] : max_coord.x;
		// Y
		model_coefficients[j+1] = triangle.v1.pos.y;
		min_coord.y = (model_coefficients[j+1] < min_coord.y) ? model_coefficients[j+1] : min_coord.y;
		max_coord.y = (model_coefficients[j+1] > max_coord.y) ? model_coefficients[j+1] : max_coord.y;
		// Z
		model_coefficients[j+2] = triangle.v1.pos.z;
		min_coord.z = (model_coefficients[j+2] < min_coord.z) ? model_coefficients[j+2] : min_coord.z;
		max_coord.z = (model_coefficients[j+2] > max_coord.z) ? model_coefficients[j+2] : max_coord.z;
		// W
		model_coefficients[j+3] = 1.0f;
		j+=4;
		// v2
		// X
		model_coefficients[j]   = triangle.v2.pos.x;
		min_coord.x = (model_coefficients[j] < min_coord.x) ? model_coefficients[j] : min_coord.x;
		max_coord.x = (model_coefficients[j] > max_coord.x) ? model_coefficients[j] : max_coord.x;
		// Y
		model_coefficients[j+1] = triangle.v2.pos.y;
		min_coord.y = (model_coefficients[j+1] < min_coord.y) ? model_coefficients[j+1] : min_coord.y;
		max_coord.y = (model_coefficients[j+1] > max_coord.y) ? model_coefficients[j+1] : max_coord.y;
		// Z
		model_coefficients[j+2] = triangle.v2.pos.z;
		min_coord.z = (model_coefficients[j+2] < min_coord.z) ? model_coefficients[j+2] : min_coord.z;
		max_coord.z = (model_coefficients[j+2] > max_coord.z) ? model_coefficients[j+2] : max_coord.z;
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

    SceneObject sceneModel;
    sceneModel.name           = "model";
    sceneModel.first_index    = (void*)0; // Primeiro índice está em indices[0]
    sceneModel.num_indices    =  model.num_triangles * 3;       // Último índice está em indices[35]; total de 36 índices.
    sceneModel.rendering_mode = GL_TRIANGLES; // Índices correspondem ao tipo de rasterização GL_TRIANGLES.
	sceneModel.min_coord      = min_coord;
	sceneModel.max_coord      = max_coord;
	g_VirtualScene["model"] = sceneModel;
	
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

void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mod)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GL_TRUE);
	}
	if (key == GLFW_KEY_W) {
        if (action == GLFW_PRESS) {
            g_W_pressed = true;
        }
        else if (action == GLFW_RELEASE) {
            g_W_pressed = false;
        }
        else if (action == GLFW_REPEAT) {
        }
    }

    if (key == GLFW_KEY_A) {
        if (action == GLFW_PRESS) {
            g_A_pressed = true;
        }
        else if (action == GLFW_RELEASE) {
            g_A_pressed = false;
        }
        else if (action == GLFW_REPEAT) {
        }
    }

    if (key == GLFW_KEY_S) {
        if (action == GLFW_PRESS) {
            g_S_pressed = true;
        }
        else if (action == GLFW_RELEASE) {
            g_S_pressed = false;
        }
        else if (action == GLFW_REPEAT) {
        }
    }

    if (key == GLFW_KEY_D) {
        if (action == GLFW_PRESS) {
            g_D_pressed = true;
        }
        else if (action == GLFW_RELEASE) {
            g_D_pressed = false;
        }
        else if (action == GLFW_REPEAT) {
        }
    }
	
	if (key == GLFW_KEY_R) {
        if (action == GLFW_PRESS) {
			g_CameraTheta = 0.0f;
			g_CameraPhi = 0.0f;
			g_CameraDistance = 2.5f;
			g_ResetCamera = true;
		}
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
	// Deslocamento do cursor do mouse em x e y de coordenadas de tela!
    float dx = xpos - g_LastCursorPosX;
    float dy = ypos - g_LastCursorPosY;

    // Atualizamos parâmetros da câmera com os deslocamentos
    g_CameraTheta -= 0.01f*dx;
    g_CameraPhi   += 0.01f*dy;

    // Em coordenadas esféricas, o ângulo phi deve ficar entre -pi/2 e +pi/2.
    float phimax = 3.141592f/2;
    float phimin = -phimax;

    if (g_CameraPhi > phimax)
        g_CameraPhi = phimax;

    if (g_CameraPhi < phimin)
        g_CameraPhi = phimin;

    // Atualizamos as variáveis globais para armazenar a posição atual do
    // cursor como sendo a última posição conhecida do cursor.
    g_LastCursorPosX = xpos;
    g_LastCursorPosY = ypos;
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

int openFile(HWND hWnd)
{
    OPENFILENAME ofn;
    ZeroMemory(&ofn, sizeof(OPENFILENAME));

    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFile = g_ModelFilename;
    ofn.lpstrFile[0] = '\0';
    ofn.nMaxFile = FILENAME_MAX * 2;
    ofn.lpstrFilter = "Model Files (.in)\0*.IN\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_FILEMUSTEXIST;

    GetOpenFileName(&ofn);

    if (!g_ModelFilename[0]) {
        return 0;
    }
    
    FILE *file;
    if (!(file = fopen(g_ModelFilename, "r"))) {
        MessageBox(NULL, "File not found!", "ERROR", MB_OK);
        return -1;
    }
    fclose(file);

    return 1;
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
		  case PROC_OPEN_FILE:
		    if (openFile(hWnd)) {
				printf("opening file");
				FILE *file;
				if (!(file = fopen(g_ModelFilename, "r"))) {
					MessageBox(NULL, "Arquivo nao encontrado!", "ERRO", MB_OK);
					return -1;
				}
				g_Model = ReadModelFile(g_ModelFilename);
				g_vertex_array_object_id = BuildTriangles(g_Model);
			}
			break;
		  case PROC_TOGGLE_CW:
			int checkedState = SendMessageW(w_toggleCW, BM_GETCHECK, 0, 0);
			if (checkedState == BST_CHECKED) {
				glFrontFace(GL_CW);
			} else if (checkedState == BST_UNCHECKED) {
				glFrontFace(GL_CCW);
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

void AddControls(HWND hWnd)
{
    CreateWindowW(
        L"BUTTON",
        L"OPEN MODEL",      // Button text 
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        20,         // x position 
        20,         // y position 
        200,        // Button width
        25,        // Button height
        hWnd,     // Parent window
        (HMENU)PROC_OPEN_FILE, // procedure
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
        NULL);
	
	w_toggleCW = CreateWindowW(
		L"BUTTON",
		L"CW",
		WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
		20, 60,
		100, 25,
		hWnd,
		(HMENU)PROC_TOGGLE_CW,
		(HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
		NULL);
	w_toggleCCW = CreateWindowW(
		L"BUTTON",
		L"CCW",
		WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
		20, 90,
		100, 25,
		hWnd,
		(HMENU)PROC_TOGGLE_CCW,
		(HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
		NULL);
	
}