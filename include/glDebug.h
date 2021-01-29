#ifndef GL_DEBUG_H
#define GL_DEBUG_H

#include <glad/gl.h>

#ifdef __cplusplus
extern "C" {
#endif

void GLAPIENTRY glDebugOutput(GLenum source, GLenum type, unsigned int id, GLenum severity, GLsizei length, char const *message, void const *userParam);

#ifdef __cplusplus
}
#endif


#endif // !GL_DEBUG
