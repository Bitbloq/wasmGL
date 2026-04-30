#ifndef WASMGL_GL_H
#define WASMGL_GL_H

/* Native: GLEW after GLFW context. Wasm: GLES3 from Emscripten (no GLEW). */
#ifdef __EMSCRIPTEN__
#include <GLES3/gl3.h>
#else
#include <GL/glew.h>
#endif

#endif
