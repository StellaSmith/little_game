#include <glDebug.h>

#include <stdio.h>

void GLAPIENTRY glDebugOutput(GLenum source, GLenum type, unsigned int id, GLenum severity, GLsizei length, char const *message, void const *user_data)
{

    (void)user_data;
    // ignore non-significant error/warning codes
    // if(id == 131169 || id == 131185 || id == 131218 || id == 131204) return;
    // if (id == 2) return;

    fputs("---------------\n", stderr);
    fprintf(stderr, "Debug message (%u): %.*s\n", id, length, message);

#if defined(GL_KHR_debug)
    switch (source) {
    case GL_DEBUG_SOURCE_API: fputs("Source: API", stderr); break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM: fputs("Source: Window System", stderr); break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER: fputs("Source: Shader Compiler", stderr); break;
    case GL_DEBUG_SOURCE_THIRD_PARTY: fputs("Source: Third Party", stderr); break;
    case GL_DEBUG_SOURCE_APPLICATION: fputs("Source: Application", stderr); break;
    case GL_DEBUG_SOURCE_OTHER: fputs("Source: Other", stderr); break;
    }
#else
    fputs("Source: Unknown", stderr);
#endif
    fputc('\n', stderr);

#if defined(GL_KHR_debug)
    switch (type) {
    case GL_DEBUG_TYPE_ERROR: fputs("Type: Error", stderr); break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: fputs("Type: Deprecated Behaviour", stderr); break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: fputs("Type: Undefined Behaviour", stderr); break;
    case GL_DEBUG_TYPE_PORTABILITY: fputs("Type: Portability", stderr); break;
    case GL_DEBUG_TYPE_PERFORMANCE: fputs("Type: Performance", stderr); break;
    case GL_DEBUG_TYPE_MARKER: fputs("Type: Marker", stderr); break;
    case GL_DEBUG_TYPE_PUSH_GROUP: fputs("Type: Push Group", stderr); break;
    case GL_DEBUG_TYPE_POP_GROUP: fputs("Type: Pop Group", stderr); break;
    case GL_DEBUG_TYPE_OTHER: fputs("Type: Other", stderr); break;
    }
#else
    fputs("Type: Unknown", stderr);
#endif
    fputc('\n', stderr);

#if defined(GL_KHR_debug)
    switch (severity) {
    case GL_DEBUG_SEVERITY_HIGH: fputs("Severity: high", stderr); break;
    case GL_DEBUG_SEVERITY_MEDIUM: fputs("Severity: medium", stderr); break;
    case GL_DEBUG_SEVERITY_LOW: fputs("Severity: low", stderr); break;
    case GL_DEBUG_SEVERITY_NOTIFICATION: fputs("Severity: notification", stderr); break;
    }
#else
    fputs("Severity: unknown", stderr);
#endif
    fputc('\n', stderr);
}
