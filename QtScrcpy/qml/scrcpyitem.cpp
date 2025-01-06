#include "scrcpyitem.h"
#include <QGuiApplication>
#include <QPainter>
#include <QScreen>
#include <QFileInfo>
#include <QMimeData>
#include <QOpenGLContext>
#include <QQuickWindow>
#include <QCoreApplication>
#include <cstring>
#include <QtMath>

#include "util/config.h"

ScrcpyItem::ScrcpyItem(QQuickItem *parent)
    : QQuickFramebufferObject(parent)
{
    setMirrorVertically(false);
    setTextureFollowsItemSize(true);
    initUI();
}

QQuickFramebufferObject::Renderer* ScrcpyItem::createRenderer() const
{
    return new ScrcpyItemRenderer(const_cast<ScrcpyItem*>(this));
}

void ScrcpyItem::initUI()
{
    setAcceptTouchEvents(true);
    setAcceptedMouseButtons(Qt::AllButtons);
}

void ScrcpyItem::setFrameSize(const QSize &fs)
{
    if (m_frameSize != fs) {
        m_frameSize = fs;
        emit frameSizeChanged();
        update();
    }
}

void ScrcpyItem::updateTextures(quint8* dataY,
                                quint8* dataU,
                                quint8* dataV,
                                quint32 linesizeY,
                                quint32 linesizeU,
                                quint32 linesizeV)
{
    m_dataY = dataY;
    m_dataU = dataU;
    m_dataV = dataV;
    m_linesizeY = linesizeY;
    m_linesizeU = linesizeU;
    m_linesizeV = linesizeV;

    m_newFrameAvailable = true;
    update();
}

void ScrcpyItem::updateRender(int width, int height,
                              uint8_t *dataY, uint8_t *dataU, uint8_t *dataV,
                              int linesizeY, int linesizeU, int linesizeV)
{
    setFrameSize(QSize(width, height));
    updateTextures(dataY, dataU, dataV, linesizeY, linesizeU, linesizeV);
}

void ScrcpyItem::setSerial(const QString &serial)
{
    m_serial = serial;
}

QString ScrcpyItem::serial() const
{
    return m_serial;
}

void ScrcpyItem::onFrame(int width, int height,
                         uint8_t* dataY, uint8_t* dataU, uint8_t* dataV,
                         int linesizeY, int linesizeU, int linesizeV)
{
    updateRender(width, height, dataY, dataU, dataV, linesizeY, linesizeU, linesizeV);
}

void ScrcpyItem::mousePressEvent(QMouseEvent *event)
{
    auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
    if (event->button() == Qt::MiddleButton && device && !device->isCurrentCustomKeymap()) {
        device->postGoHome();
        return;
    }

    if (event->button() == Qt::RightButton && device && !device->isCurrentCustomKeymap()) {
        device->postGoBack();
        return;
    }

    if (boundingRect().contains(event->position())) {
        if (!device) {
            return;
        }
        QPointF mappedPos = event->position();
        emit device->mouseEvent(new QMouseEvent(event->type(),
                                                mappedPos,
                                                event->globalPosition(),
                                                event->button(),
                                                event->buttons(),
                                                event->modifiers()),
                                QSize(frameSize().width(), frameSize().height()),
                                size().toSize());
        if (event->button() == Qt::LeftButton) {
            qreal x = mappedPos.x() / width();
            qreal y = mappedPos.y() / height();
            qInfo() << QString(R"("pos": {"x": %1, "y": %2})").arg(x).arg(y);
        }
    } else {
        if (event->button() == Qt::LeftButton) {
            event->accept();
        }
    }
}

void ScrcpyItem::mouseReleaseEvent(QMouseEvent *event)
{
    auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
    if (!device) {
        return;
    }

    QPointF local = event->position();

    if (local.x() < 0)  local.setX(0);
    if (local.x() > width())  local.setX(width());
    if (local.y() < 0)  local.setY(0);
    if (local.y() > height()) local.setY(height());

    emit device->mouseEvent(new QMouseEvent(event->type(),
                                            local,
                                            event->globalPosition(),
                                            event->button(),
                                            event->buttons(),
                                            event->modifiers()),
                            QSize(frameSize().width(), frameSize().height()),
                            size().toSize());
}

void ScrcpyItem::mouseMoveEvent(QMouseEvent *event)
{
    auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
    if (boundingRect().contains(event->position())) {
        if (!device) {
            return;
        }
        QPointF mappedPos = event->position();
        emit device->mouseEvent(new QMouseEvent(event->type(),
                                                mappedPos,
                                                event->globalPosition(),
                                                event->button(),
                                                event->buttons(),
                                                event->modifiers()),
                                QSize(frameSize().width(), frameSize().height()),
                                size().toSize());
    }
}

void ScrcpyItem::mouseDoubleClickEvent(QMouseEvent *event)
{
    auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
    if (event->button() == Qt::RightButton && device && !device->isCurrentCustomKeymap()) {
        emit device->postBackOrScreenOn(event->type() == QEvent::MouseButtonPress);
    }

    if (boundingRect().contains(event->position())) {
        if (!device) {
            return;
        }
        QPointF mappedPos = event->position();
        emit device->mouseEvent(new QMouseEvent(event->type(),
                                                mappedPos,
                                                event->globalPosition(),
                                                event->button(),
                                                event->buttons(),
                                                event->modifiers()),
                                QSize(frameSize().width(), frameSize().height()),
                                size().toSize());
    }
}

void ScrcpyItem::wheelEvent(QWheelEvent *event)
{
    auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
    if (boundingRect().contains(event->position())) {
        if (!device) {
            return;
        }
        QPointF pos = event->position();
        
        QWheelEvent adjustedEvent(
            pos,
            event->globalPosition(),
            event->pixelDelta(),
            event->angleDelta(),
            event->buttons(),
            event->modifiers(),
            event->phase(),
            event->inverted()
            );
        emit device->wheelEvent(&adjustedEvent,
                                QSize(frameSize().width(), frameSize().height()),
                                size().toSize());
    }
}

void ScrcpyItem::keyPressEvent(QKeyEvent *event)
{
    auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
    if (!device) {
        return;
    }
    emit device->keyEvent(event,
                          QSize(frameSize().width(), frameSize().height()),
                          size().toSize());
}

void ScrcpyItem::keyReleaseEvent(QKeyEvent *event)
{
    auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
    if (!device) {
        return;
    }
    emit device->keyEvent(event,
                          QSize(frameSize().width(), frameSize().height()),
                          size().toSize());
}

void ScrcpyItem::dropEvent(QDropEvent *event)
{
    auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
    if (!device) {
        return;
    }
    const QMimeData *qm = event->mimeData();
    QList<QUrl> urls = qm->urls();

    for (const QUrl &url : urls) {
        QString file = url.toLocalFile();
        QFileInfo fileInfo(file);
        if (!fileInfo.exists()) {
            continue;
        }
        if (fileInfo.isFile() && fileInfo.suffix() == "apk") {
            emit device->installApkRequest(file);
            continue;
        }
        emit device->pushFileRequest(file, Config::getInstance().getPushFilePath() + fileInfo.fileName());
    }
}

static const GLfloat s_coords[20] = {
    -1.0f, -1.0f, 0.0f,
    1.0f, -1.0f, 0.0f,
    -1.0f,  1.0f, 0.0f,
    1.0f,  1.0f, 0.0f,

    0.0f,  0.0f,
    1.0f,  0.0f,
    0.0f,  1.0f,
    1.0f,  1.0f
};

const GLfloat ScrcpyItemRenderer::s_coordinates[20] = {
    s_coords[0], s_coords[1], s_coords[2], s_coords[3], s_coords[4],
    s_coords[5], s_coords[6], s_coords[7], s_coords[8], s_coords[9],
    s_coords[10], s_coords[11], s_coords[12], s_coords[13], s_coords[14],
    s_coords[15], s_coords[16], s_coords[17], s_coords[18], s_coords[19]
};

const char* ScrcpyItemRenderer::s_vertShaderSrc = R"(
attribute vec3 vertexIn;
attribute vec2 textureIn;
varying vec2 textureOut;
void main(void)
{
    gl_Position = vec4(vertexIn, 1.0);
    textureOut = textureIn;
}
)";

const char* ScrcpyItemRenderer::s_fragShaderSrc = R"(
#ifdef GL_ES
precision mediump float;
precision mediump int;
#endif

varying vec2 textureOut;
uniform sampler2D textureY;
uniform sampler2D textureU;
uniform sampler2D textureV;

void main(void)
{
    vec3 yuv;
    vec3 rgb;

    const vec3 Rcoeff = vec3(1.1644,  0.0000,  1.7927);
    const vec3 Gcoeff = vec3(1.1644, -0.2132, -0.5329);
    const vec3 Bcoeff = vec3(1.1644,  2.1124,  0.0000);

    yuv.x = texture2D(textureY, textureOut).r - 0.0625;
    yuv.y = texture2D(textureU, textureOut).r - 0.5;
    yuv.z = texture2D(textureV, textureOut).r - 0.5;

    rgb.r = dot(yuv, Rcoeff);
    rgb.g = dot(yuv, Gcoeff);
    rgb.b = dot(yuv, Bcoeff);

    gl_FragColor = vec4(rgb, 1.0);
}
)";

ScrcpyItemRenderer::ScrcpyItemRenderer(ScrcpyItem* item)
    : m_item(item)
{
    initializeOpenGLFunctions();
}

ScrcpyItemRenderer::~ScrcpyItemRenderer()
{
    releaseGLResources();
}

void ScrcpyItemRenderer::synchronize(QQuickFramebufferObject* qItem)
{
    auto item = static_cast<ScrcpyItem*>(qItem);
    if (!item) return;

    if (item->m_newFrameAvailable) {
        m_dataY = item->m_dataY;
        m_dataU = item->m_dataU;
        m_dataV = item->m_dataV;
        m_linesizeY = item->m_linesizeY;
        m_linesizeU = item->m_linesizeU;
        m_linesizeV = item->m_linesizeV;
        m_needUpdateTextures = true;
        item->m_newFrameAvailable = false;
    }

    if (m_localFrameSize != item->m_frameSize) {
        m_localFrameSize = item->m_frameSize;
        deInitTextures();
    }
}

void ScrcpyItemRenderer::render()
{
    if (!m_shaderInited) {
        initShader();
        m_shaderInited = true;
    }
    if (!m_vboInited) {
        m_vbo.create();
        m_vbo.bind();
        m_vbo.allocate(s_coordinates, sizeof(s_coordinates));
        m_vboInited = true;
    }
    if (!m_texturesInited && m_localFrameSize.isValid()) {
        initTextures();
        m_texturesInited = true;
    }

    glViewport(0, 0, size().width(), size().height());
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    if (m_needUpdateTextures && m_texturesInited) {
        if (m_dataY) updateTexture(m_textures[0], 0, m_dataY, m_linesizeY);
        if (m_dataU) updateTexture(m_textures[1], 1, m_dataU, m_linesizeU);
        if (m_dataV) updateTexture(m_textures[2], 2, m_dataV, m_linesizeV);
        m_needUpdateTextures = false;
    }

    m_program.bind();
    m_vbo.bind();

    m_program.enableAttributeArray("vertexIn");
    m_program.setAttributeBuffer("vertexIn",
                                 GL_FLOAT,
                                 0,
                                 3,
                                 3 * sizeof(GLfloat));
    m_program.enableAttributeArray("textureIn");
    m_program.setAttributeBuffer("textureIn",
                                 GL_FLOAT,
                                 4 * 3 * sizeof(GLfloat),
                                 2,
                                 2 * sizeof(GLfloat));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_textures[0]);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_textures[1]);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, m_textures[2]);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    m_program.release();
    m_vbo.release();

    update();
}

QSize ScrcpyItemRenderer::size() const
{
    return m_item->size().toSize();
}

void ScrcpyItemRenderer::releaseGLResources()
{
    if (m_vboInited) {
        m_vbo.destroy();
        m_vboInited = false;
    }
    if (m_texturesInited) {
        deInitTextures();
    }
    if (m_shaderInited) {
        m_program.removeAllShaders();
        m_shaderInited = false;
    }
}

void ScrcpyItemRenderer::initShader()
{
    m_program.addShaderFromSourceCode(QOpenGLShader::Vertex, s_vertShaderSrc);
    m_program.addShaderFromSourceCode(QOpenGLShader::Fragment, s_fragShaderSrc);
    m_program.link();
    m_program.bind();

    m_program.setUniformValue("textureY", 0);
    m_program.setUniformValue("textureU", 1);
    m_program.setUniformValue("textureV", 2);

    m_program.release();
}

void ScrcpyItemRenderer::initTextures()
{
    glGenTextures(3, m_textures);

    glBindTexture(GL_TEXTURE_2D, m_textures[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_LUMINANCE,
                 m_localFrameSize.width(),
                 m_localFrameSize.height(),
                 0,
                 GL_LUMINANCE,
                 GL_UNSIGNED_BYTE,
                 nullptr);

    glBindTexture(GL_TEXTURE_2D, m_textures[1]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_LUMINANCE,
                 m_localFrameSize.width() / 2,
                 m_localFrameSize.height() / 2,
                 0,
                 GL_LUMINANCE,
                 GL_UNSIGNED_BYTE,
                 nullptr);

    glBindTexture(GL_TEXTURE_2D, m_textures[2]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_LUMINANCE,
                 m_localFrameSize.width() / 2,
                 m_localFrameSize.height() / 2,
                 0,
                 GL_LUMINANCE,
                 GL_UNSIGNED_BYTE,
                 nullptr);
}

void ScrcpyItemRenderer::deInitTextures()
{
    glDeleteTextures(3, m_textures);
    std::memset(m_textures, 0, sizeof(m_textures));
    m_texturesInited = false;
}

void ScrcpyItemRenderer::updateTexture(GLuint texture, int textureType,
                                       quint8* pixels, quint32 stride)
{
    if (!pixels)
        return;
    glBindTexture(GL_TEXTURE_2D, texture);

    QSize planeSize = (textureType == 0) ? m_localFrameSize : (m_localFrameSize / 2);

    glPixelStorei(GL_UNPACK_ROW_LENGTH, stride);
    glTexSubImage2D(GL_TEXTURE_2D,
                    0,
                    0, 0,
                    planeSize.width(),
                    planeSize.height(),
                    GL_LUMINANCE,
                    GL_UNSIGNED_BYTE,
                    pixels);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
}
