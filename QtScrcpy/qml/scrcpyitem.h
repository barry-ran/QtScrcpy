#ifndef SCRCPYITEM_H
#define SCRCPYITEM_H

#include <QQuickFramebufferObject>
#include <QPointer>
#include <QMargins>
#include <QSize>
#include <QPoint>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDragLeaveEvent>
#include <QDropEvent>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include "QtScrcpyCore/include/QtScrcpyCore.h"


class ScrcpyItemRenderer;

class ScrcpyItem : public QQuickFramebufferObject, public qsc::DeviceObserver
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QSize frameSize READ frameSize WRITE setFrameSize NOTIFY frameSizeChanged)

public:
    explicit ScrcpyItem(QQuickItem *parent = nullptr);

    Renderer* createRenderer() const override;

    QSize frameSize() const { return m_frameSize; }
    void setFrameSize(const QSize &fs);

    Q_INVOKABLE void updateTextures(quint8* dataY,
                                    quint8* dataU,
                                    quint8* dataV,
                                    quint32 linesizeY,
                                    quint32 linesizeU,
                                    quint32 linesizeV);

    void updateRender(int width, int height,
                      uint8_t *dataY, uint8_t *dataU, uint8_t *dataV,
                      int linesizeY, int linesizeU, int linesizeV);
    void setSerial(const QString &serial);

    QString serial() const;

signals:
    void frameSizeChanged();

private:
    void onFrame(int width, int height,
                 uint8_t* dataY, uint8_t* dataU, uint8_t* dataV,
                 int linesizeY, int linesizeU, int linesizeV) override;

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

    void dropEvent(QDropEvent *event) override;

private:
    void initUI();

    QSize m_frameSize;
    bool m_newFrameAvailable = false;

    quint8* m_dataY = nullptr;
    quint8* m_dataU = nullptr;
    quint8* m_dataV = nullptr;
    quint32 m_linesizeY = 0;
    quint32 m_linesizeU = 0;
    quint32 m_linesizeV = 0;

    QString m_serial;

    friend class ScrcpyItemRenderer;
};


class ScrcpyItemRenderer : public QQuickFramebufferObject::Renderer,
                           protected QOpenGLFunctions
{
public:
    ScrcpyItemRenderer(ScrcpyItem* item);
    ~ScrcpyItemRenderer() override;

    void render() override;
    void synchronize(QQuickFramebufferObject* item) override;
    QSize size() const;

private:
    void releaseGLResources();
    void initShader();
    void initTextures();
    void deInitTextures();
    void updateTexture(GLuint texture, int textureType, quint8* pixels, quint32 stride);

    ScrcpyItem* m_item = nullptr;

    bool m_texturesInited = false;
    bool m_shaderInited = false;
    bool m_vboInited = false;

    GLuint m_textures[3] = {0};
    QOpenGLShaderProgram m_program;
    QOpenGLBuffer m_vbo;

    quint8* m_dataY = nullptr;
    quint8* m_dataU = nullptr;
    quint8* m_dataV = nullptr;
    quint32 m_linesizeY = 0;
    quint32 m_linesizeU = 0;
    quint32 m_linesizeV = 0;
    bool m_needUpdateTextures = false;

    QSize m_localFrameSize;

    static const GLfloat s_coordinates[20];
    static const char* s_vertShaderSrc;
    static const char* s_fragShaderSrc;
};

#endif
