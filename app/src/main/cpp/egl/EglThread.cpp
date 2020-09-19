#include <stdlib.h>
#include <stdio.h>
#include "EglThread.h"

///
// Create a simple 2x2 texture image with four different colors
//
GLuint CreateSimpleTexture2D( )
{
    // Texture object handle
    GLuint textureId;

    // 2x2 Image, 3 bytes per pixel (R, G, B)
    GLubyte pixels[4 * 3] =
            {
                    255,   0,   0, // Red
                    0, 255,   0, // Green
                    0,   0, 255, // Blue
                    255, 255,   0  // Yellow
            };

    // Use tightly packed data
    glPixelStorei ( GL_UNPACK_ALIGNMENT, 1 );

    // Generate a texture object
    glGenTextures ( 1, &textureId );

    // Bind the texture object
    glBindTexture ( GL_TEXTURE_2D, textureId );

    // Load the texture
    glTexImage2D ( GL_TEXTURE_2D, 0, GL_RGB, 2, 2, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels );

    // Set the filtering mode
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

    return textureId;
}

EglThread::EglThread() {
    pthread_mutex_init(&pthread_mutex, NULL);
    pthread_cond_init(&pthread_cond, NULL);


}

EglThread::~EglThread() {
    pthread_mutex_destroy(&pthread_mutex);
    pthread_cond_destroy(&pthread_cond);
}


void *eglThreadImpl(void *context) {
    EglThread *eglThread = static_cast<EglThread *>(context);
    if (!eglThread) {
        LOGE("eglThreadImpl eglThread is null");
        return 0;
    }

    EglHelper *eglHelper = new EglHelper();

    if (eglHelper->initEgl(eglThread->mANativeWindow) != 0 || eglHelper->initShaderAndProgram() != 0) {
        LOGE("eglHelper initEgl error");
        return 0;
    }
    eglThread->mTextureId = CreateSimpleTexture2D();
    eglThread->isExit = false;
    while (!eglThread->isExit) {

        if (eglThread->isCreate) {
            eglThread->isCreate = false;
            eglThread->onCreate();
        }

        if (eglThread->isChange) {
            eglThread->isChange = false;
            eglThread->isStart = true;
            eglThread->onChange(eglThread->surfaceWidth, eglThread->surfaceHeight);
        }

        if (eglThread->isStart) {
            eglThread->onDraw();
            eglThread->Draw(eglHelper);
            //切换缓冲区，显示
            eglHelper->swapBuffers();

            if (eglThread->mRenderType == RENDER_MODULE_AUTO) {
                usleep(1000000 / 60);
            } else {
                pthread_mutex_lock(&eglThread->pthread_mutex);
                pthread_cond_wait(&eglThread->pthread_cond, &eglThread->pthread_mutex);
                pthread_mutex_unlock(&eglThread->pthread_mutex);
            }
        }

    }

    eglHelper->destroyEgl();
    delete eglHelper;
    eglHelper = NULL;
    //return 0表示线程结束
    return 0;

}


void EglThread::onSurfaceCreate(EGLNativeWindowType window) {
    if (mEglThread == -1) {
        isCreate = true;
        mANativeWindow = window;
        //获取surface长宽信息
        surfaceWidth = ANativeWindow_getWidth(window);
        surfaceHeight = ANativeWindow_getHeight(window);
        LOGD("surface  width:%d ,height:%d",surfaceWidth,surfaceHeight);
        pthread_create(&mEglThread, NULL, eglThreadImpl, this);
    }
}


void EglThread::onSurfaceChange(int width, int height) {
    if (mEglThread != -1) {
        surfaceWidth = width;
        surfaceHeight = height;
        isChange = true;

        notifyRender();
    }
}

void EglThread::setRenderModule(int renderType) {
    mRenderType = renderType;
    notifyRender();
}

void EglThread::notifyRender() {
    pthread_mutex_lock(&pthread_mutex);
    pthread_cond_signal(&pthread_cond);
    pthread_mutex_unlock(&pthread_mutex);
}


void EglThread::callBackOnCreate(EglThread::OnCreate onCreate) {
    this->onCreate = onCreate;
}

void EglThread::callBackOnChange(EglThread::OnChange onChange) {
    this->onChange = onChange;
}

void EglThread::callBackOnDraw(EglThread::OnDraw onDraw) {
    this->onDraw = onDraw;
}

void EglThread::Draw(EglHelper *eglHelper) {
    GLfloat vVertices[] = { -0.5f,  0.5f, 0.0f,  // Position 0
                            0.0f,  0.0f,        // TexCoord 0
                            -0.5f, -0.5f, 0.0f,  // Position 1
                            0.0f,  1.0f,        // TexCoord 1
                            0.5f, -0.5f, 0.0f,  // Position 2
                            1.0f,  1.0f,        // TexCoord 2
                            0.5f,  0.5f, 0.0f,  // Position 3
                            1.0f,  0.0f         // TexCoord 3
    };
    GLushort indices[] = { 0, 1, 2, 0, 2, 3 };

    // Set the viewport
    glViewport ( 0, 0, surfaceWidth, surfaceHeight );

    // Clear the color buffer
    glClear ( GL_COLOR_BUFFER_BIT );

    // Use the program object
    glUseProgram ( eglHelper->programObject );

    // Load the vertex position
    glVertexAttribPointer ( 0, 3, GL_FLOAT,
                            GL_FALSE, 5 * sizeof ( GLfloat ), vVertices );
    // Load the texture coordinate
    glVertexAttribPointer ( 1, 2, GL_FLOAT,
                            GL_FALSE, 5 * sizeof ( GLfloat ), &vVertices[3] );

    glEnableVertexAttribArray ( 0 );
    glEnableVertexAttribArray ( 1 );

    // Bind the texture
    glActiveTexture ( GL_TEXTURE0 );
    //不变的纹理ID可以复用
    //glBindTexture ( GL_TEXTURE_2D, CreateSimpleTexture2D());
    glBindTexture ( GL_TEXTURE_2D, mTextureId);

    // Set the sampler texture unit to 0
    glUniform1i ( eglHelper->samplerLoc, 0 );

    glDrawElements ( GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices );
}