#include "bufferutil.h"

void BufferUtil::write32(QBuffer &buffer, quint32 value)
{
    buffer.putChar(value >> 24);
    buffer.putChar(value >> 16);
    buffer.putChar(value >> 8);
    buffer.putChar(value);
}

void BufferUtil::write64(QBuffer &buffer, quint64 value)
{
    write32(buffer, value >> 32);
    write32(buffer, (quint32)value);
}

void BufferUtil::write16(QBuffer &buffer, quint32 value)
{
    buffer.putChar(value >> 8);
    buffer.putChar(value);
}

quint16 BufferUtil::read16(QBuffer &buffer)
{
    uchar c;
    quint16 ret = 0;
    buffer.getChar(reinterpret_cast<char *>(&c));
    ret |= (c << 8);
    buffer.getChar(reinterpret_cast<char *>(&c));
    ret |= c;

    return ret;
}

quint32 BufferUtil::read32(QBuffer &buffer)
{
    uchar c;
    quint32 ret = 0;
    buffer.getChar(reinterpret_cast<char *>(&c));
    ret |= (c << 24);
    buffer.getChar(reinterpret_cast<char *>(&c));
    ret |= (c << 16);
    buffer.getChar(reinterpret_cast<char *>(&c));
    ret |= (c << 8);
    buffer.getChar(reinterpret_cast<char *>(&c));
    ret |= c;

    return ret;
}

quint64 BufferUtil::read64(QBuffer &buffer)
{
    quint32 msb = read32(buffer);
    quint32 lsb = read32(buffer);

    return ((quint64)msb << 32) | lsb;
    ;
}
