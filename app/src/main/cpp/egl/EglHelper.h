#ifndef NATIVEOPNEGLDEMO_EGLHELPER_H
#define NATIVEOPNEGLDEMO_EGLHELPER_H


#include "EGL/egl.h"
#include <EGL/egl.h>
#include "../log/JniLog.h"
#include "android/native_window.h"
#include "GLES2/gl2.h"

class EglHelper {


public:
    EGLDisplay mEglDisplay;
    EGLContext mEglContext;
    EGLSurface mEglSurface;
    // Handle to a program object
    GLuint programObject;
    // Sampler location
    GLuint samplerLoc;
public:
    EglHelper();

    ~EglHelper();

    int initEgl(EGLNativeWindowType surface);
    int initShaderAndProgram();
    int swapBuffers();
    void destroyEgl();
private:
    GLuint esLoadShader ( GLenum type, const char *shaderSrc );
};


#endif //NATIVEOPNEGLDEMO_EGLHELPER_H
