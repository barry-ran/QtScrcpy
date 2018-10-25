#include "yuvglwidget.h"

YUVGLWidget::YUVGLWidget(QWidget *parent) :
    QOpenGLWidget(parent)
{
    // 设置opengl兼容性格式为CoreProfile
    QSurfaceFormat defaultFormat = QSurfaceFormat::defaultFormat();
    defaultFormat.setProfile(QSurfaceFormat::CoreProfile);
    defaultFormat.setVersion(3, 3); // Adapt to your system
    QSurfaceFormat::setDefaultFormat(defaultFormat);

    setFormat(defaultFormat);
}

YUVGLWidget::~YUVGLWidget()
{
    makeCurrent();
    if (y_tex) glDeleteTextures(1, &y_tex);
    if (u_tex) glDeleteTextures(1, &u_tex);
    if (v_tex) glDeleteTextures(1, &v_tex);
    doneCurrent();
}

QSize YUVGLWidget::minimumSizeHint() const
{
    return QSize(50, 50);
}

QSize YUVGLWidget::sizeHint() const
{
    return QSize(3840, 2160); // 4k video
}

void YUVGLWidget::setFrameSize(unsigned int width, unsigned int height)
{
    m_frameWidth = width;
    m_frameHeight = height;
}

void YUVGLWidget::initializeGL()
{
    initializeOpenGLFunctions();
    glDisable(GL_DEPTH_TEST);

    initializeTextures();

    QString vert = R"(
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

    QString frag = R"(
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
    // Setup shaders
    m_program.addShaderFromSourceCode(QOpenGLShader::Vertex, vert);
    m_program.addShaderFromSourceCode(QOpenGLShader::Fragment, frag);

//    m_program.bindAttributeLocation("y_tex", 0);
//    m_program.bindAttributeLocation("u_tex", 1);
//    m_program.bindAttributeLocation("v_tex", 2);

    m_program.link();
    m_program.bind();

    m_program.setUniformValue("y_tex", 0);
    m_program.setUniformValue("u_tex", 1);
    m_program.setUniformValue("v_tex", 2);

    u_pos = m_program.uniformLocation("draw_pos");

    m_vao.create();
}

void YUVGLWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_vao.bind();

    QMatrix4x4 m;
    m.ortho(0, width(), height(), 0, 0.0, 100.0f);

    m_program.setUniformValue("u_pm", m);

    glUniform4f(u_pos, 0, 0, width(), height());

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, y_tex);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, u_tex);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, v_tex);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}


void YUVGLWidget::setYPixels(uint8_t* pixels, int stride)
{
    bindPixelTexture(y_tex, YTexture, pixels, stride);
}

void YUVGLWidget::setUPixels(uint8_t* pixels, int stride)
{
    bindPixelTexture(u_tex, UTexture, pixels, stride);
}

void YUVGLWidget::setVPixels(uint8_t* pixels, int stride)
{
    bindPixelTexture(v_tex, VTexture, pixels, stride);
}


void YUVGLWidget::initializeTextures()
{
    //TODO: use FBO?

    // Setup textures
    glGenTextures(1, &y_tex);
    glBindTexture(GL_TEXTURE_2D, y_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, m_frameWidth, m_frameHeight, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);

    glGenTextures(1, &u_tex);
    glBindTexture(GL_TEXTURE_2D, u_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, m_frameWidth/2, m_frameHeight/2, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);

    glGenTextures(1, &v_tex);
    glBindTexture(GL_TEXTURE_2D, v_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, m_frameWidth/2, m_frameHeight/2, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
}

void YUVGLWidget::bindPixelTexture(GLuint texture, YUVGLWidget::YUVTextureType textureType, uint8_t* pixels, int stride)
{
    if (!pixels)
        return;

    unsigned int const width = textureType == YTexture ? m_frameWidth : m_frameWidth/2;
    unsigned int const height = textureType == YTexture ? m_frameHeight : m_frameHeight/2;

    makeCurrent();
    glBindTexture(GL_TEXTURE_2D, texture);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, stride);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RED, GL_UNSIGNED_BYTE, pixels);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, m_frameWidth/2, m_frameHeight/2, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
    doneCurrent();
}
