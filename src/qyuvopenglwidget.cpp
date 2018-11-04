#include <QCoreApplication>
#include <QOpenGLTexture>
#include <QOpenGLPixelTransferOptions>

#include "qyuvopenglwidget.h"

// 存储顶点坐标和纹理坐标
// 存在一起缓存在vbo
// 使用glVertexAttribPointer指定访问方式即可
static const GLfloat coordinate[] = {
    // 顶点坐标，存储4个xyz坐标
    // 坐标范围为[-1,1],中心点为 0,0
    // 二维图像z始终为0
    // GL_TRIANGLE_STRIP的绘制方式：
    // 使用前3个坐标绘制一个三角形，使用后三个坐标绘制一个三角形，正好为一个矩形
    // x     y     z
    -1.0f, -1.0f, 0.0f,
    1.0f, -1.0f, 0.0f,
    -1.0f, 1.0f, 0.0f,
    1.0f, 1.0f, 0.0f,

    // 纹理坐标，存储4个xy坐标
    // 坐标范围为[0,1],左下角为 0,0
    // TODO 为什么这个顺序指定四个顶点？顶点坐标和纹理坐标如何映射的？
    0.0f, 1.0f,
    1.0f, 1.0f,
    0.0f, 0.0f,
    1.0f, 0.0f
};

// 顶点着色器
static const QString s_vertShader = R"(
    attribute vec3 vertexIn;    // xyz顶点坐标
    attribute vec2 textureIn;   // xy纹理坐标
    varying vec2 textureOut;    // 传递给片段着色器的纹理坐标
    void main(void)
    {
        gl_Position = vec4(vertexIn, 1.0);  // 1.0表示vertexIn是一个顶点位置
        textureOut = textureIn; // 纹理坐标直接传递给片段着色器
    }
)";

// 片段着色器
static QString s_fragShader = R"(
    varying vec2 textureOut;        // 由顶点着色器传递过来的纹理坐标
    uniform sampler2D textureY;     // uniform 纹理单元，利用纹理单元可以使用多个纹理
    uniform sampler2D textureU;     // sampler2D是2D采样器
    uniform sampler2D textureV;     // 声明yuv三个纹理单元
    void main(void)
    {
        vec3 yuv;
        vec3 rgb;
        // 根据指定的纹理textureY和坐标textureOut来采样
        yuv.x = texture2D(textureY, textureOut).r;
        yuv.y = texture2D(textureU, textureOut).r - 0.5;
        yuv.z = texture2D(textureV, textureOut).r - 0.5;
        // 采样完转为rgb
        rgb = mat3(1.0, 1.0, 1.0,
                    0.0, -0.39465, 2.03211,
                    1.13983, -0.58060, 0.0) * yuv;
        // 输出颜色值
        gl_FragColor = vec4(rgb, 1.0);
    }
)";

QYUVOpenGLWidget::QYUVOpenGLWidget(QWidget *parent) : QOpenGLWidget(parent)
{
    m_textureY = new QOpenGLTexture(QOpenGLTexture::Target2D);
    m_textureU = new QOpenGLTexture(QOpenGLTexture::Target2D);
    m_textureV = new QOpenGLTexture(QOpenGLTexture::Target2D);
}

QYUVOpenGLWidget::~QYUVOpenGLWidget()
{
    makeCurrent();
    m_vbo.destroy();
    deInitTextures();

    if (m_textureY) {
        delete m_textureY;
        m_textureY = Q_NULLPTR;
    }
    if (m_textureU) {
        delete m_textureU;
        m_textureU = Q_NULLPTR;
    }
    if (m_textureV) {
        delete m_textureV;
        m_textureV = Q_NULLPTR;
    }
    doneCurrent();
}

QSize QYUVOpenGLWidget::minimumSizeHint() const
{
    return QSize(50, 50);
}

QSize QYUVOpenGLWidget::sizeHint() const
{
    return size();
}

void QYUVOpenGLWidget::setFrameSize(const QSize &frameSize)
{
    if (m_frameSize != frameSize) {
        m_frameSize = frameSize;
        m_needUpdate = true;
        repaint();
    }
}

void QYUVOpenGLWidget::updateTextures(quint8 *dataY, quint8 *dataU, quint8 *dataV, quint32 linesizeY, quint32 linesizeU, quint32 linesizeV)
{
    if (m_frameSize.isEmpty()) {
        return;
    }
    QOpenGLPixelTransferOptions options;
    if (m_textureY->isStorageAllocated()) {
        options.setRowLength(linesizeY);
        m_textureY->setData(QOpenGLTexture::Luminance, QOpenGLTexture::UInt8,dataY, &options);
    }
    if (m_textureU->isStorageAllocated()) {
        options.setRowLength(linesizeU);
        m_textureU->setData(QOpenGLTexture::Luminance, QOpenGLTexture::UInt8,dataU, &options);
    }
    if (m_textureV->isStorageAllocated()) {
        options.setRowLength(linesizeV);
        m_textureV->setData(QOpenGLTexture::Luminance, QOpenGLTexture::UInt8,dataV, &options);
    }
    update();
}

void QYUVOpenGLWidget::initializeGL()
{
    initializeOpenGLFunctions();
    glDisable(GL_DEPTH_TEST);

    // 顶点缓冲对象初始化
    m_vbo.create();
    m_vbo.bind();
    m_vbo.allocate(coordinate, sizeof(coordinate));
    initShader();
    // 设置背景清理色为黑色
    glClearColor(0.0,0.0,0.0,0.0);
    // 清理颜色背景
    glClear(GL_COLOR_BUFFER_BIT);
}

void QYUVOpenGLWidget::paintGL()
{    
    if (m_frameSize.isEmpty()) {
        return;
    }
    if (m_needUpdate) {
        initTextures();        
        m_needUpdate = false;
    }

    if (m_textureY->isStorageAllocated()) {
        m_textureY->bind(0);        
    }
    if (m_textureU->isStorageAllocated()) {
        m_textureU->bind(1);
    }
    if (m_textureV->isStorageAllocated()) {
        m_textureV->bind(2);
    }

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // 没有画面则显示黑屏
    //glClear(GL_COLOR_BUFFER_BIT);
}

void QYUVOpenGLWidget::resizeGL(int width, int height)
{
    glViewport(0, 0, width, height);
}

void QYUVOpenGLWidget::initShader()
{
    // opengles的float、int等要手动指定精度
    if (QCoreApplication::testAttribute(Qt::AA_UseOpenGLES)) {
        s_fragShader.prepend(R"(
                             precision mediump int;
                             precision mediump float;
                             )");
    }
    m_shaderProgram.addShaderFromSourceCode(QOpenGLShader::Vertex, s_vertShader);
    m_shaderProgram.addShaderFromSourceCode(QOpenGLShader::Fragment, s_fragShader);
    m_shaderProgram.link();
    m_shaderProgram.bind();

    // 指定顶点坐标在vbo中的访问方式
    // 参数解释：顶点坐标在shader中的参数名称，顶点坐标为float，起始偏移为0，顶点坐标类型为vec3，步幅为3个float
    m_shaderProgram.setAttributeBuffer("vertexIn", GL_FLOAT, 0, 3, 3 * sizeof(float));
    // 启用顶点属性
    m_shaderProgram.enableAttributeArray("vertexIn");

    // 指定纹理坐标在vbo中的访问方式
    // 参数解释：纹理坐标在shader中的参数名称，纹理坐标为float，起始偏移为12个float（跳过前面存储的12个顶点坐标），纹理坐标类型为vec2，步幅为2个float
    m_shaderProgram.setAttributeBuffer("textureIn", GL_FLOAT, 12 * sizeof(float), 2, 2 * sizeof(float));
    m_shaderProgram.enableAttributeArray("textureIn");

    // 关联片段着色器中的纹理单元和opengl中的纹理单元（opengl一般提供16个纹理单元）
    m_shaderProgram.setUniformValue("textureY", 0);
    m_shaderProgram.setUniformValue("textureU", 1);
    m_shaderProgram.setUniformValue("textureV", 2);
}

void QYUVOpenGLWidget::initTextures()
{
    deInitTextures();
    if (m_textureY) {
        m_textureY->create();
        m_textureY->setSize(m_frameSize.width(), m_frameSize.height());
        // 设置纹理缩放时的策略
        m_textureY->setMinificationFilter(QOpenGLTexture::Linear);
        m_textureY->setMagnificationFilter(QOpenGLTexture::Linear);
        // 设置所有方向上纹理超出坐标时的显示策略（也可单个方向单独设置）
        m_textureY->setWrapMode(QOpenGLTexture::ClampToEdge);
        m_textureY->setMipLevels(1);
        m_textureY->setFormat(QOpenGLTexture::LuminanceFormat);
        m_textureY->allocateStorage(QOpenGLTexture::Luminance, QOpenGLTexture::UInt8);
    }

    if (m_textureU) {
        m_textureU->create();
        m_textureU->setSize(m_frameSize.width()/2, m_frameSize.height()/2);
        m_textureU->setMinificationFilter(QOpenGLTexture::Linear);
        m_textureU->setMagnificationFilter(QOpenGLTexture::Linear);
        m_textureU->setWrapMode(QOpenGLTexture::ClampToEdge);
        m_textureU->setMipLevels(1);
        m_textureU->setFormat(QOpenGLTexture::LuminanceFormat);
        m_textureU->allocateStorage(QOpenGLTexture::Luminance, QOpenGLTexture::UInt8);
    }

    if (m_textureV) {
        m_textureV->create();
        m_textureV->setSize(m_frameSize.width()/2, m_frameSize.height()/2);
        m_textureV->setMinificationFilter(QOpenGLTexture::Linear);
        m_textureV->setMagnificationFilter(QOpenGLTexture::Linear);
        m_textureV->setWrapMode(QOpenGLTexture::ClampToEdge);
        m_textureV->setMipLevels(1);
        m_textureV->setFormat(QOpenGLTexture::LuminanceFormat);
        m_textureV->allocateStorage(QOpenGLTexture::Luminance, QOpenGLTexture::UInt8);
    }
}

void QYUVOpenGLWidget::deInitTextures()
{
    if (m_textureY) {
        m_textureY->destroy();
    }
    if (m_textureU) {
        m_textureU->destroy();
    }
    if (m_textureV) {
        m_textureV->destroy();
    }
}

void QYUVOpenGLWidget::updateTexture(GLuint texture, quint32 textureType, quint8 *pixels, quint32 stride)
{
    if (!pixels)
        return;

    QSize size = 0 == textureType ? m_frameSize : m_frameSize/2;

    makeCurrent();
    glBindTexture(GL_TEXTURE_2D, texture);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, stride);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, size.width(), size.height(), GL_RED, GL_UNSIGNED_BYTE, pixels);
    doneCurrent();
}
