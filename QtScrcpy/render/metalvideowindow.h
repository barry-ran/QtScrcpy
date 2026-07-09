#ifndef METALVIDEOWINDOW_H
#define METALVIDEOWINDOW_H

#include <QWidget>

typedef struct __CVBuffer *CVPixelBufferRef;

class MetalVideoWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MetalVideoWidget(QWidget *parent = nullptr);
    ~MetalVideoWidget();

    bool initMetal();
    void renderFrame(CVPixelBufferRef pixelBuffer, int width, int height);

    QPaintEngine* paintEngine() const override { return nullptr; }

protected:
    void paintEvent(QPaintEvent *) override {}

private:
    struct Impl;
    Impl *d = nullptr;
    bool m_metalReady = false;
};

#endif
