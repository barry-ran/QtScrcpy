#include <QtTest>
#include <QFile>
#include <QCoreApplication>
#include <QElapsedTimer>
#include "../../QtScrcpy/audio/audiooutput.h"
class PartialWriter : public QIODevice {
public:
    QByteArray received;
    PartialWriter() { open(QIODevice::WriteOnly); }
    qint64 readData(char *, qint64) override { return -1; }
    qint64 writeData(const char *data, qint64 count) override {
        const qint64 n = qMin<qint64>(count, 4);
        received.append(data, int(n)); return n;
    }
};
class AudioStartTest : public QObject {
    Q_OBJECT
private slots:
    void retainsPartialWritesAndBoundsLatency() {
        PcmBuffer pcm;
        PartialWriter output;
        pcm.append("abcdefghijkl");
        QCOMPARE(pcm.drain(&output, 0), qint64(0));
        QCOMPARE(pcm.size(), 12);
        QCOMPARE(pcm.drain(&output, 12), qint64(4));
        QCOMPARE(pcm.size(), 8);
        pcm.drain(&output, 8); pcm.drain(&output, 4);
        QCOMPARE(output.received, QByteArray("abcdefghijkl"));
        pcm.append(QByteArray(50000, 'x'));
        QVERIFY(pcm.size() <= 38400);
        QCOMPARE(pcm.size() % 4, 0);
        pcm.clear(); QCOMPARE(pcm.size(), 0);
    }

    void pendingStartupDoesNotBlockUi() {
        QFile script(QCoreApplication::applicationDirPath() + "/sndcpy.sh");
        QVERIFY(script.open(QIODevice::WriteOnly));
        script.write("#!/bin/bash\nexec sleep 2\n"); script.close();
        AudioOutput audio;
        QElapsedTimer elapsed; elapsed.start();
        QVERIFY(audio.start("test-device", 28200));
        QVERIFY2(elapsed.elapsed() < 200, "Audio startup blocked the UI thread");
        bool responsive = false;
        QTimer::singleShot(20, [&]() { responsive = true; });
        QTest::qWait(100);
        QVERIFY(responsive);
        QVERIFY(!audio.start("test-device", 28200));
        audio.stopForDevice("another-device");
        QVERIFY(audio.isStarting());
        QCOMPARE(audio.serial(),QString("test-device"));
        audio.stopForDevice("test-device");
        QVERIFY(!audio.isActive());
        QVERIFY(audio.serial().isEmpty());
        audio.stop();
        QTest::qWait(100);
        QVERIFY(audio.start("test-device", 28200));
        audio.stop();
        QTest::qWait(100);
    }
};
QTEST_GUILESS_MAIN(AudioStartTest)
#include "audio-start-test.moc"
