#ifndef QYUVOPENGLWIDGET_H
#define QYUVOPENGLWIDGET_H
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>

//QT_FORWARD_DECLARE_CLASS(QOpenGLTexture) //TODO先不用QOpenGLTexture，Texture先使用opengl原生方式
class QYUVOpenGLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    explicit QYUVOpenGLWidget(QWidget *parent = nullptr);
    virtual ~QYUVOpenGLWidget();

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

    void setFrameSize(const QSize& frameSize);
    void updateTextures(quint8* dataY, quint8* dataU, quint8* dataV, quint32 linesizeY, quint32 linesizeU, quint32 linesizeV);

protected:
    void initializeGL();
    void paintGL();
    void resizeGL(int width, int height);

private:    
    void initShader();
    void initTextures();
    void deInitTextures();
    void updateTexture(GLuint texture, quint32 textureType, quint8* pixels, quint32 stride);

private:
    // 视频帧尺寸
    QSize m_frameSize = {-1, -1};
    bool m_needInit = false;

    // 顶点缓冲对象(Vertex Buffer Objects, VBO)：默认即为VertexBuffer(GL_ARRAY_BUFFER)类型
    QOpenGLBuffer m_vbo;

    // 着色器程序：编译链接着色器
    QOpenGLShaderProgram m_shaderProgram;

    // YUV纹理，用于生成纹理贴图
    //QOpenGLTexture* m_textrueY = Q_NULLPTR;
    //QOpenGLTexture* m_textrueU = Q_NULLPTR;
    //QOpenGLTexture* m_textrueV = Q_NULLPTR;

    // YUV纹理，用于生成纹理贴图
    GLuint m_texture[3] = {0};
};

#endif // QYUVOPENGLWIDGET_H
