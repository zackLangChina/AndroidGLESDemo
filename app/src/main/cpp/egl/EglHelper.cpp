#include <stdlib.h>
#include "EglHelper.h"

EglHelper::EglHelper() {
    mEglDisplay = EGL_NO_DISPLAY;
    mEglSurface = EGL_NO_SURFACE;
    mEglContext = EGL_NO_CONTEXT;
}

EglHelper::~EglHelper(){

}

int EglHelper::initEgl(EGLNativeWindowType window) {
    //1.得到默认的显示设备（就是窗口） -- eglGetDisplay
    mEglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (mEglDisplay == EGL_NO_DISPLAY) {
        LOGE("eglGetDisplay error");
        return -1;
    }

    //2. 初始化默认显示设备 -- eglInitialize
    EGLint *version = new EGLint[2];
    if (!eglInitialize(mEglDisplay, &version[0], &version[1])) {
        LOGE("eglInitialize error");
        return -1;
    }

    //3. 设置显示设备的属性
    const EGLint attrib_config_list[] = {
            EGL_RED_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_BLUE_SIZE, 8,
            EGL_ALPHA_SIZE, 8,
            EGL_DEPTH_SIZE, 8,
            EGL_STENCIL_SIZE, 8,// 眼睛屏幕的距离
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,//版本号
            EGL_NONE
    };

    //3.1 根据所需的参数获取符合该参数的config_size，主要是解决有些手机eglChooseConfig失败的兼容性问题
    EGLint num_config;

    if (!eglChooseConfig(mEglDisplay, attrib_config_list, NULL, 1, &num_config)) {
        LOGE("eglChooseConfig error");
        return -1;
    }
    //3.2 根据获取到的config_size得到eglConfig
    EGLConfig eglConfig;
    if (!eglChooseConfig(mEglDisplay, attrib_config_list, &eglConfig, num_config, &num_config)) {
        LOGE("eglChooseConfig error");
        return -1;
    }

    //4. 创建egl上下文 eglCreateContext
    const EGLint attrib_ctx_list[] = {
            EGL_CONTEXT_CLIENT_VERSION, 2,
            EGL_NONE
    };
    mEglContext = eglCreateContext(mEglDisplay, eglConfig, NULL, attrib_ctx_list);
    if (mEglContext == EGL_NO_CONTEXT) {
        LOGE("eglCreateContext  error");
        return -1;
    }
    //5.创建渲染的surface
    mEglSurface = eglCreateWindowSurface(mEglDisplay, eglConfig, window, NULL);
    if (mEglSurface == EGL_NO_SURFACE) {
        LOGE("eglCreateWindowSurface  error");
        return -1;
    }

    //6. 绑定eglContext和surface到display
    if (!eglMakeCurrent(mEglDisplay, mEglSurface, mEglSurface, mEglContext)) {
        LOGE("eglMakeCurrent  error");
        return -1;
    }

    //7. 刷新数据，显示渲染场景 -- eglSwapBuffers

    return 0;
}

int EglHelper::swapBuffers() {
    if (mEglDisplay != EGL_NO_DISPLAY && mEglSurface != EGL_NO_SURFACE &&
        eglSwapBuffers(mEglDisplay, mEglSurface)) {
        return 0;
    }
    return -1;
}

void EglHelper::destroyEgl() {
    if (mEglDisplay != EGL_NO_DISPLAY) {
        //解绑display上的eglContext和surface
        eglMakeCurrent(mEglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

        //销毁surface 和 eglContext
        if (mEglSurface != EGL_NO_SURFACE) {
            eglDestroySurface(mEglDisplay, mEglSurface);
            mEglSurface = EGL_NO_SURFACE;
        }

        if (mEglContext != EGL_NO_CONTEXT) {
            eglDestroyContext(mEglDisplay, mEglContext);
            mEglContext = EGL_NO_CONTEXT;
        }

        if (mEglDisplay != EGL_NO_DISPLAY) {
            eglTerminate(mEglDisplay);
            mEglDisplay = EGL_NO_DISPLAY;
        }
    }
}

GLuint EglHelper::esLoadShader ( GLenum type, const char *shaderSrc )
{
    GLuint shader;
    GLint compiled;

    // Create the shader object
    shader = glCreateShader ( type );

    if ( shader == 0 )
    {
        return 0;
    }

    // Load the shader source
    glShaderSource ( shader, 1, &shaderSrc, NULL );

    // Compile the shader
    glCompileShader ( shader );

    // Check the compile status
    glGetShaderiv ( shader, GL_COMPILE_STATUS, &compiled );

    if ( !compiled )
    {
        GLint infoLen = 0;

        glGetShaderiv ( shader, GL_INFO_LOG_LENGTH, &infoLen );

        if ( infoLen > 1 )
        {
            char *infoLog = (char*)malloc ( sizeof ( char ) * infoLen );

            glGetShaderInfoLog ( shader, infoLen, NULL, infoLog );
            //esLogMessage ( "Error compiling shader:\n%s\n", infoLog );

            free ( infoLog );
        }

        glDeleteShader ( shader );
        return 0;
    }

    return shader;

}

int EglHelper::initShaderAndProgram() {
    // Use the program object
    char vShaderStr[] =
            "#version 300 es                            \n"
            "layout(location = 0) in vec4 a_position;   \n"
            "layout(location = 1) in vec2 a_texCoord;   \n"
            "out vec2 v_texCoord;                       \n"
            "void main()                                \n"
            "{                                          \n"
            "   gl_Position = a_position;               \n"
            "   v_texCoord = a_texCoord;                \n"
            "}                                          \n";

    char fShaderStr[] =
            "#version 300 es                                     \n"
            "precision mediump float;                            \n"
            "in vec2 v_texCoord;                                 \n"
            "layout(location = 0) out vec4 outColor;             \n"
            "uniform sampler2D s_texture;                        \n"
            "void main()                                         \n"
            "{                                                   \n"
            "  outColor = texture( s_texture, v_texCoord );      \n"
            "}                                                   \n";
    GLuint vertexShader;
    GLuint fragmentShader;
    GLint linked;

    // Load the shaders and get a linked program object
    // Load the vertex/fragment shaders
    vertexShader = esLoadShader ( GL_VERTEX_SHADER, vShaderStr );

    if ( vertexShader == 0 )
    {
        return 0;
    }

    fragmentShader = esLoadShader ( GL_FRAGMENT_SHADER, fShaderStr );

    if ( fragmentShader == 0 )
    {
        glDeleteShader ( vertexShader );
        return 0;
    }

    // Create the program object
    programObject = glCreateProgram ( );

    if ( programObject == 0 )
    {
        return 0;
    }

    glAttachShader ( programObject, vertexShader );
    glAttachShader ( programObject, fragmentShader );

    // Link the program
    glLinkProgram ( programObject );

    // Check the link status
    glGetProgramiv ( programObject, GL_LINK_STATUS, &linked );

    if ( !linked )
    {
        GLint infoLen = 0;

        glGetProgramiv ( programObject, GL_INFO_LOG_LENGTH, &infoLen );

        if ( infoLen > 1 )
        {
            char *infoLog = (char*)malloc ( sizeof ( char ) * infoLen );

            glGetProgramInfoLog ( programObject, infoLen, NULL, infoLog );
            //esLogMessage ( "Error linking program:\n%s\n", infoLog );

            free ( infoLog );
        }

        glDeleteProgram ( programObject );
        return 0;
    }

    // Free up no longer needed shader resources
    glDeleteShader ( vertexShader );
    glDeleteShader ( fragmentShader );

    // Get the sampler location
    samplerLoc = glGetUniformLocation ( programObject, "s_texture" );
    return 0;
}