#pragma once
#include <QByteArray>
#include <QIODevice>
#include <QtGlobal>

// 48 kHz stereo signed 16-bit PCM: 192 bytes per millisecond.
class PcmBuffer {
public:
    void append(const QByteArray &data) {
        bytes.append(data);
        const int excess = bytes.size() - 38400; // Bound queued delay to 200 ms.
        if (excess > 0) bytes.remove(0, ((excess + 3) / 4) * 4);
    }
    qint64 drain(QIODevice *output, qint64 freeBytes) {
        qint64 count = qMin<qint64>(bytes.size(), freeBytes);
        count -= count % 4;
        if (!output || count <= 0) return 0;
        const qint64 written = output->write(bytes.constData(), count);
        if (written > 0) bytes.remove(0, int(written));
        return written;
    }
    int size() const { return bytes.size(); }
    void clear() { bytes.clear(); }
private:
    QByteArray bytes;
};
