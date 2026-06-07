#ifndef __PENUMBRA_WCBSERIAL_H__
#define __PENUMBRA_WCBSERIAL_H__

#include "ReelTwo.h"

class WCBSerialCommandWriter
{
public:
    explicit WCBSerialCommandWriter(Stream& serial) :
        fSerial(serial)
    {
    }

    size_t sendRaw(const char* cmd)
    {
        return fSerial.print(cmd);
    }

    size_t sendRaw(const String& cmd)
    {
        return fSerial.print(cmd);
    }

    size_t sendLine(const char* cmd, int wcbId = -1, int serialPort = -1)
    {
        size_t written = sendRoutePrefix(wcbId, serialPort);
        written += sendRaw(cmd);
        written += fSerial.print("\r\n");
        return written;
    }

    size_t sendLine(const String& cmd, int wcbId = -1, int serialPort = -1)
    {
        size_t written = sendRoutePrefix(wcbId, serialPort);
        written += sendRaw(cmd);
        written += fSerial.print("\r\n");
        return written;
    }

private:
    size_t sendRoutePrefix(int wcbId, int serialPort)
    {
        size_t written = 0;
        if (wcbId >= 0)
        {
            written += fSerial.print(";W");
            written += fSerial.print(wcbId);
            written += fSerial.print(",");
        }
        if (serialPort >= 0)
        {
            written += fSerial.print(";S");
            written += fSerial.print(serialPort);
        }
        return written;
    }

    Stream& fSerial;
};

#endif
