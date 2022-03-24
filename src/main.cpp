// emcc -std=c++17 *.cpp -I /user/include/ -s USE_GLFW=3 -s FULL_ES3=1 -s GL_ASSERTIONS=1 -s GL_DEBUG=1  -s WASM=1 -o main.html

#include <memory>
#include <stdio.h>
#include <string.h>
#include <cmath>
#include <vector>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "window/window.h"
#include "core/mesh.h"
#include "primitives/pyramid.h"
#include "primitives/cube.h"
#include "primitives/sphere.h"
#include "primitives/box.h"

#include "shaders/shader.h"

#include "camera/camera.h"
#include "complexobjects/csgmesh.h"
#include "threecsg/threebsp.h"

#include <chrono>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

const float toRadians = 3.14159265f / 180.0f;

Window mainWindow;
std::vector<std::shared_ptr<Mesh>> meshList;
std::vector<Shader> shaderList;
Camera camera;
GLfloat deltaTime{0.0f};
GLfloat lastTime{0.0f};
int loops{0};
GLfloat totaltime{0};

// Vertex Shader
const GLchar *vShader =
    "#version 300 es                                          \n"
    "out vec4 vCol;                                           \n"
    "uniform mat4 projection;                                 \n"
    "uniform mat4 model;                                      \n"
    "uniform mat4 view;                                       \n"
    "layout (location = 0) in vec3 pos;                       \n"
    "void main()                                              \n"
    "{                                                        \n"
    "    gl_Position = projection * view * model * vec4(pos, 1.0);   \n"
    "    vCol = vec4(clamp(pos, 0.0f, 1.0f), 1.0f);           \n"
    "}                                                        \n";

// Fragment Shader
static const char *fShader =
    "#version 300 es                              \n"
    "precision mediump float;                     \n"
    "in vec4 vCol;                                \n"
    "out vec4 color;                              \n"
    "void main()                                  \n"
    "{                                            \n"
    "    color = vCol;                            \n"
    "}                                            \n";

void CreateObjects()
{

  // std::shared_ptr<Pyramid> obj1 = std::make_shared<Pyramid>(PyramidDimensions{1.0f, 1.0f});
  // obj1->translate(glm::vec3(0.0f, 0.0f, -2.5f));
  // obj1->rotate(glm::vec3(0.0f, 0.0f, toRadians * 90.0f));

  // meshList.push_back(obj1);

  // std::shared_ptr<Cube> cube = std::make_shared<Cube>(CubeDimensions{1.0f});
  // std::shared_ptr<Box> box = std::make_shared<Box>(BoxDimensions{0.5f, 0.5f, 2.5f});
  std::shared_ptr<Sphere> sphere1 = std::make_shared<Sphere>(SphereDimensions{0.6f}, SphereParameters{8, 9, 0.0f, 2 * M_PI, 0.0f, M_PI});
  sphere1->translate(glm::vec3(0.2f, 0.0f, 0.0f));

  std::shared_ptr<Sphere> sphere2 = std::make_shared<Sphere>(SphereDimensions{0.6f}, SphereParameters{8, 9, 0.0f, 2 * M_PI, 0.0f, M_PI});
  sphere2->translate(glm::vec3(-0.2f, 0.0f, 0.0f));

  // meshList.push_back(sphere1);

  // meshList.push_back(cube);
  // meshList.push_back(box);

  // auto cubeTree = make_shared<ThreeBSP>(ThreeBSP(cube));
  // auto boxTree = make_shared<ThreeBSP>(ThreeBSP(box));

  auto startTime = std::chrono::high_resolution_clock::now();
  auto sphere1Tree = make_shared<ThreeBSP>(ThreeBSP(sphere1));
  auto end1 = std::chrono::high_resolution_clock::now();
  auto duration1 = std::chrono::duration_cast<std::chrono::milliseconds>(end1 - startTime).count();
  std::cout << "Sphere 1: " << duration1 << " ms" << std::endl;

  auto sphere2Tree = make_shared<ThreeBSP>(ThreeBSP(sphere2));
  auto csg = sphere1Tree->intersect(sphere2Tree);
  std::shared_ptr<CSGMesh> csgObj = csg->toMesh();
  auto endTime = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

  std::cout << "Time taken to create CSG: " << duration << "ms" << std::endl;

  meshList.push_back(csgObj);
}

void CreateShaders()
{
  Shader *shader1 = new Shader();
  shader1->CreateFromString(vShader, fShader);
  shaderList.push_back(*shader1);
}

void emcmainloop(void *mainLoopArg);
void mainloop(glm::mat4 &projection);
int main()
{

  mainWindow = Window(800, 600);
  mainWindow.Initialise();

  CreateObjects();
  CreateShaders();

  camera = Camera(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, 0.0f, 5.0f, 0.1f);

  glm::mat4 projection = glm::perspective(glm::radians(45.0f), (GLfloat)mainWindow.getBufferWidth() / mainWindow.getBufferHeight(), 0.1f, 100.0f);

#ifdef __EMSCRIPTEN__
  int fps = 0; // Use browser's requestAnimationFrame
  emscripten_set_main_loop_arg(emcmainloop, &projection, fps, true);
#else
  // Loop until window closed
  while (!mainWindow.getShouldClose())
  {
    mainloop(projection);
  }
#endif

  return 0;
}

void emcmainloop(void *mainLoopArg)
{
  mainloop(*(glm::mat4 *)mainLoopArg);
}

void mainloop(glm::mat4 &projection)
{
  // Calculate deltatime of current frame
  GLfloat currentTime = glfwGetTime(); // Time in seconds
  deltaTime = currentTime - lastTime;  // Time in seconds
  lastTime = currentTime;              // Set lastTime to currentTime for next frame
  totaltime += deltaTime;
  GLfloat meanTime = totaltime / loops;
  int fps = 1 / meanTime;
  loops++;
  // std::cout << "FPS: " << fps << std::endl;

  // std::cout << deltaTime << std::endl;
  GLuint uniformProjection = 0, uniformModel = 0, uniformView = 0;
  // Get + Handle User Input
  glfwPollEvents();
  camera.keyControl(mainWindow.getKeys(), deltaTime);
  camera.mouseControl(mainWindow.getXChange(), mainWindow.getYChange());

  // Clear the window
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  shaderList[0].UseShader();
  uniformModel = shaderList[0].GetModelLocation();
  uniformProjection = shaderList[0].GetProjectionLocation();
  uniformView = shaderList[0].GetViewLocation();

  // glUniformMatrix4fv(uniformModel, 1, GL_FALSE, meshList.at(0)->getModelPtr());
  glUniformMatrix4fv(uniformProjection, 1, GL_FALSE, glm::value_ptr(projection));

  for (auto mesh : meshList)
  {
    glUniformMatrix4fv(uniformModel, 1, GL_FALSE, mesh->getModelPtr());
    glUniformMatrix4fv(uniformView, 1, GL_FALSE, glm::value_ptr(camera.calculateViewMatrix()));
    mesh->RenderMesh();
  }

  glUseProgram(0);
  mainWindow.swapBuffers();
}
