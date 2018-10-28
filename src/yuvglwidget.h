#ifndef YUVGLWIDGET_H
#define YUVGLWIDGET_H

#include <QOpenGLFunctions>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLWidget>
//#include <QSurfaceFormat>

class QOpenGLShaderProgram;
class YUVGLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    explicit YUVGLWidget(QWidget* parent = 0);
    ~YUVGLWidget();

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

    void setFrameSize(const QSize& frameSize);
    void updateTextures(quint8* dataY, quint8* dataU, quint8* dataV, quint32 linesizeY, quint32 linesizeU, quint32 linesizeV);

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int width, int height) override;

private:
    enum YUVTextureType {
        Texture_NULL = -1,
        Texture_Y,
        Texture_U,
        Texture_V,
        Texture_Size
    };

    void initShader();
    void initTexture(qint32 textureType);
    void initTextures();
    void deInitTextures();
    void calcTextureSize(qint32 textureType, QSize& size);

    void bindPixelTexture(GLuint texture, YUVTextureType textureType, quint8* pixels, quint32 stride);

private:
    QSize m_frameSize = {0, 0};
    bool m_needInit = false;

    QOpenGLShaderProgram* m_program = Q_NULLPTR;
    QOpenGLVertexArrayObject m_vao;
    // yuv texture
    GLuint m_texture[Texture_Size] = {0};
    GLint m_drawPos = -1;
};

#endif // YUVGLWIDGET_H
