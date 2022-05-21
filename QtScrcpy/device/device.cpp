#include <QDir>
#include <QMessageBox>
#include <QTimer>

#include "config.h"
#include "controller.h"
#include "devicemsg.h"
#include "decoder.h"
#include "device.h"
#include "filehandler.h"
#include "mousetap/mousetap.h"
#include "recorder.h"
#include "server.h"
#include "stream.h"
#include "videoform.h"

Device::Device(DeviceParams params, QObject *parent) : QObject(parent), m_params(params)
{
    if (!params.display && m_params.recordFileName.trimmed().isEmpty()) {
        qCritical("not display must be recorded");
        return;
    }

    if (params.display) {
        m_decoder = new Decoder([this](int width, int height, uint8_t* dataY, uint8_t* dataU, uint8_t* dataV, int linesizeY, int linesizeU, int linesizeV) {
            if (m_videoForm) {
                m_videoForm->updateRender(width, height, dataY, dataU, dataV, linesizeY, linesizeU, linesizeV);
            }
        }, this);
        m_fileHandler = new FileHandler(this);
        m_controller = new Controller([this](const QByteArray& buffer) -> qint64 {
            if (!m_server || !m_server->getControlSocket()) {
                return 0;
            }

            return m_server->getControlSocket()->write(buffer.data(), buffer.length());
        }, params.gameScript, this);
        m_videoForm = new VideoForm(params.framelessWindow, Config::getInstance().getSkin());
        m_videoForm->setDevice(this);
    }

    m_stream = new Stream([this](quint8 *buf, qint32 bufSize) -> qint32 {
        auto videoSocket = m_server->getVideoSocket();
        if (!videoSocket) {
            return 0;
        }

        return videoSocket->subThreadRecvData(buf, bufSize);
    }, this);

    m_server = new Server(this);
    if (!m_params.recordFileName.trimmed().isEmpty()) {
        m_recorder = new Recorder(m_params.recordFileName, this);
    }
    initSignals();
}

Device::~Device()
{
    disconnectDevice();
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

void Device::screenshot()
{
    if (!m_decoder) {
        return;
    }

    // screenshot
    m_decoder->peekFrame([this](int width, int height, uint8_t* dataRGB32) {
       saveFrame(width, height, dataRGB32);
    });
}

void Device::showTouch(bool show)
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
    if (m_controller) {
        connect(m_controller, &Controller::grabCursor, this, &Device::grabCursor);
    }
    if (m_controller) {
        /*
        connect(this, &Device::postAppSwitch, m_controller, &Controller::onPostAppSwitch);
        connect(this, &Device::postPower, m_controller, &Controller::onPostPower);
        connect(this, &Device::postVolumeUp, m_controller, &Controller::onPostVolumeUp);
        connect(this, &Device::postVolumeDown, m_controller, &Controller::onPostVolumeDown);
        connect(this, &Device::postCopy, m_controller, &Controller::onCopy);
        connect(this, &Device::postCut, m_controller, &Controller::onCut);
        connect(this, &Device::setScreenPowerMode, m_controller, &Controller::onSetScreenPowerMode);
        connect(this, &Device::expandNotificationPanel, m_controller, &Controller::onExpandNotificationPanel);
        connect(this, &Device::collapsePanel, m_controller, &Controller::onCollapsePanel);
        connect(this, &Device::mouseEvent, m_controller, &Controller::onMouseEvent);
        connect(this, &Device::wheelEvent, m_controller, &Controller::onWheelEvent);
        connect(this, &Device::keyEvent, m_controller, &Controller::onKeyEvent);

        connect(this, &Device::postBackOrScreenOn, m_controller, &Controller::onPostBackOrScreenOn);
        connect(this, &Device::setDeviceClipboard, m_controller, &Controller::onSetDeviceClipboard);
        connect(this, &Device::clipboardPaste, m_controller, &Controller::onClipboardPaste);
        connect(this, &Device::postTextInput, m_controller, &Controller::onPostTextInput);
        */
    }
    if (m_fileHandler) {
        connect(m_fileHandler, &FileHandler::fileHandlerResult, this, [this](FileHandler::FILE_HANDLER_RESULT processResult, bool isApk) {
            QString tipsType = "";
            if (isApk) {
                tipsType = tr("install apk");
            } else {
                tipsType = tr("file transfer");
            }
            QString tips;
            if (FileHandler::FAR_IS_RUNNING == processResult) {
                tips = tr("wait current %1 to complete").arg(tipsType);
            }
            if (FileHandler::FAR_SUCCESS_EXEC == processResult) {
                tips = tr("%1 complete, save in %2").arg(tipsType).arg(Config::getInstance().getPushFilePath());
            }
            if (FileHandler::FAR_ERROR_EXEC == processResult) {
                tips = tr("%1 failed").arg(tipsType);
            }
            qInfo() << tips;
        });
    }

    if (m_server) {
        connect(m_server, &Server::serverStarted, this, [this](bool success, const QString &deviceName, const QSize &size) {
            emit deviceConnected(success, m_params.serial, deviceName, size);
            if (success) {
                double diff = m_startTimeCount.elapsed() / 1000.0;
                qInfo() << QString("server start finish in %1s").arg(diff).toStdString().c_str();


                // update ui
                if (m_videoForm) {
                    // must be show before updateShowSize
                    m_videoForm->show();
                    QString name = Config::getInstance().getNickName(m_params.serial);
                    if (name.isEmpty()) {
                        name = Config::getInstance().getTitle();
                    }
                    m_videoForm->setWindowTitle(name + "-" + m_params.serial);
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
                    if (!m_recorder->open()) {
                        qCritical("Could not open recorder");
                    }

                    if (!m_recorder->startRecorder()) {
                        qCritical("Could not start recorder");
                    }
                }

                // init decoder
                if (m_decoder) {
                    m_decoder->open();
                }

                // init decoder
                m_stream->startDecode();

                // recv device msg
                connect(m_server->getControlSocket(), &QTcpSocket::readyRead, this, [this](){
                    if (!m_controller) {
                        return;
                    }

                    auto controlSocket = m_server->getControlSocket();
                    while (controlSocket->bytesAvailable()) {
                        QByteArray byteArray = controlSocket->peek(controlSocket->bytesAvailable());
                        DeviceMsg deviceMsg;
                        qint32 consume = deviceMsg.deserialize(byteArray);
                        if (0 >= consume) {
                            break;
                        }
                        controlSocket->read(consume);
                        m_controller->recvDeviceMsg(&deviceMsg);
                    }
                });

                // 显示界面时才自动息屏（m_params.display）
                if (m_params.closeScreen && m_params.display && m_controller) {
                    m_controller->setScreenPowerMode(ControlMsg::SPM_OFF);
                }
            } else {
                m_server->stop();
            }
        });
        connect(m_server, &Server::serverStoped, this, [this]() {
            disconnectDevice();
            qDebug() << "server process stop";
        });
    }

    if (m_stream) {
        connect(m_stream, &Stream::onStreamStop, this, [this]() {
            disconnectDevice();
            qDebug() << "stream thread stop";
        });
        connect(m_stream, &Stream::getFrame, this, [this](AVPacket *packet) {
            if (m_decoder && !m_decoder->push(packet)) {
                qCritical("Could not send packet to decoder");
            }

            if (m_recorder && !m_recorder->push(packet)) {
                qCritical("Could not send packet to recorder");
            }
        }, Qt::DirectConnection);
        connect(m_stream, &Stream::getConfigFrame, this, [this](AVPacket *packet) {
            if (m_recorder && !m_recorder->push(packet)) {
                qCritical("Could not send config packet to recorder");
            }
        }, Qt::DirectConnection);
    }

    if (m_decoder) {
        connect(m_decoder, &Decoder::updateFPS, m_videoForm, &VideoForm::updateFPS);
    }
}

bool Device::connectDevice()
{
    if (!m_server) {
        return false;
    }

    // fix: macos cant recv finished signel, timer is ok
    QTimer::singleShot(0, this, [this]() {
        m_startTimeCount.start();
        // max size support 480p 720p 1080p 设备原生分辨率
        // support wireless connect, example:
        //m_server->start("192.168.0.174:5555", 27183, m_maxSize, m_bitRate, "");
        // only one devices, serial can be null
        // mark: crop input format: "width:height:x:y" or "" for no crop, for example: "100:200:0:0"
        Server::ServerParams params;
        params.serverLocalPath = m_params.serverLocalPath;
        params.serverRemotePath = m_params.serverRemotePath;
        params.serial = m_params.serial;
        params.localPort = m_params.localPort;
        params.maxSize = m_params.maxSize;
        params.bitRate = m_params.bitRate;
        params.maxFps = m_params.maxFps;
        params.crop = "";
        params.control = true;
        params.useReverse = m_params.useReverse;
        params.lockVideoOrientation = m_params.lockVideoOrientation;
        params.stayAwake = m_params.stayAwake;
        m_server->start(params);
    });

    return true;
}

void Device::disconnectDevice()
{
    if (!m_server) {
        return;
    }
    m_server->stop();
    m_server = Q_NULLPTR;

    if (m_stream) {
        m_stream->stopDecode();
    }

    // server must stop before decoder, because decoder block main thread
    if (m_decoder) {
        m_decoder->close();
    }

    if (m_recorder) {
        if (m_recorder->isRunning()) {
            m_recorder->stopRecorder();
            m_recorder->wait();
        }
        m_recorder->close();
    }
    if (m_videoForm) {
        m_videoForm->close();
        m_videoForm->deleteLater();
    }

    emit deviceDisconnected(m_params.serial);
}

void Device::postGoBack()
{
    if (!m_controller) {
        return;
    }
    m_controller->postGoBack();
}

void Device::postGoHome()
{
    if (!m_controller) {
        return;
    }
    m_controller->postGoHome();
}

void Device::postGoMenu()
{
    if (!m_controller) {
        return;
    }
    m_controller->postGoMenu();
}

void Device::postAppSwitch()
{
    if (!m_controller) {
        return;
    }
    m_controller->postAppSwitch();
}

void Device::postPower()
{
    if (!m_controller) {
        return;
    }
    m_controller->postPower();
}

void Device::postVolumeUp()
{
    if (!m_controller) {
        return;
    }
    m_controller->postVolumeUp();
}

void Device::postVolumeDown()
{
    if (!m_controller) {
        return;
    }
    m_controller->postVolumeDown();
}

void Device::postCopy()
{
    if (!m_controller) {
        return;
    }
    m_controller->copy();
}

void Device::postCut()
{
    if (!m_controller) {
        return;
    }
    m_controller->cut();
}

void Device::setScreenPowerMode(ControlMsg::ScreenPowerMode mode)
{
    if (!m_controller) {
        return;
    }
    m_controller->setScreenPowerMode(mode);
}

void Device::expandNotificationPanel()
{
    if (!m_controller) {
        return;
    }
    m_controller->expandNotificationPanel();
}

void Device::collapsePanel()
{
    if (!m_controller) {
        return;
    }
    m_controller->collapsePanel();
}

void Device::postBackOrScreenOn(bool down)
{
    if (!m_controller) {
        return;
    }
    m_controller->postBackOrScreenOn(down);
}

void Device::postTextInput(QString &text)
{
    if (!m_controller) {
        return;
    }
    m_controller->postTextInput(text);
}

void Device::requestDeviceClipboard()
{
    if (!m_controller) {
        return;
    }
    m_controller->requestDeviceClipboard();
}

void Device::setDeviceClipboard(bool pause)
{
    if (!m_controller) {
        return;
    }
    m_controller->setDeviceClipboard(pause);
}

void Device::clipboardPaste()
{
    if (!m_controller) {
        return;
    }
    m_controller->clipboardPaste();
}

void Device::pushFileRequest(const QString &file, const QString &devicePath)
{
    if (!m_fileHandler) {
        return;
    }
    m_fileHandler->onPushFileRequest(getSerial(), file, devicePath);
}

void Device::installApkRequest(const QString &apkFile)
{
    if (!m_fileHandler) {
        return;
    }
    m_fileHandler->onInstallApkRequest(getSerial(), apkFile);
}

void Device::mouseEvent(const QMouseEvent *from, const QSize &frameSize, const QSize &showSize)
{
    if (!m_controller) {
        return;
    }
    m_controller->mouseEvent(from, frameSize, showSize);
}

void Device::wheelEvent(const QWheelEvent *from, const QSize &frameSize, const QSize &showSize)
{
    if (!m_controller) {
        return;
    }
    m_controller->wheelEvent(from, frameSize, showSize);
}

void Device::keyEvent(const QKeyEvent *from, const QSize &frameSize, const QSize &showSize)
{
    if (!m_controller) {
        return;
    }
    m_controller->keyEvent(from, frameSize, showSize);
}

void Device::grabCursor(bool grab)
{
    if (!m_videoForm) {
        return;
    }
    QRect rc = m_videoForm->getGrabCursorRect();
    MouseTap::getInstance()->enableMouseEventTap(rc, grab);
}

bool Device::isCurrentCustomKeymap()
{
    if (!m_controller) {
        return false;
    }
    return m_controller->isCurrentCustomKeymap();
}

bool Device::saveFrame(int width, int height, uint8_t* dataRGB32)
{
    if (!dataRGB32) {
        return false;
    }

    QImage rgbImage(dataRGB32, width, height, QImage::Format_RGB32);

    // save
    QString absFilePath;
    QString fileDir(m_params.recordPath);
    if (fileDir.isEmpty()) {
        qWarning() << "please select record save path!!!";
        return false;
    }
    QDateTime dateTime = QDateTime::currentDateTime();
    QString fileName = dateTime.toString("_yyyyMMdd_hhmmss_zzz");
    fileName = Config::getInstance().getTitle() + fileName + ".png";
    QDir dir(fileDir);
    absFilePath = dir.absoluteFilePath(fileName);
    int ret = rgbImage.save(absFilePath, "PNG", 100);
    if (!ret) {
        return false;
    }

    qInfo() << "screenshot save to " << absFilePath;
    return true;
}
