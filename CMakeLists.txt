cmake_minimum_required(VERSION 3.16)

project(wasmGL VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source list: avoid GLOB so adds/removes are explicit in version control
set(WASMGL_SOURCES
  src/main.cpp
  src/camera/orbit_camera.cpp
  src/core/color.cpp
  src/core/face3.cpp
  src/core/functions.cpp
  src/core/mesh.cpp
  src/primitives/box.cpp
  src/primitives/cylinder.cpp
  src/primitives/pyramid.cpp
  src/primitives/sphere.cpp
  src/primitives/torus.cpp
  src/shaders/shader.cpp
  src/threecsg/node.cpp
  src/threecsg/polygon.cpp
  src/threecsg/threebsp.cpp
  src/threecsg/vertex.cpp
  src/window/window.cpp
)

add_executable(wasmgl ${WASMGL_SOURCES})

target_include_directories(wasmgl PRIVATE
  ${CMAKE_SOURCE_DIR}/src/include
)

if(EMSCRIPTEN)
  set_target_properties(wasmgl PROPERTIES
    OUTPUT_NAME "wasmGL"
    SUFFIX ".js"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/wasmbuild"
  )
  # Keep in sync with src/include/wasmgl_exports.h
  # BSP/CSG recursion can run very deep on large meshes; default wasm stack is small and
  # overflows manifest as uncaught RuntimeError: memory access out of bounds.
  target_link_options(wasmgl PRIVATE
    "SHELL:-sUSE_GLFW=3"
    "SHELL:-sMIN_WEBGL_VERSION=2"
    "SHELL:-sSTACK_SIZE=33554432"
    "SHELL:-sALLOW_MEMORY_GROWTH=1"
    "SHELL:-sEXPORTED_FUNCTIONS=[_main,_addCube,_addSphere,_addPyramid,_addCylinder,_addTorus,_cameraZoomIn,_cameraZoomOut,_cameraNudgeViewYawDegrees,_cameraNudgeViewPitchDegrees,_cameraNudgePositionView,_getSceneObjectCount,_setSelectedObjectIndex,_getSelectedObjectIndex,_getObjectKind,_getSelectedBoxWidth,_getSelectedBoxHeight,_getSelectedBoxDepth,_getSelectedSphereRadius,_getSelectedSphereWidthSegments,_getSelectedSphereHeightSegments,_getSelectedPyramidSide,_getSelectedPyramidHeight,_getSelectedCylinderRadiusBottom,_getSelectedCylinderRadiusTop,_getSelectedCylinderHeight,_getSelectedCylinderRadialSegments,_getSelectedCylinderHeightSegments,_getSelectedTorusMajorRadius,_getSelectedTorusMinorRadius,_getSelectedTorusRadialSegments,_getSelectedTorusTubularSegments,_resizeSelectedBox,_resizeSelectedSphere,_resizeSelectedPyramid,_resizeSelectedCylinder,_resizeSelectedTorus,_nudgeSelectedTranslate,_nudgeSelectedRotateDegrees,_setBooleanOperandA,_setBooleanOperandB,_getBooleanOperandA,_getBooleanOperandB,_performBooleanUnion,_performBooleanDifference,_performBooleanIntersection]"
  )
else()
  find_package(OpenGL REQUIRED)
  find_package(PkgConfig QUIET)
  if(PkgConfig_FOUND)
    pkg_check_modules(PC_GLFW glfw3)
  endif()

  if(PC_GLFW_FOUND)
    target_include_directories(wasmgl PRIVATE ${PC_GLFW_INCLUDE_DIRS})
    target_compile_options(wasmgl PRIVATE ${PC_GLFW_CFLAGS_OTHER})
    target_link_libraries(wasmgl PRIVATE ${PC_GLFW_LIBRARIES})
  else()
    find_path(GLFW_INCLUDE_DIR NAMES GLFW/glfw3.h)
    find_library(GLFW_LIBRARY NAMES glfw glfw3)
    if(NOT GLFW_INCLUDE_DIR OR NOT GLFW_LIBRARY)
      message(FATAL_ERROR
        "Native build needs GLFW (e.g. sudo apt install libglfw3-dev) or pkg-config glfw3.")
    endif()
    target_include_directories(wasmgl PRIVATE ${GLFW_INCLUDE_DIR})
    target_link_libraries(wasmgl PRIVATE ${GLFW_LIBRARY})
  endif()

  find_package(GLEW QUIET)
  if(TARGET GLEW::GLEW)
    target_link_libraries(wasmgl PRIVATE GLEW::GLEW)
  else()
    find_library(GLEW_LIBRARY NAMES GLEW REQUIRED)
    find_path(GLEW_INCLUDE_DIR NAMES GL/glew.h REQUIRED)
    target_include_directories(wasmgl PRIVATE ${GLEW_INCLUDE_DIR})
    target_link_libraries(wasmgl PRIVATE ${GLEW_LIBRARY})
  endif()

  target_link_libraries(wasmgl PRIVATE OpenGL::GL)
endif()
