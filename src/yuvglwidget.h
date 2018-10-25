#ifndef YUVGLWIDGET_H
#define YUVGLWIDGET_H

#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLWidget>
#include <QSurfaceFormat>

class YUVGLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    explicit YUVGLWidget(QWidget* parent = 0);
    ~YUVGLWidget();

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

    void setFrameSize(unsigned int width, unsigned int height);

    void setYPixels(uint8_t* pixels, int stride);
    void setUPixels(uint8_t* pixels, int stride);
    void setVPixels(uint8_t* pixels, int stride);

protected:
    void initializeGL() override;
    void paintGL() override;
    //void resizeGL(int width, int height) override;

private:
    enum YUVTextureType {
        YTexture,
        UTexture,
        VTexture
    };

    void initializeTextures();
    void bindPixelTexture(GLuint texture, YUVTextureType textureType, uint8_t* pixels, int stride);

    QOpenGLShaderProgram m_program;
    QOpenGLVertexArrayObject m_vao;

    //unsigned int m_frameWidth {3840}; // 4k
    //unsigned int m_frameHeight {2160};
    unsigned int m_frameWidth {2160};
    unsigned int m_frameHeight {1080};

    GLuint y_tex {0};
    GLuint u_tex {0};
    GLuint v_tex {0};
    GLint u_pos {0};
};

#endif // YUVGLWIDGET_H
