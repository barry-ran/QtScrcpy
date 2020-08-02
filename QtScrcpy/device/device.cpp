#include <QDir>
#include <QMessageBox>
#include <QTimer>

#include "avframeconvert.h"
#include "config.h"
#include "controller.h"
#include "decoder.h"
#include "device.h"
#include "filehandler.h"
#include "mousetap/mousetap.h"
#include "recorder.h"
#include "server.h"
#include "stream.h"
#include "videobuffer.h"
#include "videoform.h"
extern "C"
{
#include "libavutil/imgutils.h"
}

Device::Device(DeviceParams params, QObject *parent) : QObject(parent), m_params(params)
{
    if (!params.display && m_params.recordFileName.trimmed().isEmpty()) {
        qCritical("not display must be recorded");
        deleteLater();
        return;
    }

    if (params.display) {
        m_vb = new VideoBuffer();
        m_vb->init(params.renderExpiredFrames);
        m_decoder = new Decoder(m_vb, this);
        m_fileHandler = new FileHandler(this);
        m_controller = new Controller(params.gameScript, this);
        m_videoForm = new VideoForm(Config::getInstance().getFramelessWindow(), Config::getInstance().getSkin());
        m_videoForm->setDevice(this);
    }

    m_stream = new Stream(this);
    if (m_decoder) {
        m_stream->setDecoder(m_decoder);
    }
    m_server = new Server(this);
    if (!m_params.recordFileName.trimmed().isEmpty()) {
        m_recorder = new Recorder(m_params.recordFileName);
        m_stream->setRecoder(m_recorder);
    }
    initSignals();
    startServer();
}

Device::~Device()
{
    if (m_server) {
        m_server->stop();
    }
    // server must stop before decoder, because decoder block main thread
    if (m_stream) {
        m_stream->stopDecode();
    }

    if (m_recorder) {
        delete m_recorder;
    }
    if (m_vb) {
        m_vb->deInit();
        delete m_vb;
    }
    if (m_videoForm) {
        m_videoForm->close();
        delete m_videoForm;
    }
    emit deviceDisconnect(m_params.serial);
}

VideoForm *Device::getVideoForm()
{
    return m_videoForm;
}

Server *Device::getServer()
{
    return m_server;
}

const QString &Device::getSerial()
{
    return m_params.serial;
}

const QSize Device::frameSize()
{
    QSize size;
    if (!m_videoForm) {
        return size;
    }
    return m_videoForm->frameSize();
}

void Device::updateScript(QString script)
{
    if (m_controller) {
        m_controller->updateScript(script);
    }
}

void Device::onScreenshot()
{
    if (!m_vb) {
        return;
    }

    m_vb->lock();
    // screenshot
    saveFrame(m_vb->peekRenderedFrame());
    m_vb->unLock();
}

void Device::onShowTouch(bool show)
{
    AdbProcess *adb = new AdbProcess();
    if (!adb) {
        return;
    }
    connect(adb, &AdbProcess::adbProcessResult, this, [this](AdbProcess::ADB_EXEC_RESULT processResult) {
        if (AdbProcess::AER_SUCCESS_START != processResult) {
            sender()->deleteLater();
        }
    });
    adb->setShowTouchesEnabled(getSerial(), show);

    qInfo() << getSerial() << " show touch " << (show ? "enable" : "disable");
}

void Device::initSignals()
{
    connect(this, &Device::screenshot, this, &Device::onScreenshot);
    connect(this, &Device::showTouch, this, &Device::onShowTouch);
    connect(this, &Device::setControlState, this, &Device::onSetControlState);
    connect(this, &Device::grabCursor, this, &Device::onGrabCursor);

    if (m_controller) {
        connect(m_controller, &Controller::grabCursor, this, &Device::grabCursor);
    }
    if (m_controller) {
        connect(this, &Device::postGoBack, m_controller, &Controller::onPostGoBack);
        connect(this, &Device::postGoHome, m_controller, &Controller::onPostGoHome);
        connect(this, &Device::postGoMenu, m_controller, &Controller::onPostGoMenu);
        connect(this, &Device::postAppSwitch, m_controller, &Controller::onPostAppSwitch);
        connect(this, &Device::postPower, m_controller, &Controller::onPostPower);
        connect(this, &Device::postVolumeUp, m_controller, &Controller::onPostVolumeUp);
        connect(this, &Device::postVolumeDown, m_controller, &Controller::onPostVolumeDown);
        connect(this, &Device::setScreenPowerMode, m_controller, &Controller::onSetScreenPowerMode);
        connect(this, &Device::expandNotificationPanel, m_controller, &Controller::onExpandNotificationPanel);
        connect(this, &Device::collapseNotificationPanel, m_controller, &Controller::onCollapseNotificationPanel);
        connect(this, &Device::mouseEvent, m_controller, &Controller::onMouseEvent);
        connect(this, &Device::wheelEvent, m_controller, &Controller::onWheelEvent);
        connect(this, &Device::keyEvent, m_controller, &Controller::onKeyEvent);

        connect(this, &Device::postBackOrScreenOn, m_controller, &Controller::onPostBackOrScreenOn);
        connect(this, &Device::requestDeviceClipboard, m_controller, &Controller::onRequestDeviceClipboard);
        connect(this, &Device::setDeviceClipboard, m_controller, &Controller::onSetDeviceClipboard);
        connect(this, &Device::clipboardPaste, m_controller, &Controller::onClipboardPaste);
        connect(this, &Device::postTextInput, m_controller, &Controller::onPostTextInput);
    }
    if (m_videoForm) {
        connect(m_videoForm, &VideoForm::destroyed, this, [this](QObject *obj) {
            Q_UNUSED(obj)
            deleteLater();
        });

        connect(this, &Device::switchFullScreen, m_videoForm, &VideoForm::onSwitchFullScreen);
    }
    if (m_fileHandler) {
        connect(this, &Device::pushFileRequest, this, [this](const QString &file, const QString &devicePath) {
            m_fileHandler->onPushFileRequest(getSerial(), file, devicePath);
        });
        connect(this, &Device::installApkRequest, this, [this](const QString &apkFile) { m_fileHandler->onInstallApkRequest(getSerial(), apkFile); });
        connect(m_fileHandler, &FileHandler::fileHandlerResult, this, [this](FileHandler::FILE_HANDLER_RESULT processResult, bool isApk) {
            QString tipsType = "";
            if (isApk) {
                tipsType = tr("install apk");
            } else {
                tipsType = tr("file transfer");
            }
            QString tips;
            if (FileHandler::FAR_IS_RUNNING == processResult && m_videoForm) {
                tips = tr("wait current %1 to complete").arg(tipsType);
            }
            if (FileHandler::FAR_SUCCESS_EXEC == processResult && m_videoForm) {
                tips = tr("%1 complete, save in %2").arg(tipsType).arg(Config::getInstance().getPushFilePath());
            }
            if (FileHandler::FAR_ERROR_EXEC == processResult && m_videoForm) {
                tips = tr("%1 failed").arg(tipsType);
            }
            qInfo() << tips;
            if (m_controlState == GCS_CLIENT) {
                return;
            }
            //QMessageBox::information(m_videoForm, "QtScrcpy", tips, QMessageBox::Ok);
        });
    }

    if (m_server) {
        connect(m_server, &Server::serverStartResult, this, [this](bool success) {
            if (success) {
                m_server->connectTo();
            } else {
                deleteLater();
            }
        });
        connect(m_server, &Server::connectToResult, this, [this](bool success, const QString &deviceName, const QSize &size) {
            if (success) {
                double diff = m_startTimeCount.elapsed() / 1000.0;
                qInfo() << QString("server start finish in %1s").arg(diff).toStdString().c_str();

                // update ui
                if (m_videoForm) {
                    // must be show before updateShowSize
                    m_videoForm->show();

                    m_videoForm->setWindowTitle(deviceName);
                    m_videoForm->updateShowSize(size);

                    bool deviceVer = size.height() > size.width();
                    QRect rc = Config::getInstance().getRect(getSerial());
                    bool rcVer = rc.height() > rc.width();
                    // same width/height rate
                    if (rc.isValid() && (deviceVer == rcVer)) {
                        // mark: resize is for fix setGeometry magneticwidget bug
                        m_videoForm->resize(rc.size());
                        m_videoForm->setGeometry(rc);
                    }
                }

                // init recorder
                if (m_recorder) {
                    m_recorder->setFrameSize(size);
                }

                // init decoder
                m_stream->setVideoSocket(m_server->getVideoSocket());
                m_stream->startDecode();

                // init controller
                if (m_controller) {
                    m_controller->setControlSocket(m_server->getControlSocket());
                }

                // 显示界面时才自动息屏（m_params.display）
                if (m_params.closeScreen && m_params.display && m_controller) {
                    emit m_controller->onSetScreenPowerMode(ControlMsg::SPM_OFF);
                }
            }
        });
        connect(m_server, &Server::onServerStop, this, [this]() {
            deleteLater();
            qDebug() << "server process stop";
        });
    }

    if (m_stream) {
        connect(m_stream, &Stream::onStreamStop, this, [this]() {
            deleteLater();
            qDebug() << "stream thread stop";
        });
    }

    if (m_decoder && m_vb) {
        // must be Qt::QueuedConnection, ui update must be main thread
        connect(
            m_decoder,
            &Decoder::onNewFrame,
            this,
            [this]() {
                m_vb->lock();
                const AVFrame *frame = m_vb->consumeRenderedFrame();
                if (m_videoForm) {
                    m_videoForm->updateRender(frame);
                }
                m_vb->unLock();
            },
            Qt::QueuedConnection);
        connect(m_vb->getFPSCounter(), &::FpsCounter::updateFPS, m_videoForm, &VideoForm::updateFPS);
    }
}

void Device::startServer()
{
    // fix: macos cant recv finished signel, timer is ok
    QTimer::singleShot(0, this, [this]() {
        m_startTimeCount.start();
        // max size support 480p 720p 1080p 设备原生分辨率
        // support wireless connect, example:
        //m_server->start("192.168.0.174:5555", 27183, m_maxSize, m_bitRate, "");
        // only one devices, serial can be null
        // mark: crop input format: "width:height:x:y" or - for no crop, for example: "100:200:0:0"
        Server::ServerParams params;
        params.serial = m_params.serial;
        params.localPort = m_params.localPort;
        params.maxSize = m_params.maxSize;
        params.bitRate = m_params.bitRate;
        params.maxFps = m_params.maxFps;
        params.crop = "-";
        params.control = true;
        params.useReverse = m_params.useReverse;
        params.lockVideoOrientation = m_params.lockVideoOrientation;
        params.stayAwake = m_params.stayAwake;
        m_server->start(params);
    });
}

void Device::onSetControlState(Device *device, Device::GroupControlState state)
{
    Q_UNUSED(device)
    if (m_controlState == state) {
        return;
    }
    GroupControlState oldState = m_controlState;
    m_controlState = state;
    emit controlStateChange(this, oldState, m_controlState);
}

void Device::onGrabCursor(bool grab)
{
    if (!m_videoForm) {
        return;
    }
    if (m_controlState == GCS_CLIENT) {
        return;
    }
    QRect rc = m_videoForm->getGrabCursorRect();
    MouseTap::getInstance()->enableMouseEventTap(rc, grab);
}

Device::GroupControlState Device::controlState()
{
    return m_controlState;
}

bool Device::isCurrentCustomKeymap()
{
    if (!m_controller) {
        return false;
    }
    return m_controller->isCurrentCustomKeymap();
}

bool Device::saveFrame(const AVFrame *frame)
{
    if (!frame) {
        return false;
    }

    // create buffer
    QImage rgbImage(frame->width, frame->height, QImage::Format_RGB32);
    AVFrame *rgbFrame = av_frame_alloc();
    if (!rgbFrame) {
        return false;
    }

    // bind buffer to AVFrame
    av_image_fill_arrays(rgbFrame->data, rgbFrame->linesize, rgbImage.bits(), AV_PIX_FMT_RGB32, frame->width, frame->height, 4);

    // convert
    AVFrameConvert convert;
    convert.setSrcFrameInfo(frame->width, frame->height, AV_PIX_FMT_YUV420P);
    convert.setDstFrameInfo(frame->width, frame->height, AV_PIX_FMT_RGB32);
    bool ret = false;
    ret = convert.init();
    if (!ret) {
        return false;
    }
    ret = convert.convert(frame, rgbFrame);
    if (!ret) {
        return false;
    }
    convert.deInit();
    av_free(rgbFrame);

    // save
    QString absFilePath;
    QString fileDir(Config::getInstance().getRecordPath());
    if (fileDir.isEmpty()) {
        qWarning() << "please select record save path!!!";
        return false;
    }
    QDateTime dateTime = QDateTime::currentDateTime();
    QString fileName = dateTime.toString("_yyyyMMdd_hhmmss_zzz");
    fileName = Config::getInstance().getTitle() + fileName + ".png";
    QDir dir(fileDir);
    absFilePath = dir.absoluteFilePath(fileName);
    ret = rgbImage.save(absFilePath, "PNG", 100);
    if (!ret) {
        return false;
    }

    qInfo() << "screenshot save to " << absFilePath;
    return true;
}
