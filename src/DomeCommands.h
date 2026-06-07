#ifndef __PENUMBRA_DOMECOMMANDS_H__
#define __PENUMBRA_DOMECOMMANDS_H__

#include "ReelTwo.h"
#include "WCBSerial.h"

class DomeCommands
{
public:
    DomeCommands(WCBSerialCommandWriter& writer, int wcbId = -1, int serialPort = -1) :
        fWriter(writer),
        fWCBId(wcbId),
        fSerialPort(serialPort)
    {
    }

    size_t home()
    {
        return send(":DPH");
    }

    size_t homeWithSpeed(int speedPercent)
    {
        String cmd = ":DPH";
        cmd += speedPercent;
        return send(cmd);
    }

    size_t rotateSpeed(int speedPercent)
    {
        String cmd = ":DPR";
        cmd += speedPercent;
        return send(cmd);
    }

    size_t rotateRandomSpeed(int maxSpeedPercent = 0)
    {
        String cmd = ":DPRR";
        if (maxSpeedPercent > 0)
            cmd += maxSpeedPercent;
        return send(cmd);
    }

    size_t rotateAbsolute(int degrees, int speedPercent = -1, bool noWait = false)
    {
        return sendTargetCommand('A', String(degrees), speedPercent, noWait);
    }

    size_t rotateAbsoluteRandom(int speedPercent = -1, bool noWait = false)
    {
        return sendTargetCommand('A', "R", speedPercent, noWait);
    }

    size_t rotateRelative(int degrees, int speedPercent = -1, bool noWait = false)
    {
        return sendTargetCommand('D', String(degrees), speedPercent, noWait);
    }

    size_t rotateRelativeRandom(int speedPercent = -1, bool noWait = false)
    {
        return sendTargetCommand('D', "R", speedPercent, noWait);
    }

private:
    size_t send(const char* cmd)
    {
        return fWriter.sendLine(cmd, fWCBId, fSerialPort);
    }

    size_t send(const String& cmd)
    {
        return fWriter.sendLine(cmd, fWCBId, fSerialPort);
    }

    size_t sendTargetCommand(char mode, const String& target, int speedPercent, bool noWait)
    {
        String cmd = ":DP";
        cmd += mode;
        cmd += target;
        if (speedPercent >= 0)
        {
            cmd += ",";
            cmd += speedPercent;
        }
        if (noWait)
            cmd += "+";
        return send(cmd);
    }

    WCBSerialCommandWriter& fWriter;
    int fWCBId = -1;
    int fSerialPort = -1;
};

#endif
