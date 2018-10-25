#ifndef GLYUVWIDGET_H
#define GLYUVWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLBuffer>
#include <QTimer>

QT_FORWARD_DECLARE_CLASS(QOpenGLShaderProgram)
QT_FORWARD_DECLARE_CLASS(QOpenGLTexture)

class GLYuvWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    GLYuvWidget(QWidget *parent = 0);
    ~GLYuvWidget();

public:
    void setVideoSize(quint32 videoWidth, quint32 videoHeight);
    //显示一帧Yuv图像
    void updateTexture(quint8* bufferY, quint8* bufferU, quint8* bufferV, quint32 linesizeY, quint32 linesizeU, quint32 linesizeV);

protected:
    void initializeGL() Q_DECL_OVERRIDE;
    void paintGL() Q_DECL_OVERRIDE;
    void inittexture();

private:            
    QOpenGLShaderProgram *m_shaderProgram = Q_NULLPTR;  // 着色器程序
    QOpenGLBuffer m_vbo;    // 顶点缓冲对象

    //opengl中yuv纹理位置
    GLuint m_textureUniformY = 0;
    GLuint m_textureUniformU = 0;
    GLuint m_textureUniformV = 0;

    // yuv纹理对象
    QOpenGLTexture *m_textureY = Q_NULLPTR;
    QOpenGLTexture *m_textureU = Q_NULLPTR;
    QOpenGLTexture *m_textureV = Q_NULLPTR;

    // 纹理对象ID
    GLuint m_idY = 0;
    GLuint m_idU = 0;
    GLuint m_idV = 0;

    // 视频宽高
    quint32 m_videoWidth = 2160;
    quint32 m_videoHeight = 1080;

    quint8* m_bufferY = Q_NULLPTR;
    quint8* m_bufferU = Q_NULLPTR;
    quint8* m_bufferV = Q_NULLPTR;
    quint32 m_linesizeY = 0;
    quint32 m_linesizeU = 0;
    quint32 m_linesizeV = 0;
};

#endif // GLYUVWIDGET_H
