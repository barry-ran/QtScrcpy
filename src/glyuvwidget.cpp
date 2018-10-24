#include "glyuvwidget.h"
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QDebug>
#include <QTime>

#define VERTEXIN 0
#define TEXTUREIN 1

GLYuvWidget::GLYuvWidget(QWidget *parent):
    QOpenGLWidget(parent)
{
}

GLYuvWidget::~GLYuvWidget()
{
    makeCurrent();
    m_vbo.destroy();
    m_textureY->destroy();
    m_textureU->destroy();
    m_textureV->destroy();
    doneCurrent();
}

void GLYuvWidget::slotShowYuv(quint8* bufferY, quint8* bufferU, quint8* bufferV, quint32 linesizeY, quint32 linesizeU, quint32 linesizeV)
{
    qDebug() << "slotShowYuv";
    m_bufferY = bufferY;
    m_bufferU = bufferU;
    m_bufferV = bufferV;
    m_linesizeY = linesizeY;
    m_linesizeU = linesizeU;
    m_linesizeV = linesizeV;
    //update(); // 不实时
    //repaint(); // 同上
    paintEvent(nullptr);// 最实时的方案
}

void GLYuvWidget::initializeGL()
{
    initializeOpenGLFunctions();
    glDisable(GL_DEPTH_TEST); // 关闭深度缓冲测试，对于2D图形来说没用

    static const GLfloat vertices[]{
        // 顶点坐标(-1.0,1.0)
        -1.0f,-1.0f,
        -1.0f,+1.0f,
        +1.0f,+1.0f,
        +1.0f,-1.0f,
        // 纹理坐标(0.0,1.0)
        0.0f,1.0f,
        0.0f,0.0f,
        1.0f,0.0f,
        1.0f,1.0f,
    };

    // 初始化顶点缓冲
    m_vbo.create();
    m_vbo.bind();
    m_vbo.allocate(vertices,sizeof(vertices));

    // 顶点着色器
    QOpenGLShader *vShader = new QOpenGLShader(QOpenGLShader::Vertex, this);
    // 顶点着色器源码
    const char *vertexSrc =
    "attribute vec4 vertexIn; \
    attribute vec2 textureIn; \
    varying vec2 textureOut;  \
    void main(void)           \
    {                         \
        gl_Position = vertexIn; \
        textureOut = textureIn; \
    }";
    vShader->compileSourceCode(vertexSrc);

    // 片段着色器
    QOpenGLShader *fShader = new QOpenGLShader(QOpenGLShader::Fragment, this);
    // 片段着色器源码
    const char *fragmentSrc =
    "varying vec2 textureOut; \
    uniform sampler2D tex_y; \
    uniform sampler2D tex_u; \
    uniform sampler2D tex_v; \
    void main(void) \
    { \
        vec3 yuv; \
        vec3 rgb; \
        yuv.x = texture2D(tex_y, textureOut).r; \
        yuv.y = texture2D(tex_u, textureOut).r - 0.5; \
        yuv.z = texture2D(tex_v, textureOut).r - 0.5; \
        rgb = mat3( 1,       1,         1, \
                    0,       -0.39465,  2.03211, \
                    1.13983, -0.58060,  0) * yuv; \
        gl_FragColor = vec4(rgb, 1); \
    }";
    fShader->compileSourceCode(fragmentSrc);

    // 着色器程序
    m_shaderProgram = new QOpenGLShaderProgram(this);
    // 添加着色器
    m_shaderProgram->addShader(vShader);
    m_shaderProgram->addShader(fShader);
    m_shaderProgram->bindAttributeLocation("vertexIn",VERTEXIN);
    m_shaderProgram->bindAttributeLocation("textureIn",TEXTUREIN);
    // 链接着色器
    m_shaderProgram->link();
    m_shaderProgram->bind();
    m_shaderProgram->enableAttributeArray(VERTEXIN);
    m_shaderProgram->enableAttributeArray(TEXTUREIN);
    m_shaderProgram->setAttributeBuffer(VERTEXIN, GL_FLOAT, 0, 2, 2*sizeof(GLfloat));
    m_shaderProgram->setAttributeBuffer(TEXTUREIN, GL_FLOAT, 8*sizeof(GLfloat), 2, 2*sizeof(GLfloat));

    // 着色器程序中取得yuv纹理位置
    m_textureUniformY = m_shaderProgram->uniformLocation("tex_y");
    m_textureUniformU = m_shaderProgram->uniformLocation("tex_u");
    m_textureUniformV = m_shaderProgram->uniformLocation("tex_v");

    m_textureY = new QOpenGLTexture(QOpenGLTexture::Target2D);
    m_textureU = new QOpenGLTexture(QOpenGLTexture::Target2D);
    m_textureV = new QOpenGLTexture(QOpenGLTexture::Target2D);
    m_textureY->create();
    m_textureU->create();
    m_textureV->create();
    m_idY = m_textureY->textureId();
    m_idU = m_textureU->textureId();
    m_idV = m_textureV->textureId();
    glClearColor(0.0,0.0,0.0,0.0);
}

void GLYuvWidget::inittexture()
{
    //QMatrix4x4 m;
    //m.perspective(60.0f, 4.0f/3.0f, 0.1f, 100.0f );//透视矩阵随距离的变化，图形跟着变化。屏幕平面中心就是视点（摄像头）,需要将图形移向屏幕里面一定距离。

    //近裁剪平面是一个矩形,矩形左下角点三维空间坐标是（left,bottom,-near）,右上角点是（right,top,-near）所以此处为负，表示z轴最大为10；
    //远裁剪平面也是一个矩形,左下角点空间坐标是（left,bottom,-far）,右上角点是（right,top,-far）所以此处为正，表示z轴最小为-10；
    //此时坐标中心还是在屏幕水平面中间，只是前后左右的距离已限制。
    //m.ortho(-2,+2,-2,+2,-10,10);

    glActiveTexture(GL_TEXTURE0);  //激活纹理单元GL_TEXTURE0,系统里面的
    glBindTexture(GL_TEXTURE_2D, m_idY); //绑定y分量纹理对象id到激活的纹理单元
    glPixelStorei(GL_UNPACK_ROW_LENGTH, m_linesizeY);
    //使用内存中的数据创建真正的y分量纹理数据
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, m_videoWidth, m_videoHeight, 0, GL_RED, GL_UNSIGNED_BYTE, m_bufferY);
    //https://blog.csdn.net/xipiaoyouzi/article/details/53584798 纹理参数解析
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glActiveTexture(GL_TEXTURE1); //激活纹理单元GL_TEXTURE1
    glBindTexture(GL_TEXTURE1, m_idU);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, m_linesizeU);
    //使用内存中的数据创建真正的u分量纹理数据
    glTexImage2D(GL_TEXTURE_2D,0,GL_RED, m_videoWidth >> 1, m_videoHeight >> 1, 0, GL_RED, GL_UNSIGNED_BYTE, m_bufferU);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glActiveTexture(GL_TEXTURE2); //激活纹理单元GL_TEXTURE2
    glBindTexture(GL_TEXTURE_2D, m_idV);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, m_linesizeV);
    //使用内存中的数据创建真正的v分量纹理数据
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, m_videoWidth >> 1, m_videoHeight >> 1, 0, GL_RED, GL_UNSIGNED_BYTE, m_bufferV);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void GLYuvWidget::paintGL()
{    
    qDebug() << "paintGL()";
    inittexture();
    //指定y纹理要使用新值
    glUniform1i(m_textureUniformY, 0);
    //指定u纹理要使用新值
    glUniform1i(m_textureUniformU, 1);
    //指定v纹理要使用新值
    glUniform1i(m_textureUniformV, 2);
    //使用顶点数组方式绘制图形
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}
