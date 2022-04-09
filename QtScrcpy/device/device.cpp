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
        deleteLater();
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
        m_recorder = new Recorder(m_params.recordFileName);
    }
    initSignals();
    startServer();
}

Device::~Device()
{
    if (m_server) {
        m_server->stop();
    }

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
        delete m_recorder;
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
    if (!m_decoder) {
        return;
    }

    // screenshot
    m_decoder->peekFrame([this](int width, int height, uint8_t* dataRGB32) {
       saveFrame(width, height, dataRGB32);
    });
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
            Q_UNUSED(deviceName);
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

void Device::startServer()
{
    // fix: macos cant recv finished signel, timer is ok
    QTimer::singleShot(0, this, [this]() {
        m_startTimeCount.start();
        // max size support 480p 720p 1080p 设备原生分辨率
        // support wireless connect, example:
        //m_server->start("192.168.0.174:5555", 27183, m_maxSize, m_bitRate, "");
        // only one devices, serial can be null
        // mark: crop input format: "width:height:x:y" or "" for no crop, for example: "100:200:0:0"
        Server::ServerParams params;
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
