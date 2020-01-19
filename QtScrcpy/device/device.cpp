#include <QTimer>
#include <QMessageBox>

#include "device.h"
#include "recorder.h"
#include "server.h"
#include "videobuffer.h"
#include "decoder.h"
#include "filehandler.h"
#include "stream.h"
#include "videoform.h"
#include "controller.h"
#include "config.h"

Device::Device(DeviceParams params, QObject *parent)
    : QObject(parent)
    , m_params(params)
{
    if (!params.display && m_params.recordFileName.trimmed().isEmpty()) {
        qCritical("not display must be recorded");
        deleteLater();
        return;
    }

    if (params.display) {
        m_vb = new VideoBuffer();
        m_vb->init();
        m_decoder = new Decoder(m_vb, this);
        m_fileHandler = new FileHandler(this);
        m_controller = new Controller(params.gameScript, this);
        m_videoForm = new VideoForm(Config::getInstance().getSkin());
        m_videoForm->setSerial(m_params.serial);
        if (m_controller) {
            m_videoForm->setController(m_controller);
        }
        if (m_fileHandler) {
            m_videoForm->setFileHandler(m_fileHandler);
        }
        m_videoForm->show();
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
        delete m_videoForm;
    }
    emit deviceDisconnect(m_params.serial);
}

VideoForm *Device::getVideoForm()
{
    return m_videoForm;
}

Controller *Device::getController()
{
    return m_controller;
}

Server *Device::getServer()
{
    return m_server;
}

void Device::updateScript(QString script)
{
    if(m_controller){
        m_controller->updateScript(script);
    }
}

void Device::initSignals()
{
    if (m_controller && m_videoForm) {
        connect(m_controller, &Controller::grabCursor, m_videoForm, &VideoForm::onGrabCursor);
    }
    if (m_videoForm) {
        connect(m_videoForm, &VideoForm::destroyed, this, [this](QObject *obj){
            Q_UNUSED(obj);
            deleteLater();
        });
    }
    if (m_fileHandler) {
        connect(m_fileHandler, &FileHandler::fileHandlerResult, this, [this](FileHandler::FILE_HANDLER_RESULT processResult, bool isApk){
            QString tips = "";
            if (isApk) {
                tips = tr("install apk");
            } else {
                tips = tr("file transfer");
            }
            if (FileHandler::FAR_IS_RUNNING == processResult && m_videoForm) {
                QMessageBox::warning(m_videoForm, "QtScrcpy", tr("wait current %1 to complete").arg(tips), QMessageBox::Ok);
            }
            if (FileHandler::FAR_SUCCESS_EXEC == processResult && m_videoForm) {
                QMessageBox::information(m_videoForm, "QtScrcpy", tr("%1 complete, save in %2").arg(tips).arg(m_fileHandler->getDevicePath()), QMessageBox::Ok);
            }
            if (FileHandler::FAR_ERROR_EXEC == processResult && m_videoForm) {
                QMessageBox::information(m_videoForm, "QtScrcpy", tr("%1 failed").arg(tips), QMessageBox::Ok);
            }
        });
    }

    if (m_server) {
        connect(m_server, &Server::serverStartResult, this, [this](bool success){
            if (success) {
                m_server->connectTo();
            } else {
                deleteLater();
            }
        });
        connect(m_server, &Server::connectToResult, this, [this](bool success, const QString &deviceName, const QSize &size){
            if (success) {
                float diff = m_startTimeCount.elapsed() / 1000.0f;
                qInfo(QString("server start finish in %1s").arg(diff).toStdString().c_str());

                // update ui
                if (m_videoForm) {
                    m_videoForm->setWindowTitle(deviceName);
                    m_videoForm->updateScreenRatio(size);
                    m_videoForm->updateShowSize(size);
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
                    m_controller->setScreenPowerMode(ControlMsg::SPM_OFF);
                }
            }
        });
        connect(m_server, &Server::onServerStop, this, [this](){
            deleteLater();
            qDebug() << "server process stop";
        });
    }

    if (m_stream) {
        connect(m_stream, &Stream::onStreamStop, this, [this](){
            deleteLater();
            qDebug() << "stream thread stop";
        });
    }

    if (m_decoder && m_vb) {
        // must be Qt::QueuedConnection, ui update must be main thread
        connect(m_decoder, &Decoder::onNewFrame, this, [this](){
            m_vb->lock();
            const AVFrame *frame = m_vb->consumeRenderedFrame();
            if (m_videoForm) {
                m_videoForm->updateRender(frame);
            }
            m_vb->unLock();
        },Qt::QueuedConnection);
    }
}

void Device::startServer()
{
    // fix: macos cant recv finished signel, timer is ok
    QTimer::singleShot(0, this, [this](){
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
        m_server->start(params);
    });
}
