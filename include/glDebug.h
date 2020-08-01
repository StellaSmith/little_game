#ifndef GL_DEBUG_H
#define GL_DEBUG_H

#include <glad/glad.h>

/*
https://www.khronos.org/registry/OpenGL/extensions/KHR/KHR_debug.txt
*/

/*
    New Procedures and Functions
*/

// NOTE: I have no idea what calling convention these should have, most likely APIENTRY.

typedef void (APIENTRY *PFN_glDebugMessageControl)(GLenum source,
    GLenum type,
    GLenum severity,
    GLsizei count,
    const GLuint* ids,
    GLboolean enabled);

typedef void (APIENTRY *PFN_glDebugMessageInsert)(GLenum source,
    GLenum type,
    GLuint id,
    GLenum severity,
    GLsizei length,
    const char* buf);

typedef void (APIENTRY *PFN_glDebugMessageCallback)(GLDEBUGPROC callback,
    const void* userParam);

typedef GLuint(APIENTRY *PFN_glGetDebugMessageLog)(GLuint count,
    GLsizei bufSize,
    GLenum* sources,
    GLenum* types,
    GLuint* ids,
    GLenum* severities,
    GLsizei* lengths,
    char* messageLog);

typedef void (APIENTRY *PFN_glGetPointerv)(GLenum pname,
    void** params);

typedef void (APIENTRY *PFN_glPushDebugGroup)(GLenum source, GLuint id, GLsizei length,
    const char* message);

typedef void (APIENTRY *PFN_glPopDebugGroup)(void);

typedef void (APIENTRY *PFN_glObjectLabel)(GLenum identifier, GLuint name, GLsizei length,
    const char* label);

typedef void (APIENTRY *PFN_glGetObjectLabel)(GLenum identifier, GLuint name, GLsizei bufSize,
    GLsizei* length, char* label);

typedef void (APIENTRY *PFN_glObjectPtrLabel)(void* ptr, GLsizei length,
    const char* label);

typedef void (APIENTRY *PFN_glGetObjectPtrLabel)(void* ptr, GLsizei bufSize,
    GLsizei* length, char* label);

/*
    New Types
*/

typedef void (APIENTRY* GLDEBUGPROC)(GLenum source,
    GLenum type,
    GLuint id,
    GLenum severity,
    GLsizei length,
    const GLchar* message,
    const void* userParam);


/*
    New Tokens
*/

// Tokens accepted by the <target> parameters of Enable, Disable, and
//IsEnabled
#define GL_DEBUG_OUTPUT                                     0x92E0
#define GL_DEBUG_OUTPUT_SYNCHRONOUS                         0x8242

// Returned by GetIntegerv when <pname> is CONTEXT_FLAGS
#define GL_CONTEXT_FLAG_DEBUG_BIT                           0x00000002

// Tokens accepted by the <value> parameters of GetBooleanv, GetIntegerv,
//GetFloatv, GetDoublevand GetInteger64v
#define GL_MAX_DEBUG_MESSAGE_LENGTH                         0x9143
#define GL_MAX_DEBUG_LOGGED_MESSAGES                        0x9144
#define GL_DEBUG_LOGGED_MESSAGES                            0x9145
#define GL_DEBUG_NEXT_LOGGED_MESSAGE_LENGTH                 0x8243
#define GL_MAX_DEBUG_GROUP_STACK_DEPTH                      0x826C
#define GL_DEBUG_GROUP_STACK_DEPTH                          0x826D
#define GL_MAX_LABEL_LENGTH                                 0x82E8

// Tokens accepted by the <pname> parameter of GetPointerv
#define GL_DEBUG_CALLBACK_FUNCTION                          0x8244
#define GL_DEBUG_CALLBACK_USER_PARAM                        0x8245

// Tokens accepted or provided by the <source> parameters of
// DebugMessageControl, DebugMessageInsertand DEBUGPROC, and the <sources>
// parameter of GetDebugMessageLog(some commands restrict <source> to a
// subset of these parameters; see the specification body for details)
#define GL_DEBUG_SOURCE_API                                 0x8246
#define GL_DEBUG_SOURCE_WINDOW_SYSTEM                       0x8247
#define GL_DEBUG_SOURCE_SHADER_COMPILER                     0x8248
#define GL_DEBUG_SOURCE_THIRD_PARTY                         0x8249
#define GL_DEBUG_SOURCE_APPLICATION                         0x824A
#define GL_DEBUG_SOURCE_OTHER                               0x824B

// Tokens accepted or provided by the <type> parameters of
// DebugMessageControl, DebugMessageInsertand DEBUGPROC, and the <types>
// parameter of GetDebugMessageLog
#define GL_DEBUG_TYPE_ERROR                                 0x824C
#define GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR                   0x824D
#define GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR                    0x824E
#define GL_DEBUG_TYPE_PORTABILITY                           0x824F
#define GL_DEBUG_TYPE_PERFORMANCE                           0x8250
#define GL_DEBUG_TYPE_OTHER                                 0x8251
#define GL_DEBUG_TYPE_MARKER                                0x8268

// Tokens accepted or provided by the <type> parameters of
// DebugMessageControland DEBUGPROC, and the <types> parameter of
// GetDebugMessageLog
#define GL_DEBUG_TYPE_PUSH_GROUP                            0x8269
#define GL_DEBUG_TYPE_POP_GROUP                             0x826A

// Tokens accepted or provided by the <severity> parameters of
// DebugMessageControl, DebugMessageInsertand DEBUGPROC callback functions,
// and the <severities> parameter of GetDebugMessageLog
#define GL_DEBUG_SEVERITY_HIGH                              0x9146
#define GL_DEBUG_SEVERITY_MEDIUM                            0x9147
#define GL_DEBUG_SEVERITY_LOW                               0x9148
#define GL_DEBUG_SEVERITY_NOTIFICATION                      0x826B

// Returned by GetError
#define GL_STACK_UNDERFLOW                                  0x0504
#define GL_STACK_OVERFLOW                                   0x0503

//Tokens accepted or provided by the <identifier> parameters of
//ObjectLabel and GetObjectLabel
#define GL_BUFFER                                           0x82E0
#define GL_SHADER                                           0x82E1
#define GL_PROGRAM                                          0x82E2
// #define GL_VERTEX_ARRAY
#define GL_QUERY                                            0x82E3
#define GL_PROGRAM_PIPELINE                                 0x82E4
// #define GL_TRANSFORM_FEEDBACK
#define GL_SAMPLER                                          0x82E6
// #define GL_TEXTURE
// #define GL_RENDERBUFFER
// #define GL_FRAMEBUFFER
// [[ Compatibility Profile ]]
#define GL_DISPLAY_LIST                                     0x82E7
// [[ End Profile - Specific Language ]]

// Accepted by the <pname> parameter of GetBooleanv, GetIntegerv,
// GetFloatv, GetDoublev, and GetInteger64v
// #define GL_MAX_LABEL_LENGTH

/*
    Custom stuff
*/

#ifdef __cplusplus
extern "C" {
#endif

void GLAPIENTRY glDebugOutput(GLenum source, GLenum type, unsigned int id, GLenum severity, GLsizei length, char const *message, void const *userParam);

#ifdef __cplusplus
}
#endif


#endif // !GL_DEBUG
