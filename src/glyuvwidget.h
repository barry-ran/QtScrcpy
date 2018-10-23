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

public slots:
    //显示一帧Yuv图像
    void slotShowYuv(QByteArray buffer, uint width, uint height);

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
    quint32 m_videoWidth;
    quint32 m_videoHeight;

    char *m_yuvPtr = Q_NULLPTR;
    QByteArray m_buffer;
};

#endif // GLYUVWIDGET_H
