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
#include <vector>

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
#define PROC_NEARPLANE 5
#define PROC_FARPLANE 6
#define PROC_RESET_CAMERA 7
#define PROC_CHANGE_COLOUR 8
#define PROC_LOOKAT_CAMERA 9
#define PROC_TOGGLE_GL 10
#define PROC_TOGGLE_SOLID 11
#define PROC_SET_HFOV 12
#define PROC_SET_VFOV 13
#define PROC_RESET_WINDOW 14
#define PROC_NO_SHADING 15
#define PROC_GOURAUD_AD 16
#define PROC_GOURAUD_ADS 17
#define PROC_PHONG 18

#define BUFFER_SIZE 100


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

struct ScreenPixel {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
    float         z;
};

struct ColorBuffer {
    int          width;
    int          height;
    ScreenPixel *pixels;
};

inline int get_index(ColorBuffer buffer, int i, int j)
{
    return (i + (j * buffer.width));
}

// global variables
std::map<const char*, SceneObject> g_VirtualScene;
double g_LastCursorPosX, g_LastCursorPosY;
float g_ScreenRatio;
float g_CameraTheta    = 0.0f;
float g_CameraPhi      = 0.0f;
float g_CameraDistance = 5.0f;
float g_NearPlane = -  0.1f;
float g_FarPlane  = -100.0f;
float g_Red   = 0.5f;
float g_Green = 0.5f;
float g_Blue  = 0.5f;
float g_hFov  = glm::radians(50.0f);
float g_vFov  = glm::radians(37.5f);
bool g_LeftMouseButtonPressed = false;
bool g_ResetCamera     = false;
bool g_W_pressed       = false;
bool g_A_pressed       = false;
bool g_S_pressed       = false;
bool g_D_pressed       = false;
bool g_Q_pressed       = false;
bool g_Z_pressed       = false;
bool g_LookAtCamera    = false;
bool g_UseClose2GL     = false;
bool g_TogglePoints    = false;
bool g_ToggleSolid     = true;
bool g_ToggleWireframe = false;
bool g_ToggleCW        = true; // true = CW, false = CCW
bool g_ToggleGouraud   = false; // gouraud shading
bool g_TogglePhong     = false; // phong light
int g_ScreenWidth  = 800;
int g_ScreenHeight = 600;

ColorBuffer g_ColorBuffer;

glm::mat4 g_ModelMatrix;
glm::mat4 g_ViewMatrix;
glm::mat4 g_ProjectionMatrix;

char g_ModelFilename[FILENAME_MAX];
ModelObject g_Model;

GLFWwindow *g_GLWindow;
GLuint g_VertexArrayObject_id;
GLint  g_UseClose2GLLocation;
GLuint g_Texture_id;

GLint g_VertexShaderTypeLocation;
GLint g_FragmentShaderTypeLocation;
int   g_VertexShaderType;
int   g_FragmentShaderType;

HWND w_ToggleCW         = NULL;
HWND w_ToggleCCW        = NULL;
HWND w_ToggleGL         = NULL;
HWND w_NearPlaneBox     = NULL;
HWND w_FarPlaneBox      = NULL;
HWND w_LookAtCheckbox   = NULL;
HWND w_RedBox           = NULL;
HWND w_GreenBox         = NULL;
HWND w_BlueBox          = NULL;
HWND w_TogglePoints     = NULL;
HWND w_ToggleWireframe  = NULL;
HWND w_ToggleSolid      = NULL;
HWND w_HorizontalFOV    = NULL;
HWND w_VerticalFOV      = NULL;
HWND w_ToggleNoShading  = NULL;
HWND w_ToggleGouraudAD  = NULL;
HWND w_ToggleGouraudADS = NULL;
HWND w_TogglePhong      = NULL;

// callback functions
void ErrorCallback(int error, const char *description);
void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mod);
void MouseButtonCallback(GLFWwindow *window, int button, int action, int mods);
void CursorPosCallback(GLFWwindow *window, double xpos, double ypos);
void FramebufferSizeCallback(GLFWwindow *window, int width, int height);

// shader functions
void   LoadTexture(unsigned char *textureData, int width, int height);
void   LoadShader(const char *filename, GLuint shader_id);
GLuint LoadShader_Vertex(const char *filename);
GLuint LoadShader_Fragment(const char *filename);
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id);

void DrawTriangle(glm::vec4 v1, glm::vec4 v2, glm::vec4 v3, glm::vec3 c1, glm::vec3 c2, glm::vec3 c3);
GLuint      BuildTriangles(ModelObject model);
ModelObject ReadModelFile(char *filename);

void ShowFramesPerSecond();

// windows.h functions
LRESULT CALLBACK WindowProcedure(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);
void    AddMenus(HWND hWnd, HMENU hMenu);
void    AddControls(HWND hWnd);
int     OpenFile(HWND hWnd);

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
                        580, 440,
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
    SendMessageW(w_LookAtCheckbox,  BM_SETCHECK, g_LookAtCamera, 0);
    SendMessageW(w_ToggleCW,        BM_SETCHECK, true,           0);
    SendMessageW(w_ToggleGL,        BM_SETCHECK, true,           0);
    SendMessageW(w_ToggleSolid,     BM_SETCHECK, true,           0);
    SendMessageW(w_ToggleNoShading, BM_SETCHECK, true,           0);

    // initialize openGL
    int success = glfwInit();
    if (!success) {
        fprintf(stderr, "ERROR: glfwInit() failed.\n");
        std::exit(EXIT_FAILURE);
    }
    glfwSetErrorCallback(ErrorCallback);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    g_GLWindow = glfwCreateWindow(800, 600, "CMP143", NULL, NULL);
    if (!g_GLWindow) {
        glfwTerminate();
        fprintf(stderr, "ERROR: glfwCreateWindow() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    glfwSetKeyCallback(g_GLWindow, KeyCallback);
    glfwSetMouseButtonCallback(g_GLWindow, MouseButtonCallback);
    glfwSetCursorPosCallback(g_GLWindow, CursorPosCallback);
    glfwSetFramebufferSizeCallback(g_GLWindow, FramebufferSizeCallback);
    glfwSetWindowSize(g_GLWindow, 800, 600);
    g_ScreenRatio = 800.0f/600.0f;

    glfwMakeContextCurrent(g_GLWindow);
    
    // load shaders
    gl3wInit();
    GLuint vertex_shader_id = LoadShader_Vertex("../triangles.vert");
    GLuint fragment_shader_id = LoadShader_Fragment("../triangles.frag");
    GLuint program_id = CreateGpuProgram(vertex_shader_id, fragment_shader_id);

    GLint modelMatrixLocation      = glGetUniformLocation(program_id, "modelMatrix"     );
    GLint viewMatrixLocation       = glGetUniformLocation(program_id, "viewMatrix"      );
    GLint projectionMatrixLocation = glGetUniformLocation(program_id, "projectionMatrix");
    GLint colorVectorLocation      = glGetUniformLocation(program_id, "colorVector"     );
    GLint textureSamplerLocation   = glGetUniformLocation(program_id, "textureSampler"  );
    
    g_VertexShaderTypeLocation   = glGetUniformLocation(program_id, "vertexShaderType");
    g_FragmentShaderTypeLocation = glGetUniformLocation(program_id, "fragmentShaderType");
    g_VertexShaderType   = 0;
    g_FragmentShaderType = 0;
    
    g_UseClose2GLLocation     = glGetUniformLocation(program_id, "useClose2GL");
    g_VertexArrayObject_id = -1;

    glFrontFace(GL_CW);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    
    glUniform1i(g_UseClose2GLLocation       , g_UseClose2GL       );
    glUniform1i(g_VertexShaderTypeLocation  , g_VertexShaderType  );
    glUniform1i(g_FragmentShaderTypeLocation, g_FragmentShaderType);
    glUniform1i(textureSamplerLocation, 0);
    
    //initialize back and front buffers
    g_ColorBuffer.width  = g_ScreenWidth;
    g_ColorBuffer.height = g_ScreenHeight;
    g_ColorBuffer.pixels = (ScreenPixel*)calloc(g_ScreenHeight * g_ScreenWidth, sizeof(ScreenPixel));
    
    std::vector<unsigned char> baseTexture;
    for (int i = 0; i < g_ScreenWidth; i++) {
        for (int j = 0; j < g_ScreenHeight; j++) {
            int index = get_index(g_ColorBuffer, i, j);
            g_ColorBuffer.pixels[index].r = 255;
            g_ColorBuffer.pixels[index].g = 255;
            g_ColorBuffer.pixels[index].b = 255;
            g_ColorBuffer.pixels[index].a = 255;
            g_ColorBuffer.pixels[index].z = -FLT_MAX;

            baseTexture.push_back(255);
            baseTexture.push_back(255);
            baseTexture.push_back(255);
            baseTexture.push_back(255);
        }
    }
    
    LoadTexture(baseTexture.data(), g_ScreenWidth, g_ScreenHeight);
    
    while (!glfwWindowShouldClose(g_GLWindow)) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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
        glm::vec4 cameraView;
        if (g_LookAtCamera) {
            cameraView = cameraTarget - camera_position_c;
        } else {
            cameraView = cameraTarget - glm::vec4(x,y,z,1.0f);
        }
        glm::vec4 cameraUp = glm::vec4(0.0f,1.0f,0.0f,0.0f);

        glm::vec4 camera_w = -cameraView/norm(cameraView);
        glm::vec4 camera_u = crossproduct(cameraUp, camera_w);
        glm::vec4 camera_v = crossproduct(camera_w, camera_u);

        g_CameraDistance = norm(cameraView);
        // front/back
        if (g_W_pressed) {
            camera_position_c -= camera_w * 0.05f;
            g_CameraDistance = norm(camera_position_c);
        }
        if (g_S_pressed) {
            camera_position_c += camera_w * 0.05f;
            g_CameraDistance = norm(camera_position_c);
        }
        // left/right
        if (g_A_pressed) {
            camera_position_c -= camera_u * 0.05f;
            g_CameraDistance = norm(camera_position_c);
            if (g_LookAtCamera) {
                g_CameraTheta -= 0.01f;
                cameraView = cameraTarget - camera_position_c;
            }
        }
        if (g_D_pressed) {
            camera_position_c += camera_u * 0.05f;
            g_CameraDistance = norm(camera_position_c);
            if (g_LookAtCamera) {
                g_CameraTheta += 0.01f;
                cameraView = cameraTarget - camera_position_c;
            }
        }
        // up/down
        if (g_Q_pressed) {
            camera_position_c += camera_v * 0.05f;
            g_CameraDistance = norm(camera_position_c);
            if (g_LookAtCamera) {
                g_CameraPhi += 0.01f;
                if (g_CameraPhi > 3.141592f/2) {
                    g_CameraPhi = 3.141592f/2;
                }
                cameraView = cameraTarget - camera_position_c;
            }
        }
        if (g_Z_pressed) {
            camera_position_c -= camera_v * 0.05f;
            g_CameraDistance = norm(camera_position_c);
            if (g_LookAtCamera) {
                g_CameraPhi -= 0.01f;
                if (g_CameraPhi < -3.141592f/2) {
                    g_CameraPhi = -3.141592f/2;
                }
                cameraView = cameraTarget - camera_position_c;
            }
        }
        g_ViewMatrix = Matrix_Camera_View(camera_position_c, cameraView, cameraUp);		
               
        g_ProjectionMatrix = Matrix_Perspective(g_vFov, g_hFov, g_ScreenRatio, g_NearPlane, g_FarPlane);

        glUniformMatrix4fv(viewMatrixLocation,       1 , GL_FALSE , glm::value_ptr(g_ViewMatrix));
        glUniformMatrix4fv(projectionMatrixLocation, 1 , GL_FALSE , glm::value_ptr(g_ProjectionMatrix));

        // rendering
        ShowFramesPerSecond();
        if (g_VertexArrayObject_id != -1) {
            glUniform4f(colorVectorLocation, g_Red, g_Green, g_Blue, 1.0f);
            float max_x = g_VirtualScene["model"].max_coord.x;
            float max_y = g_VirtualScene["model"].max_coord.y;
            float max_z = g_VirtualScene["model"].max_coord.z;
            float min_x = g_VirtualScene["model"].min_coord.x;
            float min_y = g_VirtualScene["model"].min_coord.y;
            float min_z = g_VirtualScene["model"].min_coord.z;
            float trans_x = (min_x + max_x) / 2;
            float trans_y = (min_y + max_y) / 2;
            float trans_z = (min_z + max_z) / 2;
            float size_x  = (max_x - min_x);
            float size_y  = (max_y - min_y);
            float size_z  = (max_z - min_z);
            float scaling_factor = size_x;
            scaling_factor = (size_y > scaling_factor) ? size_y : scaling_factor;
            scaling_factor = (size_z > scaling_factor) ? size_z : scaling_factor;

            if (g_UseClose2GL) {
                g_VertexArrayObject_id = BuildTriangles(g_Model);                
            }
            glBindVertexArray(g_VertexArrayObject_id);

            glm::vec3 objectTranslate = glm::vec3(-trans_x, -trans_y, -trans_z);
            glm::vec3 objectScale = glm::vec3(4.0f / scaling_factor, 4.0f / scaling_factor, 4.0f / scaling_factor);
            g_ModelMatrix = glm::mat4(1.0f);
            g_ModelMatrix = glm::scale(g_ModelMatrix, objectScale);
            g_ModelMatrix = glm::translate(g_ModelMatrix, objectTranslate);

            glDrawElements(
                g_VirtualScene["model"].rendering_mode,
                g_VirtualScene["model"].num_indices,
                GL_UNSIGNED_INT,
                (void*)g_VirtualScene["model"].first_index
            );

            glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, glm::value_ptr(g_ModelMatrix));
            glBindVertexArray(0);
        }
        
        glfwSwapBuffers(g_GLWindow);
        glfwPollEvents();
    }

    glfwDestroyWindow(g_GLWindow);

    glfwTerminate();
    return 0;
}

void LoadTexture(unsigned char *textureData, int width, int height)
{
    GLuint texture_id;
    GLuint sampler_id;
    glGenTextures(1, &texture_id);
    glGenSamplers(1, &sampler_id);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureData);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindSampler(0, sampler_id);
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
        fscanf(fp, "ambient color %f %f %f\n",  &model.ambient_color[i].x,
                                                &model.ambient_color[i].y,
                                                &model.ambient_color[i].z);
        fscanf(fp, "diffuse color %f %f %f\n",  &model.diffuse_color[i].x,
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
        model.triangles[i].v0.color.x = (unsigned char)(int)(255*(model.diffuse_color[color_index].x));
        model.triangles[i].v0.color.y = (unsigned char)(int)(255*(model.diffuse_color[color_index].y));
        model.triangles[i].v0.color.z = (unsigned char)(int)(255*(model.diffuse_color[color_index].z));

        fscanf(fp, "v1 %f %f %f %f %f %f %d\n",
                &model.triangles[i].v1.pos.x,
                &model.triangles[i].v1.pos.y,
                &model.triangles[i].v1.pos.z,
                &model.triangles[i].v1.normal.x,
                &model.triangles[i].v1.normal.y,
                &model.triangles[i].v1.normal.z,
                &color_index);
        model.triangles[i].v1.color.x = (unsigned char)(int)(255*(model.diffuse_color[color_index].x));
        model.triangles[i].v1.color.y = (unsigned char)(int)(255*(model.diffuse_color[color_index].y));
        model.triangles[i].v1.color.z = (unsigned char)(int)(255*(model.diffuse_color[color_index].z));

        fscanf(fp, "v2 %f %f %f %f %f %f %d\n",
                &model.triangles[i].v2.pos.x,
                &model.triangles[i].v2.pos.y,
                &model.triangles[i].v2.pos.z,
                &model.triangles[i].v2.normal.x,
                &model.triangles[i].v2.normal.y,
                &model.triangles[i].v2.normal.z,
                &color_index);
        model.triangles[i].v2.color.x = (unsigned char)(int)(255*(model.diffuse_color[color_index].x));
        model.triangles[i].v2.color.y = (unsigned char)(int)(255*(model.diffuse_color[color_index].y));
        model.triangles[i].v2.color.z = (unsigned char)(int)(255*(model.diffuse_color[color_index].z));	
                
        fscanf(fp, "face normal %f %f %f\n",
                &model.triangles[i].face_normal.x,
                &model.triangles[i].face_normal.y,
                &model.triangles[i].face_normal.z);
    }

    fclose(fp);
    return model;
}

void DrawTriangle(glm::vec4 v1, glm::vec4 v2, glm::vec4 v3, glm::vec3 c1, glm::vec3 c2, glm::vec3 c3)
{
    bool change = false;
    // verificar se os três vertices estão dentro da tela
    if (! (v1.x > 0 && v1.x < g_ScreenWidth && v1.y > 0 && v1.y < g_ScreenHeight &&
           v2.x > 0 && v2.x < g_ScreenWidth && v2.y > 0 && v2.y < g_ScreenHeight &&
           v3.x > 0 && v3.x < g_ScreenWidth && v3.y > 0 && v3.y < g_ScreenHeight)  ) {
        return;
    }
    
    if (g_TogglePoints) {
        int index = get_index(g_ColorBuffer, floor(v1.x), floor(v1.y));
        if (v1.z < g_ColorBuffer.pixels[index].z) {
            g_ColorBuffer.pixels[index].r = g_Red   * 255;
            g_ColorBuffer.pixels[index].g = g_Green * 255;
            g_ColorBuffer.pixels[index].b = g_Blue  * 255;
            g_ColorBuffer.pixels[index].a = 255;
            g_ColorBuffer.pixels[index].z = v1.z;
        }
        index = get_index(g_ColorBuffer, floor(v2.x), floor(v2.y));
        if (v2.z < g_ColorBuffer.pixels[index].z) {
            g_ColorBuffer.pixels[index].r = g_Red   * 255;
            g_ColorBuffer.pixels[index].g = g_Green * 255;
            g_ColorBuffer.pixels[index].b = g_Blue  * 255;
            g_ColorBuffer.pixels[index].a = 255;
            g_ColorBuffer.pixels[index].z = v2.z;
        }
        index = get_index(g_ColorBuffer, floor(v3.x), floor(v3.y));
        if (v3.z < g_ColorBuffer.pixels[index].z) {
            g_ColorBuffer.pixels[index].r = g_Red   * 255;
            g_ColorBuffer.pixels[index].g = g_Green * 255;
            g_ColorBuffer.pixels[index].b = g_Blue  * 255;
            g_ColorBuffer.pixels[index].a = 255;
            g_ColorBuffer.pixels[index].z = v3.z;
        }
        return;
    }
    
    // desenhar primeiro os vertices
    int index = get_index(g_ColorBuffer, floor(v1.x), floor(v1.y));
    if (v1.z < g_ColorBuffer.pixels[index].z) {
        g_ColorBuffer.pixels[index].r = c1.x * 255;
        g_ColorBuffer.pixels[index].g = c1.y * 255;
        g_ColorBuffer.pixels[index].b = c1.z * 255;
        g_ColorBuffer.pixels[index].a = 255;
        g_ColorBuffer.pixels[index].z = v1.z;
    }
    index = get_index(g_ColorBuffer, floor(v2.x), floor(v2.y));
    if (v2.z < g_ColorBuffer.pixels[index].z) {
        g_ColorBuffer.pixels[index].r = c2.x * 255;
        g_ColorBuffer.pixels[index].g = c2.y * 255;
        g_ColorBuffer.pixels[index].b = c2.z * 255;
        g_ColorBuffer.pixels[index].a = 255;
        g_ColorBuffer.pixels[index].z = v2.z;
    }
    index = get_index(g_ColorBuffer, floor(v3.x), floor(v3.y));
    if (v3.z < g_ColorBuffer.pixels[index].z) {
        g_ColorBuffer.pixels[index].r = c3.x * 255;
        g_ColorBuffer.pixels[index].g = c3.y * 255;
        g_ColorBuffer.pixels[index].b = c3.z * 255;
        g_ColorBuffer.pixels[index].a = 255;
        g_ColorBuffer.pixels[index].z = v3.z;
    }
    

    
    // desenhar as arestas
    // find topmost vertex
    int topmost = 0;
    if (v1.y <= v2.y && v1.y <= v3.y) {
        topmost = 1;
    } else if (v2.y <= v1.y && v2.y <= v3.y) {
        topmost = 2;
    } else if (v3.y <= v1.y && v3.y <= v2.y) {
        topmost = 3;
    }
    switch (topmost) {
      case 1: {
        float dx1 = v2.x-v1.x; float dy1 = v2.y-v1.y; float dz1 = v2.z-v1.z;
        float dx2 = v3.x-v1.x; float dy2 = v3.y-v1.y; float dz2 = v3.z-v1.z;
        float dr1 = c2.x-c1.x; float dg1 = c2.y-c1.y; float db1 = c2.z-c1.z;
        float dr2 = c3.x-c1.x; float dg2 = c3.y-c1.y; float db2 = c3.z-c1.z;
        float x0 = v1.x;       float y0 = v1.y;       float z0 = v1.z;
        float r0 = c1.x;       float g0 = c1.y;       float b0 = c1.z;
        float inc1x = dx1/dy1; float inc1z = dz1/dy1;
        float inc2x = dx2/dy2; float inc2z = dz2/dy2;
        float inc1r = dr1/dy1; float inc1g = dg1/dy1; float inc1b = db1/dy1;
        float inc2r = dr2/dy2; float inc2g = dg2/dy2; float inc2b = db2/dy2;
        // start with v2-v1 and v3-v1 as active edges
        float xe1 = x0; float xe2 = x0;
        float ze1 = z0; float ze2 = z0;
        float re1 = r0; float ge1 = g0; float be1 = b0;
        float re2 = r0; float ge2 = g0; float be2 = b0;
        y0 += 1;
        while (y0 <= v2.y && y0 <= v3.y) {
            xe1 += inc1x; xe2 += inc2x;
            if (y0 >= g_ScreenHeight || xe1 >= g_ScreenWidth || xe1 < 0 || xe2 >= g_ScreenWidth || xe2 < 0) {
                break;
            }
            ze1 += inc1z; ze2 += inc2z;
            re1 += inc1r; ge1 += inc1g; be1 += inc1b;
            re2 += inc2r; ge2 += inc2g; be2 += inc2b;
            int xini = 0, xf = 0;
            float zini, zf;
            float rini, gini, bini;
            float rf, gf, bf;
            if (floor(xe1) > floor(xe2)) {
                xini = floor(xe2); xf = floor(xe1);
                zini = ze2;        zf = ze1;
                rini = re2;        rf = re1;
                gini = ge2;        gf = ge1;
                bini = be2;        bf = be1;
            } else {
                xini = floor(xe1); xf = floor(xe2);
                zini = ze1;        zf = ze2;
                rini = re1;        rf = re2;
                gini = ge1;        gf = ge2;
                bini = be1;        bf = be2;
            }
            int pxTotal = xf - xini;
            float zstep = (float)(zf - zini)/(float)pxTotal;
            float rstep = (float)(rf - rini)/(float)pxTotal;
            float gstep = (float)(gf - gini)/(float)pxTotal;
            float bstep = (float)(bf - bini)/(float)pxTotal;
            for (; xini <= xf; xini++) {
                index = get_index(g_ColorBuffer, floor(xini), floor(y0));
                if (zini < g_ColorBuffer.pixels[index].z) {
                    g_ColorBuffer.pixels[index].r = rini * 255;
                    g_ColorBuffer.pixels[index].g = gini * 255;
                    g_ColorBuffer.pixels[index].b = bini * 255;
                    g_ColorBuffer.pixels[index].a = 255;
                    g_ColorBuffer.pixels[index].z = zini;
                }
                zini += zstep;
                rini += rstep;
                gini += gstep;
                bini += bstep;
            }
            y0 += 1;
        }
        if (y0 <= v2.y) {
            // active edges: v2-v1 and v2-v3
            dx2 = v2.x-v3.x; dy2 = v2.y-v3.y; dz2 = v2.z-v3.z;
            dr2 = c2.x-c3.x; dg2 = c2.y-c3.y; db2 = c2.z-c3.z;
            inc2x = dx2/dy2; inc2z = dz2/dy2;
            inc2r = dr2/dy2; inc2g = dg2/dy2; inc2b = db2/dy2;
            xe2 = v3.x; ze2 = v3.z;
            re2 = c3.x; ge2 = c3.y; be2 = c3.z;
            // linha intermediária (y0)
            {
                int xini = 0, xf = 0;
                float zini, zf;
                float rini, gini, bini;
                float rf, gf, bf;
                if (floor(xe1) > floor(xe2)) {
                    xini = floor(xe2); xf = floor(xe1);
                    zini = ze2;        zf = ze1;
                    rini = re2;        rf = re1;
                    gini = ge2;        gf = ge1;
                    bini = be2;        bf = be1;
                } else {
                    xini = floor(xe1); xf = floor(xe2);
                    zini = ze1;        zf = ze2;
                    rini = re1;        rf = re2;
                    gini = ge1;        gf = ge2;
                    bini = be1;        bf = be2;
                }
                int pxTotal = xf - xini;
                float zstep = (float)(zf - zini)/(float)pxTotal;
                float rstep = (float)(rf - rini)/(float)pxTotal;
                float gstep = (float)(gf - gini)/(float)pxTotal;
                float bstep = (float)(bf - bini)/(float)pxTotal;
                for (; xini <= xf; xini++) {
                    index = get_index(g_ColorBuffer, floor(xini), floor(y0));
                    if (zini < g_ColorBuffer.pixels[index].z) {
                        g_ColorBuffer.pixels[index].r = rini * 255;
                        g_ColorBuffer.pixels[index].g = gini * 255;
                        g_ColorBuffer.pixels[index].b = bini * 255;
                        g_ColorBuffer.pixels[index].a = 255;
                        g_ColorBuffer.pixels[index].z = zini;
                    }
                    zini += zstep;
                    rini += rstep;
                    gini += gstep;
                    bini += bstep;
                }
            }
            //
            y0 += 1;
            while (y0 <= v2.y) {
                xe1 += inc1x; xe2 += inc2x;
                if (y0 >= g_ScreenHeight || xe1 >= g_ScreenWidth || xe1 < 0 || xe2 >= g_ScreenWidth || xe2 < 0) {
                    break;
                }
                ze1 += inc1z; ze2 += inc2z;
                re1 += inc1r; ge1 += inc1g; be1 += inc1b;
                re2 += inc2r; ge2 += inc2g; be2 += inc2b;
                int xini = 0, xf = 0;
                float zini, zf;
                float rini, gini, bini;
                float rf, gf, bf;
                if (floor(xe1) > floor(xe2)) {
                    xini = floor(xe2); xf = floor(xe1);
                    zini = ze2;        zf = ze1;
                    rini = re2;        rf = re1;
                    gini = ge2;        gf = ge1;
                    bini = be2;        bf = be1;
                } else {
                    xini = floor(xe1); xf = floor(xe2);
                    zini = ze1;        zf = ze2;
                    rini = re1;        rf = re2;
                    gini = ge1;        gf = ge2;
                    bini = be1;        bf = be2;
                }
                int pxTotal = xf - xini;
                float zstep = (float)(zf - zini)/(float)pxTotal;
                float rstep = (float)(rf - rini)/(float)pxTotal;
                float gstep = (float)(gf - gini)/(float)pxTotal;
                float bstep = (float)(bf - bini)/(float)pxTotal;
                for (; xini <= xf; xini++) {
                    index = get_index(g_ColorBuffer, floor(xini), floor(y0));
                    if (zini < g_ColorBuffer.pixels[index].z) {
                        g_ColorBuffer.pixels[index].r = rini * 255;
                        g_ColorBuffer.pixels[index].g = gini * 255;
                        g_ColorBuffer.pixels[index].b = bini * 255;
                        g_ColorBuffer.pixels[index].a = 255;
                        g_ColorBuffer.pixels[index].z = zini;
                    }
                    zini += zstep;
                    rini += rstep;
                    gini += gstep;
                    bini += bstep;
                }
                y0 += 1;
            }
        } else if (y0 <= v3.y) {
            // active edges: v3-v2 and v3-v1
            dx1 = v3.x-v2.x; dy1 = v3.y-v2.y; dz1 = v3.z-v2.z;
            dr1 = c3.x-c2.x; dg1 = c3.y-c2.y; db1 = c3.z-c2.z;
            inc1x = dx1/dy1; inc1z = dz1/dy1;
            inc1r = dr1/dy1; inc1g = dg1/dy1; inc1b = db1/dy1;
            xe1 = v2.x; ze1 = v2.z;
            re1 = c2.x; ge1 = c2.y; be1 = c2.z;
            // linha intermediária (y0)
            {
                int xini = 0, xf = 0;
                float zini, zf;
                float rini, gini, bini;
                float rf, gf, bf;
                if (floor(xe1) > floor(xe2)) {
                    xini = floor(xe2); xf = floor(xe1);
                    zini = ze2;        zf = ze1;
                    rini = re2;        rf = re1;
                    gini = ge2;        gf = ge1;
                    bini = be2;        bf = be1;
                } else {
                    xini = floor(xe1); xf = floor(xe2);
                    zini = ze1;        zf = ze2;
                    rini = re1;        rf = re2;
                    gini = ge1;        gf = ge2;
                    bini = be1;        bf = be2;
                }
                int pxTotal = xf - xini;
                float zstep = (float)(zf - zini)/(float)pxTotal;
                float rstep = (float)(rf - rini)/(float)pxTotal;
                float gstep = (float)(gf - gini)/(float)pxTotal;
                float bstep = (float)(bf - bini)/(float)pxTotal;
                for (; xini <= xf; xini++) {
                    index = get_index(g_ColorBuffer, floor(xini), floor(y0));
                    if (zini < g_ColorBuffer.pixels[index].z) {
                        g_ColorBuffer.pixels[index].r = rini * 255;
                        g_ColorBuffer.pixels[index].g = gini * 255;
                        g_ColorBuffer.pixels[index].b = bini * 255;
                        g_ColorBuffer.pixels[index].a = 255;
                        g_ColorBuffer.pixels[index].z = zini;
                    }
                    zini += zstep;
                    rini += rstep;
                    gini += gstep;
                    bini += bstep;
                }
            }
            //
            y0 += 1;
            while (y0 <= v3.y) {
                xe1 += inc1x; xe2 += inc2x;
                if (y0 >= g_ScreenHeight || xe1 >= g_ScreenWidth || xe1 < 0 || xe2 >= g_ScreenWidth || xe2 < 0) {
                    break;
                }
                ze1 += inc1z; ze2 += inc2z;
                re1 += inc1r; ge1 += inc1g; be1 += inc1b;
                re2 += inc2r; ge2 += inc2g; be2 += inc2b;
                int xini = 0, xf = 0;
                float zini, zf;
                float rini, gini, bini;
                float rf, gf, bf;
                if (floor(xe1) > floor(xe2)) {
                    xini = floor(xe2); xf = floor(xe1);
                    zini = ze2;        zf = ze1;
                    rini = re2;        rf = re1;
                    gini = ge2;        gf = ge1;
                    bini = be2;        bf = be1;
                } else {
                    xini = floor(xe1); xf = floor(xe2);
                    zini = ze1;        zf = ze2;
                    rini = re1;        rf = re2;
                    gini = ge1;        gf = ge2;
                    bini = be1;        bf = be2;
                }
                int pxTotal = xf - xini;
                float zstep = (float)(zf - zini)/(float)pxTotal;
                float rstep = (float)(rf - rini)/(float)pxTotal;
                float gstep = (float)(gf - gini)/(float)pxTotal;
                float bstep = (float)(bf - bini)/(float)pxTotal;
                for (; xini <= xf; xini++) {
                    index = get_index(g_ColorBuffer, floor(xini), floor(y0));
                    if (zini < g_ColorBuffer.pixels[index].z) {
                        g_ColorBuffer.pixels[index].r = rini * 255;
                        g_ColorBuffer.pixels[index].g = gini * 255;
                        g_ColorBuffer.pixels[index].b = bini * 255;
                        g_ColorBuffer.pixels[index].a = 255;
                        g_ColorBuffer.pixels[index].z = zini;
                    }
                    zini += zstep;
                    rini += rstep;
                    gini += gstep;
                    bini += bstep;
                }
                y0 += 1;
            }
        }
      }
      break;
      case 2: {
        float dx1 = v1.x-v2.x; float dy1 = v1.y-v2.y; float dz1 = v1.z-v2.z;
        float dx2 = v3.x-v2.x; float dy2 = v3.y-v2.y; float dz2 = v3.z-v2.z;
        float dr1 = c1.x-c2.x; float dg1 = c1.y-c2.y; float db1 = c1.z-c2.z;
        float dr2 = c3.x-c2.x; float dg2 = c3.y-c2.y; float db2 = c3.z-c2.z;
        float x0 = v2.x;       float y0 = v2.y;       float z0 = v2.z;
        float r0 = c2.x;       float g0 = c2.y;       float b0 = c2.z;
        float inc1x = dx1/dy1; float inc1z = dz1/dy1;
        float inc2x = dx2/dy2; float inc2z = dz2/dy2;
        float inc1r = dr1/dy1; float inc1g = dg1/dy1; float inc1b = db1/dy1;
        float inc2r = dr2/dy2; float inc2g = dg2/dy2; float inc2b = db2/dy2;
        // start with v1-v2 and v3-v2 as active edges
        float xe1 = x0; float xe2 = x0;
        float ze1 = z0; float ze2 = z0;
        float re1 = r0; float ge1 = g0; float be1 = b0;
        float re2 = r0; float ge2 = g0; float be2 = b0;
        y0 += 1;
        while (y0 <= v1.y && y0 <= v3.y) {
            xe1 += inc1x; xe2 += inc2x;
            if (y0 >= g_ScreenHeight || xe1 >= g_ScreenWidth || xe1 < 0 || xe2 >= g_ScreenWidth || xe2 < 0) {
                break;
            }
            ze1 += inc1z; ze2 += inc2z;
            re1 += inc1r; ge1 += inc1g; be1 += inc1b;
            re2 += inc2r; ge2 += inc2g; be2 += inc2b;
            int xini = 0, xf = 0;
            float zini, zf;
            float rini, gini, bini;
            float rf, gf, bf;
            if (floor(xe1) > floor(xe2)) {
                xini = floor(xe2); xf = floor(xe1);
                zini = ze2;        zf = ze1;
                rini = re2;        rf = re1;
                gini = ge2;        gf = ge1;
                bini = be2;        bf = be1;
            } else {
                xini = floor(xe1); xf = floor(xe2);
                zini = ze1;        zf = ze2;
                rini = re1;        rf = re2;
                gini = ge1;        gf = ge2;
                bini = be1;        bf = be2;
            }
            int pxTotal = xf - xini;
            float zstep = (float)(zf - zini)/(float)pxTotal;
            float rstep = (float)(rf - rini)/(float)pxTotal;
            float gstep = (float)(gf - gini)/(float)pxTotal;
            float bstep = (float)(bf - bini)/(float)pxTotal;
            for (; xini <= xf; xini++) {
                index = get_index(g_ColorBuffer, floor(xini), floor(y0));
                if (zini < g_ColorBuffer.pixels[index].z) {
                    g_ColorBuffer.pixels[index].r = rini * 255;
                    g_ColorBuffer.pixels[index].g = gini * 255;
                    g_ColorBuffer.pixels[index].b = bini * 255;
                    g_ColorBuffer.pixels[index].a = 255;
                    g_ColorBuffer.pixels[index].z = zini;
                }
                zini += zstep;
                rini += rstep;
                gini += gstep;
                bini += bstep;
            }
            y0 += 1;
        }
        if (y0 <= v1.y) {
            // active edges: v1-v2 and v1-v3
            dx2 = v1.x-v3.x; dy2 = v1.y-v3.y; dz2 = v1.z-v3.z;
            dr2 = c1.x-c3.x; dg2 = c1.y-c3.y; db2 = c1.z-c3.z;
            inc2x = dx2/dy2; inc2z = dz2/dy2;
            inc2r = dr2/dy2; inc2g = dg2/dy2; inc2b = db2/dy2;
            xe2 = v3.x; ze2 = v3.z;
            re2 = c3.x; ge2 = c3.y; be2 = c3.z;
            // linha intermediária (y0)
            {
                int xini = 0, xf = 0;
                float zini, zf;
                float rini, gini, bini;
                float rf, gf, bf;
                if (floor(xe1) > floor(xe2)) {
                    xini = floor(xe2); xf = floor(xe1);
                    zini = ze2;        zf = ze1;
                    rini = re2;        rf = re1;
                    gini = ge2;        gf = ge1;
                    bini = be2;        bf = be1;
                } else {
                    xini = floor(xe1); xf = floor(xe2);
                    zini = ze1;        zf = ze2;
                    rini = re1;        rf = re2;
                    gini = ge1;        gf = ge2;
                    bini = be1;        bf = be2;
                }
                int pxTotal = xf - xini;
                float zstep = (float)(zf - zini)/(float)pxTotal;
                float rstep = (float)(rf - rini)/(float)pxTotal;
                float gstep = (float)(gf - gini)/(float)pxTotal;
                float bstep = (float)(bf - bini)/(float)pxTotal;
                for (; xini <= xf; xini++) {
                    index = get_index(g_ColorBuffer, floor(xini), floor(y0));
                    if (zini < g_ColorBuffer.pixels[index].z) {
                        g_ColorBuffer.pixels[index].r = rini * 255;
                        g_ColorBuffer.pixels[index].g = gini * 255;
                        g_ColorBuffer.pixels[index].b = bini * 255;
                        g_ColorBuffer.pixels[index].a = 255;
                        g_ColorBuffer.pixels[index].z = zini;
                    }
                    zini += zstep;
                    rini += rstep;
                    gini += gstep;
                    bini += bstep;
                }
            }
            //
            y0 += 1;
            while (y0 <= v1.y) {
                xe1 += inc1x; xe2 += inc2x;
                if (y0 >= g_ScreenHeight || xe1 >= g_ScreenWidth || xe1 < 0 || xe2 >= g_ScreenWidth || xe2 < 0) {
                    break;
                }
                ze1 += inc1z; ze2 += inc2z;
                re1 += inc1r; ge1 += inc1g; be1 += inc1b;
                re2 += inc2r; ge2 += inc2g; be2 += inc2b;
                int xini = 0, xf = 0;
                float zini, zf;
                float rini, gini, bini;
                float rf, gf, bf;
                if (floor(xe1) > floor(xe2)) {
                    xini = floor(xe2); xf = floor(xe1);
                    zini = ze2;        zf = ze1;
                    rini = re2;        rf = re1;
                    gini = ge2;        gf = ge1;
                    bini = be2;        bf = be1;
                } else {
                    xini = floor(xe1); xf = floor(xe2);
                    zini = ze1;        zf = ze2;
                    rini = re1;        rf = re2;
                    gini = ge1;        gf = ge2;
                    bini = be1;        bf = be2;
                }
                int pxTotal = xf - xini;
                float zstep = (float)(zf - zini)/(float)pxTotal;
                float rstep = (float)(rf - rini)/(float)pxTotal;
                float gstep = (float)(gf - gini)/(float)pxTotal;
                float bstep = (float)(bf - bini)/(float)pxTotal;
                for (; xini <= xf; xini++) {
                    index = get_index(g_ColorBuffer, floor(xini), floor(y0));
                    if (zini < g_ColorBuffer.pixels[index].z) {
                        g_ColorBuffer.pixels[index].r = rini * 255;
                        g_ColorBuffer.pixels[index].g = gini * 255;
                        g_ColorBuffer.pixels[index].b = bini * 255;
                        g_ColorBuffer.pixels[index].a = 255;
                        g_ColorBuffer.pixels[index].z = zini;
                    }
                    zini += zstep;
                    rini += rstep;
                    gini += gstep;
                    bini += bstep;
                }
                y0 += 1;
            }
        } 
        else if (y0 <= v3.y) {
            // active edges: v3-v1 and v3-v2
            dx1 = v3.x-v1.x; dy1 = v3.y-v1.y; dz1 = v3.z-v1.z;
            dr1 = c3.x-c1.x; dg1 = c3.y-c1.y; db1 = c3.z-c1.z;
            inc1x = dx1/dy1; inc1z = dz1/dy1;
            inc1r = dr1/dy1; inc1g = dg1/dy1; inc1b = db1/dy1;
            xe1 = v1.x; ze1 = v1.z;
            re1 = c1.x; ge1 = c1.y; be1 = c1.z;
            // linha intermediária (y0)
            {
                int xini = 0, xf = 0;
                float zini, zf;
                float rini, gini, bini;
                float rf, gf, bf;
                if (floor(xe1) > floor(xe2)) {
                    xini = floor(xe2); xf = floor(xe1);
                    zini = ze2;        zf = ze1;
                    rini = re2;        rf = re1;
                    gini = ge2;        gf = ge1;
                    bini = be2;        bf = be1;
                } else {
                    xini = floor(xe1); xf = floor(xe2);
                    zini = ze1;        zf = ze2;
                    rini = re1;        rf = re2;
                    gini = ge1;        gf = ge2;
                    bini = be1;        bf = be2;
                }
                int pxTotal = xf - xini;
                float zstep = (float)(zf - zini)/(float)pxTotal;
                float rstep = (float)(rf - rini)/(float)pxTotal;
                float gstep = (float)(gf - gini)/(float)pxTotal;
                float bstep = (float)(bf - bini)/(float)pxTotal;
                for (; xini <= xf; xini++) {
                    index = get_index(g_ColorBuffer, floor(xini), floor(y0));
                    if (zini < g_ColorBuffer.pixels[index].z) {
                        g_ColorBuffer.pixels[index].r = rini * 255;
                        g_ColorBuffer.pixels[index].g = gini * 255;
                        g_ColorBuffer.pixels[index].b = bini * 255;
                        g_ColorBuffer.pixels[index].a = 255;
                        g_ColorBuffer.pixels[index].z = zini;
                    }
                    zini += zstep;
                    rini += rstep;
                    gini += gstep;
                    bini += bstep;
                }
            }
            //
            y0 += 1;
            while (y0 <= v3.y) {
                xe1 += inc1x; xe2 += inc2x;
                if (y0 >= g_ScreenHeight || xe1 >= g_ScreenWidth || xe1 < 0 || xe2 >= g_ScreenWidth || xe2 < 0) {
                    break;
                }
                ze1 += inc1z; ze2 += inc2z;
                re1 += inc1r; ge1 += inc1g; be1 += inc1b;
                re2 += inc2r; ge2 += inc2g; be2 += inc2b;
                int xini = 0, xf = 0;
                float zini, zf;
                float rini, gini, bini;
                float rf, gf, bf;
                if (floor(xe1) > floor(xe2)) {
                    xini = floor(xe2); xf = floor(xe1);
                    zini = ze2;        zf = ze1;
                    rini = re2;        rf = re1;
                    gini = ge2;        gf = ge1;
                    bini = be2;        bf = be1;
                } else {
                    xini = floor(xe1); xf = floor(xe2);
                    zini = ze1;        zf = ze2;
                    rini = re1;        rf = re2;
                    gini = ge1;        gf = ge2;
                    bini = be1;        bf = be2;
                }
                int pxTotal = xf - xini;
                float zstep = (float)(zf - zini)/(float)pxTotal;
                float rstep = (float)(rf - rini)/(float)pxTotal;
                float gstep = (float)(gf - gini)/(float)pxTotal;
                float bstep = (float)(bf - bini)/(float)pxTotal;
                for (; xini <= xf; xini++) {
                    index = get_index(g_ColorBuffer, floor(xini), floor(y0));
                    if (zini < g_ColorBuffer.pixels[index].z) {
                        g_ColorBuffer.pixels[index].r = rini * 255;
                        g_ColorBuffer.pixels[index].g = gini * 255;
                        g_ColorBuffer.pixels[index].b = bini * 255;
                        g_ColorBuffer.pixels[index].a = 255;
                        g_ColorBuffer.pixels[index].z = zini;
                    }
                    zini += zstep;
                    rini += rstep;
                    gini += gstep;
                    bini += bstep;
                }
                y0 += 1;
            }
        }
      }
      break;
      case 3: {
        float dx1 = v1.x-v3.x; float dy1 = v1.y-v3.y; float dz1 = v1.z-v3.z;
        float dx2 = v2.x-v3.x; float dy2 = v2.y-v3.y; float dz2 = v2.z-v3.z;
        float dr1 = c1.x-c3.x; float dg1 = c1.y-c3.y; float db1 = c1.z-c3.z;
        float dr2 = c2.x-c3.x; float dg2 = c2.y-c3.y; float db2 = c2.z-c3.z;
        float x0 = v3.x;       float y0 = v3.y;       float z0 = v3.z;
        float r0 = c3.x;       float g0 = c3.y;       float b0 = c3.z;
        float inc1x = dx1/dy1; float inc1z = dz1/dy1;
        float inc2x = dx2/dy2; float inc2z = dz2/dy2;
        float inc1r = dr1/dy1; float inc1g = dg1/dy1; float inc1b = db1/dy1;
        float inc2r = dr2/dy2; float inc2g = dg2/dy2; float inc2b = db2/dy2;
        // start with v1-v3 and v2-v3 as active edges
        float xe1 = x0; float xe2 = x0;
        float ze1 = z0; float ze2 = z0;
        float re1 = r0; float ge1 = g0; float be1 = b0;
        float re2 = r0; float ge2 = g0; float be2 = b0;
        y0 += 1;
        while (y0 <= v1.y && y0 <= v2.y) {
            xe1 += inc1x; xe2 += inc2x;
            if (y0 >= g_ScreenHeight || xe1 >= g_ScreenWidth || xe1 < 0 || xe2 >= g_ScreenWidth || xe2 < 0) {
                break;
            }
            ze1 += inc1z; ze2 += inc2z;
            re1 += inc1r; ge1 += inc1g; be1 += inc1b;
            re2 += inc2r; ge2 += inc2g; be2 += inc2b;
            int xini = 0, xf = 0;
            float zini, zf;
            float rini, gini, bini;
            float rf, gf, bf;
            if (floor(xe1) > floor(xe2)) {
                xini = floor(xe2); xf = floor(xe1);
                zini = ze2;        zf = ze1;
                rini = re2;        rf = re1;
                gini = ge2;        gf = ge1;
                bini = be2;        bf = be1;
            } else {
                xini = floor(xe1); xf = floor(xe2);
                zini = ze1;        zf = ze2;
                rini = re1;        rf = re2;
                gini = ge1;        gf = ge2;
                bini = be1;        bf = be2;
            }
            int pxTotal = xf - xini;
            float zstep = (float)(zf - zini)/(float)pxTotal;
            float rstep = (float)(rf - rini)/(float)pxTotal;
            float gstep = (float)(gf - gini)/(float)pxTotal;
            float bstep = (float)(bf - bini)/(float)pxTotal;
            for (; xini <= xf; xini++) {
                index = get_index(g_ColorBuffer, floor(xini), floor(y0));
                if (zini < g_ColorBuffer.pixels[index].z) {
                    g_ColorBuffer.pixels[index].r = rini * 255;
                    g_ColorBuffer.pixels[index].g = gini * 255;
                    g_ColorBuffer.pixels[index].b = bini * 255;
                    g_ColorBuffer.pixels[index].a = 255;
                    g_ColorBuffer.pixels[index].z = zini;
                }
                zini += zstep;
                rini += rstep;
                gini += gstep;
                bini += bstep;
            }
            y0 += 1;
        }
        if (y0 <= v1.y) {
            // active edges: v1-v3 and v1-v2
            dx2 = v1.x-v2.x; dy2 = v1.y-v2.y; dz2 = v1.z-v2.z;
            dr2 = c1.x-c2.x; dg2 = c1.y-c2.y; db2 = c1.z-c2.z;
            inc2x = dx2/dy2; inc2z = dz2/dy2;
            inc2r = dr2/dy2; inc2g = dg2/dy2; inc2b = db2/dy2;
            xe2 = v2.x; ze2 = v2.z;
            re2 = c2.x; ge2 = c2.y; be2 = c2.z;
            // linha intermediária (y0)
            {
                int xini = 0, xf = 0;
                float zini, zf;
                float rini, gini, bini;
                float rf, gf, bf;
                if (floor(xe1) > floor(xe2)) {
                    xini = floor(xe2); xf = floor(xe1);
                    zini = ze2;        zf = ze1;
                    rini = re2;        rf = re1;
                    gini = ge2;        gf = ge1;
                    bini = be2;        bf = be1;
                } else {
                    xini = floor(xe1); xf = floor(xe2);
                    zini = ze1;        zf = ze2;
                    rini = re1;        rf = re2;
                    gini = ge1;        gf = ge2;
                    bini = be1;        bf = be2;
                }
                int pxTotal = xf - xini;
                float zstep = (float)(zf - zini)/(float)pxTotal;
                float rstep = (float)(rf - rini)/(float)pxTotal;
                float gstep = (float)(gf - gini)/(float)pxTotal;
                float bstep = (float)(bf - bini)/(float)pxTotal;
                for (; xini <= xf; xini++) {
                    index = get_index(g_ColorBuffer, floor(xini), floor(y0));
                    if (zini < g_ColorBuffer.pixels[index].z) {
                        g_ColorBuffer.pixels[index].r = rini * 255;
                        g_ColorBuffer.pixels[index].g = gini * 255;
                        g_ColorBuffer.pixels[index].b = bini * 255;
                        g_ColorBuffer.pixels[index].a = 255;
                        g_ColorBuffer.pixels[index].z = zini;
                    }
                    zini += zstep;
                    rini += rstep;
                    gini += gstep;
                    bini += bstep;
                }
            }
            //
            y0 += 1;
            while (y0 <= v1.y) {
                xe1 += inc1x; xe2 += inc2x;
                if (y0 >= g_ScreenHeight || xe1 >= g_ScreenWidth || xe1 < 0 || xe2 >= g_ScreenWidth || xe2 < 0) {
                    break;
                }
                ze1 += inc1z; ze2 += inc2z;
                re1 += inc1r; ge1 += inc1g; be1 += inc1b;
                re2 += inc2r; ge2 += inc2g; be2 += inc2b;
                int xini = 0, xf = 0;
                float zini, zf;
                float rini, gini, bini;
                float rf, gf, bf;
                if (floor(xe1) > floor(xe2)) {
                    xini = floor(xe2); xf = floor(xe1);
                    zini = ze2;        zf = ze1;
                    rini = re2;        rf = re1;
                    gini = ge2;        gf = ge1;
                    bini = be2;        bf = be1;
                } else {
                    xini = floor(xe1); xf = floor(xe2);
                    zini = ze1;        zf = ze2;
                    rini = re1;        rf = re2;
                    gini = ge1;        gf = ge2;
                    bini = be1;        bf = be2;
                }
                int pxTotal = xf - xini;
                float zstep = (float)(zf - zini)/(float)pxTotal;
                float rstep = (float)(rf - rini)/(float)pxTotal;
                float gstep = (float)(gf - gini)/(float)pxTotal;
                float bstep = (float)(bf - bini)/(float)pxTotal;
                for (; xini <= xf; xini++) {
                    index = get_index(g_ColorBuffer, floor(xini), floor(y0));
                    if (zini < g_ColorBuffer.pixels[index].z) {
                        g_ColorBuffer.pixels[index].r = rini * 255;
                        g_ColorBuffer.pixels[index].g = gini * 255;
                        g_ColorBuffer.pixels[index].b = bini * 255;
                        g_ColorBuffer.pixels[index].a = 255;
                        g_ColorBuffer.pixels[index].z = zini;
                    }
                    zini += zstep;
                    rini += rstep;
                    gini += gstep;
                    bini += bstep;
                }
                y0 += 1;
            }
        } else if (y0 <= v2.y) {
            // active edges: v2-v1 and v2-v3
            dx1 = v2.x-v1.x; dy1 = v2.y-v1.y; dz1 = v2.z-v1.z;
            dr1 = c2.x-c1.x; dg1 = c2.y-c1.y; db1 = c2.z-c1.z;
            inc1x = dx1/dy1; inc1z = dz1/dy1;
            inc1r = dr1/dy1; inc1g = dg1/dy1; inc1b = db1/dy1;
            xe1 = v1.x; ze1 = v1.z;
            re1 = c1.x; ge1 = c1.y; be1 = c1.z;
            // linha intermediária (y0)
            {
                int xini = 0, xf = 0;
                float zini, zf;
                float rini, gini, bini;
                float rf, gf, bf;
                if (floor(xe1) > floor(xe2)) {
                    xini = floor(xe2); xf = floor(xe1);
                    zini = ze2;        zf = ze1;
                    rini = re2;        rf = re1;
                    gini = ge2;        gf = ge1;
                    bini = be2;        bf = be1;
                } else {
                    xini = floor(xe1); xf = floor(xe2);
                    zini = ze1;        zf = ze2;
                    rini = re1;        rf = re2;
                    gini = ge1;        gf = ge2;
                    bini = be1;        bf = be2;
                }
                int pxTotal = xf - xini;
                float zstep = (float)(zf - zini)/(float)pxTotal;
                float rstep = (float)(rf - rini)/(float)pxTotal;
                float gstep = (float)(gf - gini)/(float)pxTotal;
                float bstep = (float)(bf - bini)/(float)pxTotal;
                for (; xini <= xf; xini++) {
                    index = get_index(g_ColorBuffer, floor(xini), floor(y0));
                    if (zini < g_ColorBuffer.pixels[index].z) {
                        g_ColorBuffer.pixels[index].r = rini * 255;
                        g_ColorBuffer.pixels[index].g = gini * 255;
                        g_ColorBuffer.pixels[index].b = bini * 255;
                        g_ColorBuffer.pixels[index].a = 255;
                        g_ColorBuffer.pixels[index].z = zini;
                    }
                    zini += zstep;
                    rini += rstep;
                    gini += gstep;
                    bini += bstep;
                }
            }
            //
            y0 += 1;
            while (y0 <= v2.y) {
                xe1 += inc1x; xe2 += inc2x;
                if (y0 >= g_ScreenHeight || xe1 >= g_ScreenWidth || xe1 < 0 || xe2 >= g_ScreenWidth || xe2 < 0) {
                    break;
                }
                ze1 += inc1z; ze2 += inc2z;
                re1 += inc1r; ge1 += inc1g; be1 += inc1b;
                re2 += inc2r; ge2 += inc2g; be2 += inc2b;
                int xini = 0, xf = 0;
                float zini, zf;
                float rini, gini, bini;
                float rf, gf, bf;
                if (floor(xe1) > floor(xe2)) {
                    xini = floor(xe2); xf = floor(xe1);
                    zini = ze2;        zf = ze1;
                    rini = re2;        rf = re1;
                    gini = ge2;        gf = ge1;
                    bini = be2;        bf = be1;
                } else {
                    xini = floor(xe1); xf = floor(xe2);
                    zini = ze1;        zf = ze2;
                    rini = re1;        rf = re2;
                    gini = ge1;        gf = ge2;
                    bini = be1;        bf = be2;
                }
                int pxTotal = xf - xini;
                float zstep = (float)(zf - zini)/(float)pxTotal;
                float rstep = (float)(rf - rini)/(float)pxTotal;
                float gstep = (float)(gf - gini)/(float)pxTotal;
                float bstep = (float)(bf - bini)/(float)pxTotal;
                for (; xini <= xf; xini++) {
                    index = get_index(g_ColorBuffer, floor(xini), floor(y0));
                    if (zini < g_ColorBuffer.pixels[index].z) {
                        g_ColorBuffer.pixels[index].r = rini * 255;
                        g_ColorBuffer.pixels[index].g = gini * 255;
                        g_ColorBuffer.pixels[index].b = bini * 255;
                        g_ColorBuffer.pixels[index].a = 255;
                        g_ColorBuffer.pixels[index].z = zini;
                    }
                    zini += zstep;
                    rini += rstep;
                    gini += gstep;
                    bini += bstep;
                }
                y0 += 1;
            }
        }
      }
      break;
    }
}

GLuint BuildTriangles(ModelObject model)
{
    glm::vec3 min_coord = glm::vec3(FLT_MAX, FLT_MAX, FLT_MAX);
    glm::vec3 max_coord = glm::vec3(FLT_MIN, FLT_MIN, FLT_MIN);
    int num_vertices = model.num_triangles * 3;
    std::vector<float> normal_coefficients;
    std::vector<float> model_coefficients;
    for (int i = 0; i < model.num_triangles; i++) {
        Triangle triangle = model.triangles[i];
        // v0
        model_coefficients.push_back(triangle.v0.pos.x); // X
        model_coefficients.push_back(triangle.v0.pos.y); // Y
        model_coefficients.push_back(triangle.v0.pos.z); // Z
        model_coefficients.push_back(1.0f); // W
        normal_coefficients.push_back(triangle.v0.normal.x);
        normal_coefficients.push_back(triangle.v0.normal.y);
        normal_coefficients.push_back(triangle.v0.normal.z);
        normal_coefficients.push_back(0.f);
        
        // v1
        model_coefficients.push_back(triangle.v1.pos.x); // X
        model_coefficients.push_back(triangle.v1.pos.y); // Y
        model_coefficients.push_back(triangle.v1.pos.z); // Z
        model_coefficients.push_back(1.0f); // W
        normal_coefficients.push_back(triangle.v1.normal.x);
        normal_coefficients.push_back(triangle.v1.normal.y);
        normal_coefficients.push_back(triangle.v1.normal.z);
        normal_coefficients.push_back(0.f);
        
        // v2
        model_coefficients.push_back(triangle.v2.pos.x); // X
        model_coefficients.push_back(triangle.v2.pos.y); // Y
        model_coefficients.push_back(triangle.v2.pos.z); // Z
        model_coefficients.push_back(1.0f); // W
        normal_coefficients.push_back(triangle.v2.normal.x);
        normal_coefficients.push_back(triangle.v2.normal.y);
        normal_coefficients.push_back(triangle.v2.normal.z);
        normal_coefficients.push_back(0.f);

        min_coord.x = (triangle.v0.pos.x < min_coord.x) ? triangle.v0.pos.x : min_coord.x;
        max_coord.x = (triangle.v0.pos.x > max_coord.x) ? triangle.v0.pos.x : max_coord.x;
        min_coord.x = (triangle.v1.pos.x < min_coord.x) ? triangle.v1.pos.x : min_coord.x;
        max_coord.x = (triangle.v1.pos.x > max_coord.x) ? triangle.v1.pos.x : max_coord.x;
        min_coord.x = (triangle.v2.pos.x < min_coord.x) ? triangle.v2.pos.x : min_coord.x;
        max_coord.x = (triangle.v2.pos.x > max_coord.x) ? triangle.v2.pos.x : max_coord.x;
        
        min_coord.y = (triangle.v0.pos.y < min_coord.y) ? triangle.v0.pos.y : min_coord.y;
        max_coord.y = (triangle.v0.pos.y > max_coord.y) ? triangle.v0.pos.y : max_coord.y;
        min_coord.y = (triangle.v1.pos.y < min_coord.y) ? triangle.v1.pos.y : min_coord.y;
        max_coord.y = (triangle.v1.pos.y > max_coord.y) ? triangle.v1.pos.y : max_coord.y;
        min_coord.y = (triangle.v2.pos.y < min_coord.y) ? triangle.v2.pos.y : min_coord.y;
        max_coord.y = (triangle.v2.pos.y > max_coord.y) ? triangle.v2.pos.y : max_coord.y;
        
        min_coord.z = (triangle.v0.pos.z < min_coord.z) ? triangle.v0.pos.z : min_coord.z;
        max_coord.z = (triangle.v0.pos.z > max_coord.z) ? triangle.v0.pos.z : max_coord.z;
        min_coord.z = (triangle.v1.pos.z < min_coord.z) ? triangle.v1.pos.z : min_coord.z;
        max_coord.z = (triangle.v1.pos.z > max_coord.z) ? triangle.v1.pos.z : max_coord.z;
        min_coord.z = (triangle.v2.pos.z < min_coord.z) ? triangle.v2.pos.z : min_coord.z;
        max_coord.z = (triangle.v2.pos.z > max_coord.z) ? triangle.v2.pos.z : max_coord.z;    
    }
    if (g_UseClose2GL) {
        free(g_ColorBuffer.pixels);
        g_ColorBuffer.width  = g_ScreenWidth;
        g_ColorBuffer.height = g_ScreenHeight;
        g_ColorBuffer.pixels = (ScreenPixel*)calloc(g_ScreenHeight * g_ScreenWidth, sizeof(ScreenPixel));
        for (int i = 0; i < g_ScreenWidth; i++) {
            for (int j = 0; j < g_ScreenHeight; j++) {
                int index = get_index(g_ColorBuffer, i, j);
                g_ColorBuffer.pixels[index].r = 255;
                g_ColorBuffer.pixels[index].g = 255;
                g_ColorBuffer.pixels[index].b = 255;
                g_ColorBuffer.pixels[index].a = 255;
                g_ColorBuffer.pixels[index].z = FLT_MAX;
            }
        }
        
        GLfloat close2gl_coefficients[num_vertices * 4];
        int clipped_vertices = 0;
        int j = 0;
        for (int i = 0; i < num_vertices*4; i+=12) {
            bool clip_vertices = false;
            // three triangles vertices
            glm::vec4 coords1 = glm::vec4(model_coefficients[i   ],
                                          model_coefficients[i+1 ],
                                          model_coefficients[i+2 ],
                                          model_coefficients[i+3 ]);
            glm::vec4 coords2 = glm::vec4(model_coefficients[i+4 ],
                                          model_coefficients[i+5 ],
                                          model_coefficients[i+6 ],
                                          model_coefficients[i+7 ]);
            glm::vec4 coords3 = glm::vec4(model_coefficients[i+8 ],
                                          model_coefficients[i+9 ],
                                          model_coefficients[i+10],
                                          model_coefficients[i+11]);  
            glm::vec4 coords1world = g_ModelMatrix * coords1;
            glm::vec4 coords2world = g_ModelMatrix * coords2;
            glm::vec4 coords3world = g_ModelMatrix * coords3;
            
            glm::vec4 normalCoords1 = glm::vec4(normal_coefficients[i   ],
                                                normal_coefficients[i+1 ],
                                                normal_coefficients[i+2 ],
                                                normal_coefficients[i+3 ]);
            glm::vec4 normalCoords2 = glm::vec4(normal_coefficients[i+4 ],
                                                normal_coefficients[i+5 ],
                                                normal_coefficients[i+6 ],
                                                normal_coefficients[i+7 ]);
            glm::vec4 normalCoords3 = glm::vec4(normal_coefficients[i+8 ],
                                                normal_coefficients[i+9 ],
                                                normal_coefficients[i+10],
                                                normal_coefficients[i+11]);  
            
            coords1 = g_ProjectionMatrix * g_ViewMatrix * g_ModelMatrix * coords1;
            coords2 = g_ProjectionMatrix * g_ViewMatrix * g_ModelMatrix * coords2;
            coords3 = g_ProjectionMatrix * g_ViewMatrix * g_ModelMatrix * coords3;
            
            // clip if w <=  0
            if (coords1.w <= 0 || coords2.w <= 0 || coords3.w <= 0) {
                clipped_vertices += 3;
            } else {
                // division by w
                coords1.x /= coords1.w;
                coords1.y /= coords1.w;
                coords1.z /= coords1.w;
                coords1.w /= coords1.w;
                coords2.x /= coords2.w;
                coords2.y /= coords2.w;
                coords2.z /= coords2.w;
                coords2.w /= coords2.w;
                coords3.x /= coords3.w;
                coords3.y /= coords3.w;
                coords3.z /= coords3.w;
                coords3.w /= coords3.w;
                // clip if z outside (-1, 1)
                if ( coords1.z < -1 || coords1.z > 1 ||
                     coords2.z < -1 || coords2.z > 1 ||
                     coords3.z < -1 || coords3.z > 1 ) {
                    clipped_vertices += 3;
                } else {
                   
                    // calculate screen coordinates for backface culling
                    glm::mat4 viewport = Matrix_Viewport(0.0f, (float)g_ScreenWidth, (float)g_ScreenHeight, 0.0f);
                    glm::vec4 coords1sc = viewport * coords1;
                    glm::vec4 coords2sc = viewport * coords2;
                    glm::vec4 coords3sc = viewport * coords3;
         
                    // backface culling
                    float area = 0;
                    float sum  = 0;
                    sum += (coords1sc.x*coords2sc.y - coords2sc.x*coords1sc.y);
                    sum += (coords2sc.x*coords3sc.y - coords3sc.x*coords2sc.y);
                    sum += (coords3sc.x*coords1sc.y - coords1sc.x*coords3sc.y);
                    area = 0.5f * sum;

                    // phong illumination model
                    glm::vec4 origin = glm::vec4(0.f,0.f,0.f,1.f);
                    glm::vec4 cameraPosition = glm::inverse(g_ViewMatrix) * origin;
                    glm::vec3 Kd = glm::vec3(1.f,1.f,1.f);
                    glm::vec3 Ks = glm::vec3(1.f,1.f,1.f);
                    glm::vec3 Ka = glm::vec3(.2f,.2f,.2f);
                    glm::vec3 Ia = glm::vec3(.2f,.2f,.2f);
                    float q = 32.f;
                    glm::vec4 lightDirection = glm::normalize(glm::vec4(1.f,1.f,0.f,0.f));
                    glm::vec3 ambientTerm = Ka * Ia;
                    glm::vec3 colorVector = glm::vec3(g_Red, g_Blue, g_Green);
                    
                    normalCoords1 = glm::normalize(normalCoords1);
                    normalCoords2 = glm::normalize(normalCoords2);
                    normalCoords3 = glm::normalize(normalCoords3);
                    
                    glm::vec4 viewDirectionV1 = glm::normalize(cameraPosition - coords1world);
                    glm::vec4 viewDirectionV2 = glm::normalize(cameraPosition - coords2world);
                    glm::vec4 viewDirectionV3 = glm::normalize(cameraPosition - coords3world);
                    
                    glm::vec4 reflectionDirectionV1 = -lightDirection + 2.f*normalCoords1*glm::dot(normalCoords1,lightDirection);
                    glm::vec4 reflectionDirectionV2 = -lightDirection + 2.f*normalCoords2*glm::dot(normalCoords2,lightDirection);
                    glm::vec4 reflectionDirectionV3 = -lightDirection + 2.f*normalCoords3*glm::dot(normalCoords3,lightDirection);
                    
                    glm::vec3 lambertDiffuseTermV1 = Kd*glm::max(0.f,glm::dot(normalCoords1,lightDirection));
                    glm::vec3 lambertDiffuseTermV2 = Kd*glm::max(0.f,glm::dot(normalCoords2,lightDirection));
                    glm::vec3 lambertDiffuseTermV3 = Kd*glm::max(0.f,glm::dot(normalCoords3,lightDirection));

                    glm::vec3 phongSpecularTermV1 = Ks*glm::pow(glm::max(0.f,dot(reflectionDirectionV1,viewDirectionV1)),q);
                    glm::vec3 phongSpecularTermV2 = Ks*glm::pow(glm::max(0.f,dot(reflectionDirectionV2,viewDirectionV2)),q);
                    glm::vec3 phongSpecularTermV3 = Ks*glm::pow(glm::max(0.f,dot(reflectionDirectionV3,viewDirectionV3)),q);
                    
                    glm::vec3 outputColorV1 = colorVector;
                    glm::vec3 outputColorV2 = colorVector;
                    glm::vec3 outputColorV3 = colorVector;
                    if (g_ToggleGouraud && g_TogglePhong) {
                        outputColorV1 = (ambientTerm+lambertDiffuseTermV1+phongSpecularTermV1)*colorVector;
                        outputColorV1 = glm::pow(outputColorV1, glm::vec3(1.f,1.f,1.f)/2.2f);
                        outputColorV2 = (ambientTerm+lambertDiffuseTermV2+phongSpecularTermV2)*colorVector;
                        outputColorV2 = glm::pow(outputColorV2, glm::vec3(1.f,1.f,1.f)/2.2f);
                        outputColorV3 = (ambientTerm+lambertDiffuseTermV3+phongSpecularTermV3)*colorVector;
                        outputColorV3 = glm::pow(outputColorV3, glm::vec3(1.f,1.f,1.f)/2.2f);
                    } else if (g_ToggleGouraud) {
                        outputColorV1 = (ambientTerm+lambertDiffuseTermV1)*colorVector;
                        outputColorV1 = glm::pow(outputColorV1, glm::vec3(1.f,1.f,1.f)/2.2f);
                        outputColorV2 = (ambientTerm+lambertDiffuseTermV2)*colorVector;
                        outputColorV2 = glm::pow(outputColorV2, glm::vec3(1.f,1.f,1.f)/2.2f);
                        outputColorV3 = (ambientTerm+lambertDiffuseTermV3)*colorVector;
                        outputColorV3 = glm::pow(outputColorV3, glm::vec3(1.f,1.f,1.f)/2.2f);
                    }


                    if (g_ToggleCW) { // clockwise
                        if (area < 0) {
                            // cull
                            clipped_vertices += 3;
                        } else {
                            DrawTriangle(coords1sc, coords2sc, coords3sc, outputColorV1, outputColorV2, outputColorV3);
                        }
                    } else { // counterclockwise
                        if (area > 0) {
                            // cull
                            clipped_vertices += 3;
                        } else {
                            DrawTriangle(coords1sc, coords2sc, coords3sc, outputColorV1, outputColorV2, outputColorV3);
                        }
                    }
                }
            }
        }
        
        
        std::vector<unsigned char> textureData;
        for (int i = 0; i < g_ScreenHeight; i++) {
            for (int j = 0; j < g_ScreenWidth; j++) {
                textureData.push_back(g_ColorBuffer.pixels[get_index(g_ColorBuffer, j, i)].r);
                textureData.push_back(g_ColorBuffer.pixels[get_index(g_ColorBuffer, j, i)].g);
                textureData.push_back(g_ColorBuffer.pixels[get_index(g_ColorBuffer, j, i)].b);
                textureData.push_back(g_ColorBuffer.pixels[get_index(g_ColorBuffer, j, i)].a);
            }
        }
        LoadTexture(textureData.data(), g_ScreenWidth, g_ScreenHeight);
        
        GLfloat vertex_data[] = {
            -1.f, -1.f, 1.f, 1.f, // bottom left
             1.f,  1.f, 1.f, 1.f, // top right
            -1.f,  1.f,-1.f, 1.f, // top left
             1.f,  1.f, 1.f, 1.f, // top left
            -1.f, -1.f, 1.f, 1.f, // bottom left
             1.f, -1.f, -1.f, 1.f // bottom right
        };
        GLfloat texture_coefficients[] = {
            1.f, 0.f, // bottom left
            0.f, 1.f, // top right
            1.f, 1.f, // top left
            0.f, 1.f, // top left
            1.f, 0.f, // bottom left
            0.f, 0.f  // bottom right
        };
        num_vertices = 6;
        
        for (int i = 0; i < 24; i++) {
            close2gl_coefficients[i] = vertex_data[i];
        }


        GLuint VBO_model_coefficients_id;
        glGenBuffers(1, &VBO_model_coefficients_id);

        GLuint vertex_array_object_id;
        glGenVertexArrays(1, &vertex_array_object_id);
        glBindVertexArray(vertex_array_object_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_model_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertex_data), vertex_data);

        GLuint location = 0;
        GLuint number_of_dimensions = 4;
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        
        GLuint indices[num_vertices];
        for (int i = 0; i < num_vertices; i++) {
            indices[i] = i;
        }

        SceneObject sceneModel;
        sceneModel.name           = "model";
        sceneModel.first_index    = (void*)0; 
      //  sceneModel.num_indices    =  12;
        sceneModel.num_indices    = model.num_triangles * 3;
        sceneModel.rendering_mode = GL_TRIANGLES; 
        sceneModel.min_coord      = min_coord;
        sceneModel.max_coord      = max_coord;
        g_VirtualScene["model"] = sceneModel;

        GLuint indices_id;
        glGenBuffers(1, &indices_id);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_id);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(indices), indices);

        GLuint VBO_texture_coefficients_id;
        glGenBuffers(1, &VBO_texture_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_texture_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, sizeof(texture_coefficients), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(texture_coefficients), texture_coefficients);
        location = 3;
        number_of_dimensions = 2;
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glBindVertexArray(0);
        return vertex_array_object_id;
    } 
    else {
        GLuint VBO_model_coefficients_id;
        glGenBuffers(1, &VBO_model_coefficients_id);

        GLuint vertex_array_object_id;
        glGenVertexArrays(1, &vertex_array_object_id);
        glBindVertexArray(vertex_array_object_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_model_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, model_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, model_coefficients.size() * sizeof(float), model_coefficients.data());

        GLuint location = 0;
        GLint number_of_dimensions = 4;
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        
        GLuint indices[num_vertices];
        for (int i = 0; i < num_vertices; i++) {
            indices[i] = i;
        }

        SceneObject sceneModel;
        sceneModel.name           = "model";
        sceneModel.first_index    = (void*)0; 
        sceneModel.num_indices    =  model.num_triangles * 3;
        if (g_TogglePoints) {
            sceneModel.rendering_mode = GL_POINTS;
        } else if (g_ToggleWireframe) {
            sceneModel.rendering_mode = GL_TRIANGLES;
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        } else if (g_ToggleSolid) {
            sceneModel.rendering_mode = GL_TRIANGLES;
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
        sceneModel.min_coord      = min_coord;
        sceneModel.max_coord      = max_coord;
        g_VirtualScene["model"] = sceneModel;

        GLuint indices_id;
        glGenBuffers(1, &indices_id);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_id);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(indices), indices);

 
        GLuint VBO_normal_coefficients_id;
        glGenBuffers(1, &VBO_normal_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_normal_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, normal_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, normal_coefficients.size() * sizeof(float), normal_coefficients.data());
        location = 2;
        number_of_dimensions = 4;
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);


        glBindVertexArray(0);
        return vertex_array_object_id;
    }
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
        } else if (action == GLFW_RELEASE) {
            g_W_pressed = false;
        }
    }

    if (key == GLFW_KEY_A) {
        if (action == GLFW_PRESS) {
            g_A_pressed = true;
        } else if (action == GLFW_RELEASE) {
            g_A_pressed = false;
        }
    }

    if (key == GLFW_KEY_S) {
        if (action == GLFW_PRESS) {
            g_S_pressed = true;
        } else if (action == GLFW_RELEASE) {
            g_S_pressed = false;
        }
    }

    if (key == GLFW_KEY_D) {
        if (action == GLFW_PRESS) {
            g_D_pressed = true;
        } else if (action == GLFW_RELEASE) {
            g_D_pressed = false;
        }
    }

    if (key == GLFW_KEY_Q) {
        if (action == GLFW_PRESS) {
            g_Q_pressed = true;
        } else if (action == GLFW_RELEASE) {
            g_Q_pressed = false;
        }
    }

    if (key == GLFW_KEY_Z) {
        if (action == GLFW_PRESS) {
            g_Z_pressed = true;
        } else if (action == GLFW_RELEASE) {
            g_Z_pressed = false;
        }
    }

    if (key == GLFW_KEY_R) {
        if (action == GLFW_PRESS) {
            g_CameraTheta = 0.0f;
            g_CameraPhi = 0.0f;
            g_CameraDistance = 5.0f;
            g_ResetCamera = true;
            g_NearPlane = -0.1f;
            g_FarPlane = -100.0f;
            g_hFov  = glm::radians(50.0f);
            g_vFov  = glm::radians(37.5f);
            SetWindowTextW(w_NearPlaneBox, L"0.1");
            SetWindowTextW(w_FarPlaneBox, L"100.0");
            SetWindowTextW(w_HorizontalFOV, L"50.0");
            SetWindowTextW(w_VerticalFOV, L"37.5");
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
    float dx = xpos - g_LastCursorPosX;
    float dy = ypos - g_LastCursorPosY;

    g_CameraTheta -= 0.01f*dx;
    g_CameraPhi   += 0.01f*dy;

    float phimax = 3.141592f/2;
    float phimin = -phimax;

    if (g_CameraPhi > phimax)
        g_CameraPhi = phimax;

    if (g_CameraPhi < phimin)
        g_CameraPhi = phimin;

    g_LastCursorPosX = xpos;
    g_LastCursorPosY = ypos;
}

void FramebufferSizeCallback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);
    g_ScreenRatio  = (float)width / height;
    g_ScreenWidth  = width;
    g_ScreenHeight = height;
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

int OpenFile(HWND hWnd)
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
    switch (msg) {
      case WM_COMMAND: {
        switch (wp) {
          case PROC_INFO_MENU: {
            MessageBox(NULL, "CMP143", "About", MB_OK);
            break;
          }
          case PROC_EXIT_MENU: {
            int val = MessageBoxW(NULL, L"Exit?", L"", MB_YESNO | MB_ICONEXCLAMATION);
            if (val == IDYES) {
                exit(0);
            }
            break;
          }
          case PROC_OPEN_FILE: {
            if (OpenFile(hWnd)) {
                FILE *file;
                if (!(file = fopen(g_ModelFilename, "r"))) {
                    MessageBox(NULL, "Arquivo nao encontrado!", "ERRO", MB_OK);
                    return -1;
                }
                g_Model = ReadModelFile(g_ModelFilename);
                g_VertexArrayObject_id = BuildTriangles(g_Model);
            }
            break;
          }
          case PROC_TOGGLE_CW: {
            int checkedState = SendMessageW(w_ToggleCW, BM_GETCHECK, 0, 0);
            if (checkedState == BST_CHECKED) {
                if (!g_UseClose2GL) {
                    glFrontFace(GL_CW);
                }
                g_ToggleCW = true;
            } else if (checkedState == BST_UNCHECKED) {
                if (!g_UseClose2GL) {
                    glFrontFace(GL_CCW);
                }
                g_ToggleCW = false;
            }
            break;
          }
          case PROC_NEARPLANE: {
            wchar_t ws_nearplane[BUFFER_SIZE] = { 0 };
            char s_nearplane[BUFFER_SIZE] = { 0 };
            float nearplane = 0;
            GetWindowTextW(w_NearPlaneBox, ws_nearplane, BUFFER_SIZE);
            std::wcstombs(s_nearplane, ws_nearplane, BUFFER_SIZE);
            nearplane = atof(s_nearplane);
            g_NearPlane = -nearplane;
            break;
          }
          case PROC_FARPLANE: {
            wchar_t ws_farplane[BUFFER_SIZE] = { 0 };
            char s_farplane[BUFFER_SIZE] = { 0 };
            float farplane = 0;
            GetWindowTextW(w_FarPlaneBox, ws_farplane, BUFFER_SIZE);
            std::wcstombs(s_farplane, ws_farplane, BUFFER_SIZE);
            farplane = atof(s_farplane);
            g_FarPlane = -farplane;
            break;
          }
          case PROC_RESET_CAMERA: {
            g_CameraTheta = 0.0f;
            g_CameraPhi = 0.0f;
            g_CameraDistance = 5.0f;
            g_ResetCamera = true;
            g_NearPlane = -0.1f;
            g_FarPlane = -100.0f;
            g_hFov  = glm::radians(50.0f);
            g_vFov  = glm::radians(37.5f);
            SetWindowTextW(w_NearPlaneBox, L"0.1");
            SetWindowTextW(w_FarPlaneBox, L"100.0");
            SetWindowTextW(w_HorizontalFOV, L"50.0");
            SetWindowTextW(w_VerticalFOV, L"37.5");
            break;
          }
          case PROC_CHANGE_COLOUR: {
            wchar_t ws_r[BUFFER_SIZE] = { 0 };
            wchar_t ws_g[BUFFER_SIZE] = { 0 };
            wchar_t ws_b[BUFFER_SIZE] = { 0 };
            char s_r[BUFFER_SIZE] = { 0 };
            char s_g[BUFFER_SIZE] = { 0 };
            char s_b[BUFFER_SIZE] = { 0 };
            int r = 0; int g = 0; int b = 0;
            float red = 0; float green = 0; float blue = 0;
            GetWindowTextW(w_RedBox,   ws_r, BUFFER_SIZE);
            GetWindowTextW(w_GreenBox, ws_g, BUFFER_SIZE);
            GetWindowTextW(w_BlueBox,  ws_b, BUFFER_SIZE);
            std::wcstombs(s_r, ws_r, BUFFER_SIZE);
            std::wcstombs(s_g, ws_g, BUFFER_SIZE);
            std::wcstombs(s_b, ws_b, BUFFER_SIZE);
            r = atoi(s_r); g = atoi(s_g); b = atoi(s_b);
            red   = (float) r/255;
            green = (float) g/255;
            blue  = (float) b/255;

            if (red > 1.0f) {
                SetWindowTextW(w_RedBox, L"255");
                g_Red = 1.0f;
            } else if (red < 0.0f) {
                SetWindowTextW(w_RedBox, L"0");
                g_Red = 0.0f;
            } else {
                g_Red = red;
            }
            if (green > 1.0f) {
                SetWindowTextW(w_GreenBox, L"255");
                g_Green = 1.0f;
            } else if (green < 0.0f) {
                SetWindowTextW(w_GreenBox, L"0");
                g_Green = 0.0f;
            } else {
                g_Green = green;
            }
            if (blue > 1.0f) {
                SetWindowTextW(w_BlueBox, L"255");
                g_Blue = 1.0f;
            } else if (blue < 0.0f) {
                SetWindowTextW(w_BlueBox, L"0");
                g_Blue = 0.0f;
            } else {
                g_Blue = blue;
            }

            break;
          }
          case PROC_LOOKAT_CAMERA: {
            SendMessageW(w_LookAtCheckbox, BM_SETCHECK, !g_LookAtCamera, 0);
            int checkedState = SendMessageW(w_LookAtCheckbox, BM_GETCHECK, 0, 0);
            if (checkedState == BST_CHECKED) {
                g_LookAtCamera = true;
            } else {
                g_LookAtCamera = false;
            }
            break;
          }
          case PROC_TOGGLE_GL: {
            int checkedState = SendMessageW(w_ToggleGL, BM_GETCHECK, 0, 0);
            if (checkedState == BST_CHECKED) {
                g_UseClose2GL = false;
                if (g_ToggleCW) {
                    glFrontFace(GL_CW);
                } else {
                    glFrontFace(GL_CCW);
                }
                printf("usando openGL\n");
            } else if (checkedState == BST_UNCHECKED) {
                g_UseClose2GL = true;
                glFrontFace(GL_CCW);
                g_VirtualScene["model"].rendering_mode = GL_TRIANGLES;
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                printf("usando close2GL\n");
            }
            g_VertexArrayObject_id = BuildTriangles(g_Model);
            glUniform1i(g_UseClose2GLLocation, g_UseClose2GL);
            break;
          }
          case PROC_TOGGLE_SOLID: {
            int points    = SendMessageW(w_TogglePoints,    BM_GETCHECK, 0, 0);
            int wireframe = SendMessageW(w_ToggleWireframe, BM_GETCHECK, 0, 0); 
            int solid     = SendMessageW(w_ToggleSolid,     BM_GETCHECK, 0, 0);
            if (points == BST_CHECKED) {
                g_TogglePoints = true;
                if (!g_UseClose2GL) {
                    g_VirtualScene["model"].rendering_mode = GL_POINTS;
                }
            } else if (wireframe == BST_CHECKED) {
                g_TogglePoints = false;
                g_ToggleWireframe = true;
                g_ToggleSolid = false;
                if (!g_UseClose2GL) {
                    g_VirtualScene["model"].rendering_mode = GL_TRIANGLES;
                    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                }
            } else if (solid == BST_CHECKED) {
                g_TogglePoints = false;
                g_ToggleWireframe = false;
                g_ToggleSolid = true;
                if (!g_UseClose2GL) {
                    g_VirtualScene["model"].rendering_mode = GL_TRIANGLES;
                    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                }
            }
            break;
          }
          case PROC_SET_HFOV: {
            wchar_t ws_hfov[BUFFER_SIZE] = { 0 };
            char s_hfov[BUFFER_SIZE] = { 0 };
            float hfov = 0;
            GetWindowTextW(w_HorizontalFOV, ws_hfov, BUFFER_SIZE);
            std::wcstombs(s_hfov, ws_hfov, BUFFER_SIZE);
            hfov = atof(s_hfov);
            g_hFov = glm::radians(hfov);
            break;
          }
          case PROC_SET_VFOV: {
            wchar_t ws_vfov[BUFFER_SIZE] = { 0 };
            char s_vfov[BUFFER_SIZE] = { 0 };
            float vfov = 0;
            GetWindowTextW(w_VerticalFOV, ws_vfov, BUFFER_SIZE);
            std::wcstombs(s_vfov, ws_vfov, BUFFER_SIZE);
            vfov = atof(s_vfov);
            g_vFov = glm::radians(vfov);
            break;
          }
          case PROC_RESET_WINDOW: {
            glfwSetWindowSize(g_GLWindow, 800, 600);
            g_ScreenRatio = 800.0f/600.0f;
          }
          case PROC_NO_SHADING: {
            g_ToggleGouraud = false;
            g_TogglePhong = false;
            g_VertexShaderType = 0;
            g_FragmentShaderType = 0;
            glUniform1i(g_VertexShaderTypeLocation, g_VertexShaderType);
            glUniform1i(g_FragmentShaderTypeLocation, g_FragmentShaderType);
            printf("No shading\n");
            break;
          }
          case PROC_GOURAUD_AD: {
            g_ToggleGouraud = true;
            g_TogglePhong = false;
            g_VertexShaderType = 1;  
            g_FragmentShaderType = 0;
            glUniform1i(g_VertexShaderTypeLocation, g_VertexShaderType);
            glUniform1i(g_FragmentShaderTypeLocation, g_FragmentShaderType);
            printf("Gouraud AD shading\n");
            break;
          }
          case PROC_GOURAUD_ADS: {
            g_ToggleGouraud = true;
            g_TogglePhong = true;
            g_VertexShaderType = 2;
            g_FragmentShaderType = 0;
            glUniform1i(g_VertexShaderTypeLocation, g_VertexShaderType);
            glUniform1i(g_FragmentShaderTypeLocation, g_FragmentShaderType);
            printf("Gouraud ADS shading\n");
            break;
          }
          case PROC_PHONG: {
            g_ToggleGouraud = false;
            g_TogglePhong = false;
            g_VertexShaderType = 0;
            g_FragmentShaderType = 1;
            glUniform1i(g_VertexShaderTypeLocation, g_VertexShaderType);
            glUniform1i(g_FragmentShaderTypeLocation, g_FragmentShaderType);
            printf("Phong shading\n");
            break;
          }
        }
        break;
      }
      case WM_DESTROY: {
        exit(0);
        break;
      }
      default: {
        return DefWindowProcW(hWnd, msg, wp, lp);
      }
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
        520,        // Button width
        25,        // Button height
        hWnd,     // Parent window
        (HMENU)PROC_OPEN_FILE, // procedure
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
        NULL);

    w_ToggleGL = CreateWindowW(
        L"BUTTON",
        L"OPEN GL",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP,
        20, 60,
        125, 25,
        hWnd,
        (HMENU)PROC_TOGGLE_GL,
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
        NULL);
    CreateWindowW(
        L"BUTTON",
        L"CLOSE2GL",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        145, 60,
        125, 25,
        hWnd,
        (HMENU)PROC_TOGGLE_GL,
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
        NULL);

    w_ToggleCW = CreateWindowW(
        L"BUTTON",
        L"CW",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP,
        20, 100,
        125, 25,
        hWnd,
        (HMENU)PROC_TOGGLE_CW,
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
        NULL);
    w_ToggleCCW = CreateWindowW(
        L"BUTTON",
        L"CCW",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        145, 100,
        125, 25,
        hWnd,
        (HMENU)PROC_TOGGLE_CCW,
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
        NULL);

    w_NearPlaneBox = CreateWindowW(
        L"EDIT", L"0.1",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | WS_BORDER | ES_RIGHT,
        20, 140,
        115, 25,
        hWnd,
        NULL,
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
        NULL);
    CreateWindowW(
        L"BUTTON", L"SET NEAR PLANE",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        145, 140,
        125, 25,
        hWnd,
        (HMENU)PROC_NEARPLANE,
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
        NULL);

    w_FarPlaneBox = CreateWindowW(
        L"EDIT", L"100.0",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | WS_BORDER | ES_RIGHT,
        20, 180,
        115, 25,
        hWnd,
        NULL,
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
        NULL);
    CreateWindowW(
        L"BUTTON", L"SET FAR PLANE",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        145, 180,
        125, 25,
        hWnd,
        (HMENU)PROC_FARPLANE,
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
        NULL);

    w_LookAtCheckbox = CreateWindowW(
        L"BUTTON", L"LOOK AT CAMERA",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_CHECKBOX,
        20, 220,
        250, 25,
        hWnd,
        (HMENU)PROC_LOOKAT_CAMERA, 
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
        NULL);

    CreateWindowW(
        L"BUTTON", L"RESET CAMERA",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        20, 260,
        250, 25,
        hWnd,
        (HMENU)PROC_RESET_CAMERA, 
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
        NULL);

    w_RedBox = CreateWindowW(
        L"EDIT", L"128",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | WS_BORDER | ES_RIGHT,
        70, 300,
        30,25,
        hWnd,
        NULL,
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
        NULL);
    CreateWindowW(
        L"STATIC", L"R",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD,
        105, 305,
        10, 25,
        hWnd,
        NULL,
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
        NULL);

    w_GreenBox = CreateWindowW(
        L"EDIT", L"128",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | WS_BORDER | ES_RIGHT,
        120, 300,
        30,25,
        hWnd,
        NULL,
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
        NULL);
    CreateWindowW(
        L"STATIC", L"G",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD,
        155, 305,
        10, 25,
        hWnd,
        NULL,
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
        NULL);

    w_BlueBox = CreateWindowW(
        L"EDIT", L"128",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | WS_BORDER | ES_RIGHT,
        170, 300,
        30,25,
        hWnd,
        NULL,
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
        NULL);
    CreateWindowW(
        L"STATIC", L"B",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD,
        205, 305,
        10, 25,
        hWnd,
        NULL,
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
        NULL);

    CreateWindowW(
        L"BUTTON", L"CHANGE MODEL COLOUR",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        20, 330,
        250, 25,
        hWnd,
        (HMENU)PROC_CHANGE_COLOUR, 
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
        NULL);

    w_TogglePoints = CreateWindowW(
        L"BUTTON",
        L"POINTS",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP,
        290, 60,
        80, 25,
        hWnd,
        (HMENU)PROC_TOGGLE_SOLID,
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
        NULL);

    w_ToggleWireframe = CreateWindowW(
        L"BUTTON",
        L"WIREFRAME",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        370, 60,
        100, 25,
        hWnd,
        (HMENU)PROC_TOGGLE_SOLID,
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
        NULL);

    w_ToggleSolid = CreateWindowW(
        L"BUTTON",
        L"SOLID",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        475, 60,
        80, 25,
        hWnd,
        (HMENU)PROC_TOGGLE_SOLID,
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
        NULL);

    w_HorizontalFOV = CreateWindowW(
        L"EDIT", L"50.0",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | WS_BORDER | ES_RIGHT,
        290, 100,
        115, 25,
        hWnd,
        NULL,
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
        NULL);
    CreateWindowW(
        L"BUTTON", L"SET H FOV",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        415, 100,
        125, 25,
        hWnd,
        (HMENU)PROC_SET_HFOV,
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
        NULL);

    w_VerticalFOV = CreateWindowW(
        L"EDIT", L"37.5",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | WS_BORDER | ES_RIGHT,
        290, 140,
        115, 25,
        hWnd,
        NULL,
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
        NULL);
    CreateWindowW(
        L"BUTTON", L"SET V FOV",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        415, 140,
        125, 25,
        hWnd,
        (HMENU)PROC_SET_VFOV,
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
        NULL);
        
    CreateWindowW(
        L"BUTTON", L"RESET WINDOW SIZE",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        290, 180,
        250, 25,
        hWnd,
        (HMENU)PROC_RESET_WINDOW,
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
        NULL);
        
    w_ToggleNoShading = CreateWindowW(
        L"BUTTON",
        L"NO SHADING",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP,
        290, 220,
        250, 25,
        hWnd,
        (HMENU)PROC_NO_SHADING,
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
        NULL);
        
    w_ToggleGouraudAD = CreateWindowW(
        L"BUTTON",
        L"GOURAUD AD",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        290, 260,
        250, 25,
        hWnd,
        (HMENU)PROC_GOURAUD_AD,
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
        NULL);
        
    w_ToggleGouraudADS = CreateWindowW(
        L"BUTTON",
        L"GOURAUD ADS",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        290, 300,
        250, 25,
        hWnd,
        (HMENU)PROC_GOURAUD_ADS,
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
        NULL);
        
    w_TogglePhong = CreateWindowW(
        L"BUTTON",
        L"PHONG",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        290, 340,
        250, 25,
        hWnd,
        (HMENU)PROC_PHONG,
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
        NULL);
}

void ShowFramesPerSecond()
{
    // Variáveis estáticas (static) mantém seus valores entre chamadas
    // subsequentes da função!
    static float old_seconds = (float)glfwGetTime();
    static int   ellapsed_frames = 0;
    static char  buffer[20] = "CMP143 - ?? fps";

    ellapsed_frames += 1;

    // Recuperamos o número de segundos que passou desde a execução do programa
    float seconds = (float)glfwGetTime();

    // Número de segundos desde o último cálculo do fps
    float ellapsed_seconds = seconds - old_seconds;

    if ( ellapsed_seconds > 1.0f )
    {
        snprintf(buffer, 20, "CMP143 - %.2f fps", ellapsed_frames / ellapsed_seconds);

        old_seconds = seconds;
        ellapsed_frames = 0;
    }
    glfwSetWindowTitle(g_GLWindow, buffer);
}
