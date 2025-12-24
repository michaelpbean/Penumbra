#ifdef USE_BLUEPAD
#include <Arduino.h>
#include <Bluepad32.h>
#include "BluepadController.h"

ControllerPtr gAllControllers[BP32_MAX_GAMEPADS];
BluepadController* gAllBluepadControllers[BP32_MAX_GAMEPADS]; // These are vitual controllers, so could be more than number of physical controllers, but cap the same for now.

///////////////////////////////////////////////////////////////////////////////
void onConnected(ControllerPtr ctl)
{
    // Find a free slot in the controller list
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++)
    {
        if (gAllControllers[i] == nullptr)
        {
            gAllControllers[i] = ctl;

            // Assign to BluePadControllers
            for (int i = 0; i < BP32_MAX_GAMEPADS; i++)
            {
                if (gAllBluepadControllers[i] != nullptr)
                    gAllBluepadControllers[i]->assignBP32Controller(i, ctl);
            }

            ControllerProperties props = ctl->getProperties();
            Serial.printf("Controller connected at slot %d: %s\n", i, ctl->getModelName().c_str());
            Serial.printf("  VID=0x%04x PID=0x%04x\n", props.vendor_id, props.product_id);
            return;
        }
    }
    Serial.println("Controller connected, but no empty slot!");
}

///////////////////////////////////////////////////////////////////////////////
void onDisconnected(ControllerPtr ctl)
{
    // Remove from myControllers array
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++)
    {
        if (gAllControllers[i] == ctl)
        {
            Serial.printf("Controller disconnected from slot %d: %s\n", i, ctl->getModelName().c_str());
            gAllControllers[i] = nullptr;

            // Clear any assignments to BluePadControllers that match
            for (int i = 0; i < BP32_MAX_GAMEPADS; i++)
            {
                if (gAllBluepadControllers[i] != nullptr)
                    gAllBluepadControllers[i]->clearBP32Controller(i);
            }

            return;
        }
    }
    Serial.printf("Controller disconnected (unknown): %s\n", ctl->getModelName().c_str());
}

///////////////////////////////////////////////////////////////////////////////
// Debug stuff
#if 0
void dumpGamepad(ControllerPtr ctl)
{
    Serial.printf(
        "idx=%d, dpad: 0x%02x, buttons: 0x%04x, axis L: %4d, %4d, axis R: %4d, %4d, brake: %4d, throttle: %4d, "
        "misc: 0x%02x, gyro x:%6d y:%6d z:%6d, accel x:%6d y:%6d z:%6d\n",
        ctl->index(),       // Controller Index
        ctl->dpad(),        // D-pad
        ctl->buttons(),     // bitmask of pressed buttons
        ctl->axisX(),       // (-511 - 512) left X Axis
        ctl->axisY(),       // (-511 - 512) left Y axis
        ctl->axisRX(),      // (-511 - 512) right X axis
        ctl->axisRY(),      // (-511 - 512) right Y axis
        ctl->brake(),       // (0 - 1023): brake button
        ctl->throttle(),    // (0 - 1023): throttle (AKA gas) button
        ctl->miscButtons(), // bitmask of pressed "misc" buttons
        ctl->gyroX(),       // Gyro X
        ctl->gyroY(),       // Gyro Y
        ctl->gyroZ(),       // Gyro Z
        ctl->accelX(),      // Accelerometer X
        ctl->accelY(),      // Accelerometer Y
        ctl->accelZ()       // Accelerometer Z
    );
}

///////////////////////////////////////////////////////////////////////////////
void processGamepad(ControllerPtr ctl)
{
    // There are different ways to query whether a button is pressed.
    // By query each button individually:
    //  a(), b(), x(), y(), l1(), etc...
    if (ctl->a())
    {
        static int colorIdx = 0;
        // Some gamepads like DS4 and DualSense support changing the color LED.
        // It is possible to change it by calling:
        switch (colorIdx % 3)
        {
        case 0:
            // Red
            ctl->setColorLED(255, 0, 0);
            break;
        case 1:
            // Green
            ctl->setColorLED(0, 255, 0);
            break;
        case 2:
            // Blue
            ctl->setColorLED(0, 0, 255);
            break;
        }
        colorIdx++;
    }

    if (ctl->b())
    {
        // Turn on the 4 LED. Each bit represents one LED.
        static int led = 0;
        led++;
        // Some gamepads like the DS3, DualSense, Nintendo Wii, Nintendo Switch
        // support changing the "Player LEDs": those 4 LEDs that usually indicate
        // the "gamepad seat".
        // It is possible to change them by calling:
        ctl->setPlayerLEDs(led & 0x0f);
    }

    if (ctl->x())
    {
        // Some gamepads like DS3, DS4, DualSense, Switch, Xbox One S, Stadia support rumble.
        // It is possible to set it by calling:
        // Some controllers have two motors: "strong motor", "weak motor".
        // It is possible to control them independently.
        ctl->playDualRumble(0 /* delayedStartMs */, 250 /* durationMs */, 0x80 /* weakMagnitude */,
                            0x40 /* strongMagnitude */);
    }

    // Another way to query controller data is by getting the buttons() function.
    // See how the different "dump*" functions dump the Controller info.
    dumpGamepad(ctl);
}
#endif

///////////////////////////////////////////////////////////////////////////////
void processControllers()
{
    // Update state in BluepadControllers
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++)
    {
        BluepadController* pCtl = gAllBluepadControllers[i];
        if (pCtl)
            pCtl->updateState();
    }
}

///////////////////////////////////////////////////////////////////////////////
void BluepadController::startup()
{
    // Initialize our controller slots
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++)
    {
        gAllControllers[i] = nullptr;
    }
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++)
    {
        gAllBluepadControllers[i] = nullptr;
    }

    Serial.begin(115200);
    Serial.println("Bluepad32 startup");
    BP32.setup(onConnected, onDisconnected);
}

///////////////////////////////////////////////////////////////////////////////
void BluepadController::registerBluepadController(BluepadController* pCtl)
{
    if (!pCtl)
        return;

    // Add to global list
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++)
    {
        if (gAllBluepadControllers[i] == nullptr)
        {
            gAllBluepadControllers[i] = pCtl;
            return;
        }
    }

    Serial.println("BluepadController not registered, no empty slot!");
}

///////////////////////////////////////////////////////////////////////////////
void BluepadController::update()
{
    bool dataUpdated = BP32.update();

    if (dataUpdated)
        processControllers();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
BluepadController::BluepadController()
{
    mType = kPS5;
    mBluepadControllerID = -1;
}

///////////////////////////////////////////////////////////////////////////////
BluepadController::BluepadController(const char* mac, Type type)
{
    mType = type;
    mBluepadControllerID = -1;
}

///////////////////////////////////////////////////////////////////////////////
BluepadController::~BluepadController()
{
    // Remove from global list
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++)
    {
        if (gAllBluepadControllers[i] == this)
        {
            gAllBluepadControllers[i] = nullptr;
            return;
        }
    }

    Serial.println("BluepadController not found in global list!");
}


///////////////////////////////////////////////////////////////////////////////
void BluepadController::setPlayer(int player)
{

}

///////////////////////////////////////////////////////////////////////////////
void BluepadController::assignBP32Controller(int id, Controller *pController)
{
    Serial.printf("assignBP32Controller\n");

    if (id < 0 || pController == nullptr)
    {
        Serial.printf("Trying to assign invalid BP32 controller to BluepadController");
        return;
    }
    // Already assigned
    if (mBluepadControllerID >= 0)
        return;

    mBluepadControllerID = id;
    onConnect();
    fConnected = 1;

    Serial.printf("Assigned = %d, %d\n", mBluepadControllerID, fConnected);
}

///////////////////////////////////////////////////////////////////////////////
void BluepadController::clearBP32Controller(int id)
{
    Serial.printf("clearBP32Controller");
    if (mBluepadControllerID == id)
    {
        mBluepadControllerID = -1;
        onDisconnect();
    }
}

#define CHECK_FLAG(mask) ((raw & mask) != 0)
#define CHECK_FLAG2(value, mask) ((value & mask) != 0)
#define CHECK_BUTTON_DOWN(b) evt.button_down.b = (!prev.button.b && fState.button.b)
#define CHECK_BUTTON_UP(b) evt.button_up.b = (prev.button.b && !fState.button.b)

///////////////////////////////////////////////////////////////////////////////
void BluepadController::updateState()
{
    // No physical controller connected/assigned
    if (mBluepadControllerID < 0)
        return;

    ControllerPtr pBP32Ctl = gAllControllers[mBluepadControllerID];
    if (!pBP32Ctl)
    {
        Serial.printf("No valid/connected BP32 controller assigned to BluepadController ID = %d\n", mBluepadControllerID);
        return;
    }
    if (!pBP32Ctl->isConnected())
    {
        Serial.printf("BP32 controller disconnected\n");
        return;
    }
    if (!pBP32Ctl->hasData())
        return;

    Event evt = {};
    State prev = fState;

    fState.button.options   = pBP32Ctl->miscStart();
    fState.button.l3        = pBP32Ctl->thumbL();
    fState.button.r3        = pBP32Ctl->thumbR();
    fState.button.share     = pBP32Ctl->miscSelect();

    uint8_t dpad            = pBP32Ctl->dpad();
    fState.button.up        = CHECK_FLAG2(dpad, 0x1);
    fState.button.right     = CHECK_FLAG2(dpad, 0x4);
    fState.button.down      = CHECK_FLAG2(dpad, 0x2);
    fState.button.left      = CHECK_FLAG2(dpad, 0x8);

    //fState.button.upright  = pBP32Ctl->topRight();
    //fState.button.upleft   = pBP32Ctl->topLeft();
    //fState.button.downright= pBP32Ctl->bottomRight();
    //fState.button.downleft = pBP32Ctl->bottomLeft();

    fState.button.l2        = pBP32Ctl->l2();
    fState.button.r2        = pBP32Ctl->r2();
    fState.button.l1        = pBP32Ctl->l1();
    fState.button.r1        = pBP32Ctl->r1();

    fState.button.triangle  = pBP32Ctl->y();
    fState.button.circle    = pBP32Ctl->b();    
    fState.button.cross     = pBP32Ctl->a();
    fState.button.square    = pBP32Ctl->x();

    fState.button.ps        = pBP32Ctl->miscSystem();
    //fState.button.touchpad = CHECK_FLAG(kBMask_touchpad);

    // Raw stick y values report up as negative and down as positive. Flip these. 
    fState.analog.stick.lx = (int8_t) constrain(pBP32Ctl->axisX() / 4, INT8_MIN, INT8_MAX); // (-512 - 511) int32_t -> (-128 - 127) int8_t
    fState.analog.stick.ly = (int8_t) constrain(-(pBP32Ctl->axisY() + 1) / 4, INT8_MIN, INT8_MAX); // (511 -> -512) int32_t -> (-128 - 127) int8_t
    fState.analog.stick.rx = (int8_t) constrain(pBP32Ctl->axisRX() / 4, INT8_MIN, INT8_MAX); // (-512 - 511) -> (-128 - 127)
    fState.analog.stick.ry = (int8_t) constrain(-(pBP32Ctl->axisRY() + 1) / 4, INT8_MIN, INT8_MAX); // (511 -> -512) -> (-128 - 127)

    fState.analog.button.l2 = (uint8_t) constrain(pBP32Ctl->brake() / 4, 0, 255); // (0 - 1023) int32_t -> (0 - 255) uint8_t
    fState.analog.button.r2 = (uint8_t) constrain(pBP32Ctl->throttle() / 4, 0, 255); // (0 - 1023) int32_t -> (0 - 255) uint8_t

    //fState.status.battery   = pBP32Ctl->battery;
    //fState.status.charging = ((packet[kIndex_status] & kStatusMask_charging) != 0);
    //fState.status.audio    = ((packet[kIndex_status] & kStatusMask_audio) != 0);
    //fState.status.mic      = ((packet[kIndex_status] & kStatusMask_mic) != 0);

    /*
    ctl->index(),       // Controller Index
    ctl->dpad(),        // D-pad
    ctl->buttons(),     // bitmask of pressed buttons
    ctl->axisX(),       // (-511 - 512) left X Axis
    ctl->axisY(),       // (-511 - 512) left Y axis
    ctl->axisRX(),      // (-511 - 512) right X axis
    ctl->axisRY(),      // (-511 - 512) right Y axis
    ctl->brake(),       // (0 - 1023): brake button
    ctl->throttle(),    // (0 - 1023): throttle (AKA gas) button
    ctl->miscButtons(), // bitmask of pressed "misc" buttons
    ctl->gyroX(),       // Gyro X
    ctl->gyroY(),       // Gyro Y
    ctl->gyroZ(),       // Gyro Z
    ctl->accelX(),      // Accelerometer X
    ctl->accelY(),      // Accelerometer Y
    ctl->accelZ()       // Accelerometer Z
    */

    /* Button down events */
    CHECK_BUTTON_DOWN(select);
    CHECK_BUTTON_DOWN(l3);
    CHECK_BUTTON_DOWN(r3);
    CHECK_BUTTON_DOWN(start);

    CHECK_BUTTON_DOWN(up);
    CHECK_BUTTON_DOWN(right);
    CHECK_BUTTON_DOWN(down);
    CHECK_BUTTON_DOWN(left);

    CHECK_BUTTON_DOWN(upright);
    CHECK_BUTTON_DOWN(upleft);
    CHECK_BUTTON_DOWN(downright);
    CHECK_BUTTON_DOWN(downleft);

    CHECK_BUTTON_DOWN(l2);
    CHECK_BUTTON_DOWN(r2);
    CHECK_BUTTON_DOWN(l1);
    CHECK_BUTTON_DOWN(r1);

    CHECK_BUTTON_DOWN(triangle);
    CHECK_BUTTON_DOWN(circle);
    CHECK_BUTTON_DOWN(cross);
    CHECK_BUTTON_DOWN(square);

    CHECK_BUTTON_DOWN(ps);
    CHECK_BUTTON_DOWN(share);
    CHECK_BUTTON_DOWN(options);
    CHECK_BUTTON_DOWN(touchpad);

    /* Button up events */
    CHECK_BUTTON_UP(select);
    CHECK_BUTTON_UP(l3);
    CHECK_BUTTON_UP(r3);
    CHECK_BUTTON_UP(start);

    CHECK_BUTTON_UP(up);
    CHECK_BUTTON_UP(right);
    CHECK_BUTTON_UP(down);
    CHECK_BUTTON_UP(left);

    CHECK_BUTTON_UP(upright);
    CHECK_BUTTON_UP(upleft);
    CHECK_BUTTON_UP(downright);
    CHECK_BUTTON_UP(downleft);

    CHECK_BUTTON_UP(l2);
    CHECK_BUTTON_UP(r2);
    CHECK_BUTTON_UP(l1);
    CHECK_BUTTON_UP(r1);

    CHECK_BUTTON_UP(triangle);
    CHECK_BUTTON_UP(circle);
    CHECK_BUTTON_UP(cross);
    CHECK_BUTTON_UP(square);

    CHECK_BUTTON_UP(ps);
    CHECK_BUTTON_UP(share);
    CHECK_BUTTON_UP(options);
    CHECK_BUTTON_UP(touchpad);

    /* Analog events */
    evt.analog_changed.stick.lx        = fState.analog.stick.lx - prev.analog.stick.lx;
    evt.analog_changed.stick.ly        = fState.analog.stick.ly - prev.analog.stick.ly;
    evt.analog_changed.stick.rx        = fState.analog.stick.rx - prev.analog.stick.rx;
    evt.analog_changed.stick.ry        = fState.analog.stick.ry - prev.analog.stick.ry;

    evt.analog_changed.button.up       = fState.analog.button.up    - prev.analog.button.up;
    evt.analog_changed.button.right    = fState.analog.button.right - prev.analog.button.right;
    evt.analog_changed.button.down     = fState.analog.button.down  - prev.analog.button.down;
    evt.analog_changed.button.left     = fState.analog.button.left  - prev.analog.button.left;

    evt.analog_changed.button.l2       = fState.analog.button.l2 - prev.analog.button.l2;
    evt.analog_changed.button.r2       = fState.analog.button.r2 - prev.analog.button.r2;
    evt.analog_changed.button.l1       = fState.analog.button.l1 - prev.analog.button.l1;
    evt.analog_changed.button.r1       = fState.analog.button.r1 - prev.analog.button.r1;

    evt.analog_changed.button.triangle = fState.analog.button.triangle - prev.analog.button.triangle;
    evt.analog_changed.button.circle   = fState.analog.button.circle   - prev.analog.button.circle;
    evt.analog_changed.button.cross    = fState.analog.button.cross    - prev.analog.button.cross;
    evt.analog_changed.button.square   = fState.analog.button.square   - prev.analog.button.square;        

    if (fConnected)
    {
        //Serial.println("BluepadController::notify");
        state = fState;
        event = evt;
        notify();
    }
    /*
    else if (fConnecting)
    {
        fConnecting = false;
        fConnected = true;
        //setPlayer(fPlayer);
        onConnect();
    }
    */
}

#endif

