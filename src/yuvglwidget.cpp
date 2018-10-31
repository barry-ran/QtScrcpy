#include <QOpenGLShaderProgram>

#include "yuvglwidget.h"

static QString vertShader = R"(
                   #version 150 core
                   uniform mat4 u_pm;
                   uniform vec4 draw_pos;

                   const vec2 verts[4] = vec2[] (
                     vec2(-0.5,  0.5),
                     vec2(-0.5, -0.5),
                     vec2( 0.5,  0.5),
                     vec2( 0.5, -0.5)
                   );

                   const vec2 texcoords[4] = vec2[] (
                     vec2(0.0, 1.0),
                     vec2(0.0, 0.0),
                     vec2(1.0, 1.0),
                     vec2(1.0, 0.0)
                   );

                   out vec2 v_coord;

                   void main() {
                      vec2 vert = verts[gl_VertexID];
                      vec4 p = vec4((0.5 * draw_pos.z) + draw_pos.x + (vert.x * draw_pos.z),
                                    (0.5 * draw_pos.w) + draw_pos.y + (vert.y * draw_pos.w),
                                    0, 1);
                      gl_Position = u_pm * p;
                      v_coord = texcoords[gl_VertexID];
                   }
                   )";

    static QString fragShader = R"(
                   #version 150 core
                   uniform sampler2D y_tex;
                   uniform sampler2D u_tex;
                   uniform sampler2D v_tex;
                   in vec2 v_coord;
                   //layout( location = 0 ) out vec4 fragcolor;
                   out vec4 fragcolor;

                   const vec3 R_cf = vec3(1.164383,  0.000000,  1.596027);
                   const vec3 G_cf = vec3(1.164383, -0.391762, -0.812968);
                   const vec3 B_cf = vec3(1.164383,  2.017232,  0.000000);
                   const vec3 offset = vec3(-0.0625, -0.5, -0.5);

                   void main() {
                     float y = texture(y_tex, v_coord).r;
                     float u = texture(u_tex, v_coord).r;
                     float v = texture(v_tex, v_coord).r;
                     vec3 yuv = vec3(y,u,v);
                     yuv += offset;
                     fragcolor = vec4(0.0, 0.0, 0.0, 1.0);
                     fragcolor.r = dot(yuv, R_cf);
                     fragcolor.g = dot(yuv, G_cf);
                     fragcolor.b = dot(yuv, B_cf);
                   }
                   )";

YUVGLWidget::YUVGLWidget(QWidget *parent) :
    QOpenGLWidget(parent)
{
    /*
    // 设置opengl兼容性格式为CoreProfile
    QSurfaceFormat defaultFormat = QSurfaceFormat::defaultFormat();
    defaultFormat.setProfile(QSurfaceFormat::CoreProfile);
    defaultFormat.setVersion(3, 3); // Adapt to your system
    QSurfaceFormat::setDefaultFormat(defaultFormat);
    setFormat(defaultFormat);
    */
}

YUVGLWidget::~YUVGLWidget()
{
    makeCurrent();
    deInitTextures();
    doneCurrent();
}

QSize YUVGLWidget::minimumSizeHint() const
{
    return QSize(50, 50);
}

QSize YUVGLWidget::sizeHint() const
{
    return size();
}

void YUVGLWidget::setFrameSize(const QSize& frameSize)
{
    if (m_frameSize != frameSize) {
        m_frameSize = frameSize;
        m_needInit = true;
    }
}

void YUVGLWidget::initializeGL()
{
    initializeOpenGLFunctions();
    glDisable(GL_DEPTH_TEST);
    m_vao.create();
}

void YUVGLWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // init
    if (m_needInit) {
        initShader();
        initTextures();
        m_needInit = false;
    }
    m_vao.bind();
    if (m_program) {
        QMatrix4x4 matrix;
        matrix.ortho(0, width(), height(), 0, 0.0, 100.0f);
        m_program->setUniformValue("u_pm", matrix);
        glUniform4f(m_drawPos, 0, 0, width(), height());
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_texture[Texture_Y]);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_texture[Texture_U]);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, m_texture[Texture_V]);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void YUVGLWidget::resizeGL(int width, int height)
{
    glViewport(0, 0, width, height);
}

void YUVGLWidget::initShader()
{
    if (m_program) {
        m_program->release();
        delete m_program;
        m_program = Q_NULLPTR;
    }
    m_program = new QOpenGLShaderProgram(this);
    // Setup shaders
    m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, vertShader);
    m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, fragShader);
    m_program->link();
    m_program->bind();
    m_program->setUniformValue("y_tex", Texture_Y);
    m_program->setUniformValue("u_tex", Texture_U);
    m_program->setUniformValue("v_tex", Texture_V);
    m_drawPos = m_program->uniformLocation("draw_pos");
}

void YUVGLWidget::updateTextures(quint8 *dataY, quint8 *dataU, quint8 *dataV, quint32 linesizeY, quint32 linesizeU, quint32 linesizeV)
{
    bindPixelTexture(m_texture[Texture_Y], Texture_Y, dataY, linesizeY);
    bindPixelTexture(m_texture[Texture_U], Texture_U, dataU, linesizeU);
    bindPixelTexture(m_texture[Texture_V], Texture_V, dataV, linesizeV);
    update();
}

void YUVGLWidget::bindPixelTexture(GLuint texture, YUVTextureType textureType, quint8* pixels, quint32 stride)
{
    if (!pixels)
        return;

    QSize size(0, 0);
    calcTextureSize(textureType, size);

    makeCurrent();
    glBindTexture(GL_TEXTURE_2D, texture);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, stride);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, size.width(), size.height(), GL_LUMINANCE, GL_UNSIGNED_BYTE, pixels);
    doneCurrent();
}

void YUVGLWidget::initTexture(qint32 textureType)
{
    if (Texture_NULL >= textureType || Texture_Size <= textureType) {
        return;
    }
    glGenTextures(1, &m_texture[textureType]);
    glBindTexture(GL_TEXTURE_2D, m_texture[textureType]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    QSize size(0, 0);
    calcTextureSize(textureType, size);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, size.width(), size.height(), 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);
}

void YUVGLWidget::initTextures()
{
    //TODO: use FBO?

    deInitTextures();
    for (int i = Texture_Y; i < Texture_Size; i++) {
        initTexture(i);
    }
}

void YUVGLWidget::deInitTextures()
{
    glDeleteTextures(3, m_texture);
    memset(m_texture, 0, Texture_Size);
}

void YUVGLWidget::calcTextureSize(qint32 textureType, QSize &size)
{
    if (Texture_NULL >= textureType || Texture_Size <= textureType) {
        return;
    }
    if (Texture_Y == textureType) {
        size = m_frameSize;
    } else {
        size = m_frameSize / 2;
    }
}
