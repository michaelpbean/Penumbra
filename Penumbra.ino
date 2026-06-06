// =======================================================================================
//         Penumbra: Minimal drive system for the ESP32
// =======================================================================================
//  Written by: skelmir
// =======================================================================================
//
//         This program is distributed in the hope that it will be useful 
//         as a courtesy to fellow astromech club members wanting to develop
//         their own droid control system.
//
//         IT IS OFFERED WITHOUT ANY WARRANTY; without even the implied warranty of
//         MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
//         You are using this software at your own risk and, as a fellow club member, it is
//         expected you will have the proper experience / background to handle and manage that 
//         risk appropriately.  It is completely up to you to insure the safe operation of
//         your droid and to validate and test all aspects of your droid control system.
//
// =======================================================================================
//   Note: You will need an ESP32 (either WROOM or WRover).
//
//   You will need to install the Reeltwo library. 
//      https://github.com/reeltwo/Reeltwo
//
//   To use Roboteq SBL2360 as your motor controller. Define:
//     #define DRIVE_SYSTEM         DRIVE_SYSTEM_ROBOTEQ_PWM
//
//   To use Roboteq SBL2360 as your motor controller and enable turtle command control. Define:
//     #define DRIVE_SYSTEM         DRIVE_SYSTEM_ROBOTEQ_PWM_SERIAL
//
//   To use Sabertooth 2x32 as your motor controller. Define:
//     #define DRIVE_SYSTEM         DRIVE_SYSTEM_SABER
//
//   To use a PWM based motor controller. Define:
//     #define DRIVE_SYSTEM         DRIVE_SYSTEM_PWM
//
//   To use a PWM based motor controller. Define:
//     #define DRIVE_SYSTEM         DRIVE_SYSTEM_PWM
//
//   Change this macro to change the dome motor system:
//     #define DOME_DRIVE           DOME_DRIVE_SABER
//
//    To disable the dome motor support change it to:
//      #define DOME_DRIVE          DOME_DRIVE_NONE
//
//   Define MY_BT_ADDR to the address stored in your PS3 Navication controller. Or update
//   your PS3 Nav controller to match the BT address of your ESP32.
//     #define MY_BT_ADDR          "24:6f:28:44:a5:ae"
//
// =======================================================================================
//
//  PS3 Controll button mapping:
//
//    L2: Throttle
//    L1: Hard brake
//    Joystick: Drive
//
//  PS3 Dome controller:
//
//    Joystick: Left/right spin dome
//
//  All other buttons are unmapped and will print a debug message for you to repurpose
// =======================================================================================
//
// Gesture codes for buttons are as follows:
//
//    X: Cross
//    O: Circle
//    U: Up
//    R: Down
//    L: Left
//    P: PS
//
// Gesture codes for the analog stick are as follows:
//
//    1 2 3
//    4 5 6
//    7 8 9
//
// For example:
//
//    GESTURE: UDU    (KeyPad: Up-Down-Up)
//    GESTURE: 252    (Analog stick Up-Center-Up)
//
// You can disable gesture support by commenting or deleting the following:
//
// #define DOME_CONTROLLER_GESTURES
// =======================================================================================
//
#include "User_Settings.h"
#include "ReelTwo.h"
#if defined(USE_HCR_VOCALIZER) || defined(USE_MP3_TRIGGER) || defined(USE_DFMINI_PLAYER)
 #define SOUND_DEBUG printf
 extern SoftwareSerial soundSerial;
 #include "MarcduinoSound.h"
 #if defined(USE_HCR_VOCALIZER)
  #define MARC_SOUND_PLAYER MarcSound::kHCR
 #elif defined(USE_MP3_TRIGGER)
  #define MARC_SOUND_PLAYER MarcSound::kMP3Trigger
 #elif defined(USE_DFMINI_PLAYER)
  #define MARC_SOUND_PLAYER MarcSound::kDFMini
 #endif
 #define MARC_SOUND
#endif
#include "drive/TankDrivePWM.h"
#include "drive/TankDriveRoboteq.h"
#if DRIVE_SYSTEM == DRIVE_SYSTEM_SABER
 #include "drive/TankDriveSabertooth.h"
#endif
#if DOME_DRIVE != DOME_DRIVE_NONE
 #include "drive/DomeDrivePWM.h"
 #if DOME_DRIVE == DOME_DRIVE_SABER
  #include "drive/DomeDriveSabertooth.h"
 #endif
#endif
#include "ServoDispatchDirect.h"
#include "ServoEasing.h"
#include "src/Images.h"
#include <Preferences.h>
#ifdef USE_WIFI
 #include "wifi/WifiAccess.h"
 #include <ESPmDNS.h>
 #ifdef USE_WIFI_WEB
  #include "wifi/WifiWebServer.h"
 #endif
#endif

#ifdef USE_BLUEPAD
    #include "src/BluepadController/BluepadController.h"
#else
    #include "bt/PSController/PSController.h"
    #include "esp_bt_device.h"
#endif

#ifdef USE_OTA
 #include <ArduinoOTA.h>
#endif

#ifdef USE_USB
#include "usbhub.h"
#include "BTD.h"

USB Usb;
BTD Btd(&Usb);
#endif

////////////////////////////////

// Group ID is used by the ServoSequencer and some ServoDispatch functions to
// identify a group of servos.
//
//     Pin,                Min,  Max,  Group ID
const ServoSettings servoSettings[] PROGMEM = {
#ifdef NEED_DRIVE_PWM_PINS
     { LEFT_MOTOR_PWM,      1000, 2000, 0 }         // Neo motor min/max
    ,{ RIGHT_MOTOR_PWM,     1000, 2000, 0 }         // Neo motor min/max
  #ifdef THROTTLE_MOTOR_PWM
    ,{ THROTTLE_MOTOR_PWM, 1000, 2000, 0 }
  #endif
  #ifdef NEED_DOME_PWM_PINS
    ,{ DOME_MOTOR_PWM,      800, 2200, 0 }
  #endif
#endif
};
ServoDispatchDirect<SizeOfArray(servoSettings)> servoDispatch(servoSettings);

////////////////////////////////

TaskHandle_t eventTask;
Preferences preferences;
#ifdef SERIAL_MARCDUINO_TX_PIN
SoftwareSerial marcSerial;
#endif
#ifdef MARC_SOUND
SoftwareSerial soundSerial;
#endif

enum TestHCRSoundSlot
{
    kTestHCRUp,
    kTestHCRRight,
    kTestHCRDown,
    kTestHCRLeft
};

#ifdef MARC_SOUND
static HCRSound getTestHCRSound(TestHCRSoundSlot slot, bool l1Held, bool r1Held)
{
    static const HCRSound kBaseMap[4][4] = {
        { kHCRLeiaHologram,   kHCRTHX,             kHCRStarTours,       kHCRCantina1 },
        { kHCRMainTitleTheme, kHCRCantina1,        kHCRTHX,             kHCRImperialMarch },
        { kHCRMedalCeremony,  kHCRLeiaHologram,    kHCRLeiaHologram,    kHCRMainTitleTheme },
        { kHCRFoxFanfare,     kHCRMainTitleTheme,  kHCRFoxFanfare,      kHCRMedalCeremony }
    };

    uint8_t modifierIndex = (l1Held ? 2 : 0) + (r1Held ? 1 : 0);
    return kBaseMap[slot][modifierIndex];
}

static void playTestHCRSound(TestHCRSoundSlot slot, bool l1Held, bool r1Held)
{
    HCRSound sound = getTestHCRSound(slot, l1Held, r1Held);
    const HCRSoundConfig* config = getHCRSoundConfig(sound);
    if (config != nullptr)
    {
        DEBUG_PRINT("HCR test sound: ");
        DEBUG_PRINT(config->name);
        DEBUG_PRINT(" ");
        DEBUG_PRINTLN(config->number);
        sMarcSound.playHCRSoundA(sound);
    }
}
#endif

////////////////////////////////////
// Forward declare utility routines
////////////////////////////////////
void enableController();
void disableController();
void emergencyStop();
bool driveControlsEnabled();
void enableDomeController();
void disableDomeController();
void domeEmergencyStop();
void eventLoopTask(void *arg);
#ifdef MARC_SOUND
void processSoundSerial();
void processSoundSerialCommand(char* cmd);
#endif

////////////////////////////////////
#ifdef USE_BLUEPAD
class DriveController : public BluepadController
#else
class DriveController : public PSController
#endif
{
public:
    #ifdef USE_BLUEPAD
        DriveController(const char* mac = nullptr) : BluepadController(mac, DRIVE_CONTROLLER_TYPE) {}
    #else
        DriveController(const char* mac = nullptr) : PSController(mac, DRIVE_CONTROLLER_TYPE) {}
    #endif

    virtual void notify() override
    {
        fLastDataTime = millis();

        if (event.button_down.l1)
        {
            DEBUG_PRINTLN("DRIVE L1 DOWN");
        }
        if (event.button_down.r1)
        {
            DEBUG_PRINTLN("DRIVE R1 DOWN");
        }
        if (event.button_down.l2)
        {
            DEBUG_PRINTLN("DRIVE L2 DOWN");
        }
        if (event.button_down.r2)
        {
            DEBUG_PRINTLN("DRIVE R2 DOWN");
        }

        // Event handling map these actions for your droid.
        // You can choose to either respond to key down or key up
        if (event.button_down.l3)
        {
            DEBUG_PRINTLN("DRIVE L3 DOWN");
        }

        // Face buttons
        if (event.button_down.cross)
        {
            DEBUG_PRINTLN("DRIVE X DOWN");
            // Temporarily repurpose the X button to toggle drive control enabled/disabled for testing safety features. Comment out or delete this block when you want to use the X button for something else.
            //setDriveControlsEnabled(!driveControlsEnabled());
        }

        if (event.button_down.square)
        {
            DEBUG_PRINTLN("DRIVE Square DOWN");
        }

        if (event.button_down.triangle)
        {
            DEBUG_PRINTLN("DRIVE Triangle DOWN");
            #if defined(MARC_SOUND) && defined(ENABLE_SOUND_CONTROLLER_BUTTONS)
                if (state.button.r1)
                    sMarcSound.setCanonicalMode(false);
                else
                    sMarcSound.setCanonicalMode(true);
            #endif
        }

        if (event.button_down.circle)
        {
            DEBUG_PRINTLN("DRIVE O DOWN");
        }

        // Dpad buttons
        if (event.button_down.up)
        {
            DEBUG_PRINTLN("DRIVE Started pressing the up button");
            #if defined(MARC_SOUND) && defined(ENABLE_SOUND_CONTROLLER_BUTTONS)
                if (state.button.l1)
                    playTestHCRSound(kTestHCRUp, true, state.button.r1);
                else if (state.button.r1)
                    sMarcSound.playHappy(true);
                else
                    sMarcSound.playHappy(false);
            #endif
        }

        if (event.button_down.right)
        {
            DEBUG_PRINTLN("DRIVE Started pressing the right button");
            #if defined(MARC_SOUND) && defined(ENABLE_SOUND_CONTROLLER_BUTTONS)
                if (state.button.l1)
                    playTestHCRSound(kTestHCRRight, true, state.button.r1);
                else if (state.button.r1)
                    sMarcSound.playScared(true);
                else
                    sMarcSound.playScared(false);
            #endif
        }

        if (event.button_down.down)
        {
            DEBUG_PRINTLN("DRIVE Started pressing the down button");
            #if defined(MARC_SOUND) && defined(ENABLE_SOUND_CONTROLLER_BUTTONS)
                if (state.button.l1)
                    playTestHCRSound(kTestHCRDown, true, state.button.r1);
                else if (state.button.r1)
                    sMarcSound.playSad(true);
                else
                    sMarcSound.playSad(false);
            #endif
        }

        if (event.button_down.left)
        {
            DEBUG_PRINTLN("DRIVE Started pressing the left button");
            #if defined(MARC_SOUND) && defined(ENABLE_SOUND_CONTROLLER_BUTTONS)
                if (state.button.l1)
                    playTestHCRSound(kTestHCRLeft, true, state.button.r1);
                else if (state.button.r1)
                    sMarcSound.playOverload();
                else
                    sMarcSound.triggerMuse();
            #endif
        }

        // Other buttons
        if (event.button_down.ps)
        {
            DEBUG_PRINTLN("DRIVE PS DOWN");
        }
    }

    void updateSafety()
    {
        if (!isConnected())
            return;

        uint32_t currentTime = millis();
        uint32_t lagTime = (currentTime > fLastDataTime) ? currentTime - fLastDataTime : 0;
        if (lagTime > 5000)
        {
            DEBUG_PRINTLN("More than 5 seconds. Disconnect");
            emergencyStop();
            disconnect();
        }
        else if (lagTime > 500)
        {
            DEBUG_PRINTLN("It has been 500ms. Shutdown motors");
            emergencyStop();
        }
    }

    void setDriveControlsEnabled(bool enabled)
    {
        if (enabled)
        {
            DEBUG_PRINTLN("Drive controls enabled");
            enableController();
        }
        else
        {
            DEBUG_PRINTLN("Drive controls disabled");
            disableController();
        }
        #ifdef USE_BLUEPAD
            setColorLED(enabled ? 0 : 255, 0, enabled ? 255 : 0);
        #endif
    }

    virtual void onConnect() override
    {
        DEBUG_PRINTLN("Drive Stick Connected");
        setPlayer(1);
        enableController();
        enableDomeController();
        fLastDataTime = millis();
        #ifdef USE_BLUEPAD
            setColorLED(0, 0, 255);
        #endif
    }
    
    virtual void onDisconnect() override
    {
        DEBUG_PRINTLN("Drive Stick Disconnected");
        disableController();
        disableDomeController();
        fLastDataTime = 0;
    }

    uint32_t fLastDataTime = 0;
};
DriveController driveStick(DRIVE_STICK_BT_ADDR);

#if DOME_DRIVE != DOME_DRIVE_NONE && DRIVE_CONTROLLER_TYPE == 1 // kPS3Nav == 1
// If dome drive is enabled and we are using PS3 Nav Controllers
#ifdef USE_BLUEPAD
class DomeController : public BluepadController
#else
class DomeController : public PSController
#endif
{
public:
    #ifdef USE_BLUEPAD
        DomeController(const char* mac = nullptr) : BluepadController(mac) {}
    #else
        DomeController(const char* mac = nullptr) : PSController(mac) {}
    #endif
    virtual void notify() override
    {
        fLastDataTime = millis();
        process();
    }

    void updateSafety()
    {
        if (!isConnected())
            return;

        uint32_t currentTime = millis();
        uint32_t lagTime = (currentTime > fLastDataTime) ? currentTime - fLastDataTime : 0;
        if (lagTime > 5000)
        {
            DEBUG_PRINTLN("More than 5 seconds. Disconnect");
            domeEmergencyStop();
            disconnect();
        }
        else if (lagTime > 300)
        {
            DEBUG_PRINTLN("It has been 300ms. Shutdown motors");
            domeEmergencyStop();
        }
    }

    void process()
    {
        if (!fGestureCollect)
        {
        #ifdef DOME_CONTROLLER_GESTURES
            if (event.button_up.l3)
            {
                DEBUG_PRINTLN("GESTURE START COLLECTING\n");
                disableDomeController();
                fGestureCollect = true;
                fGesturePtr = fGestureBuffer;
                fGestureTimeOut = millis() + GESTURE_TIMEOUT_MS;
            }
        #else
            // Event handling map these actions for your droid.
            // You can choose to either respond to key down or key up
            if (event.button_down.l3)
            {
                DEBUG_PRINTLN("DOME L3 DOWN");
            }
            else if (event.button_up.l3)
            {
                DEBUG_PRINTLN("DOME L3 UP");
            }
        #endif
            if (event.button_down.cross)
            {
                DEBUG_PRINTLN("DOME X DOWN");
            }
            else if (event.button_up.cross)
            {
                DEBUG_PRINTLN("DOME X UP");
            }

            if (event.button_down.circle)
            {
                DEBUG_PRINTLN("DOME O DOWN");
            }
            else if (event.button_up.circle)
            {
                DEBUG_PRINTLN("DOME O UP");
            }

            if (event.button_down.up)
            {
                DEBUG_PRINTLN("DOME Started pressing the up button");
            }
            else if (event.button_up.up)
            {
                DEBUG_PRINTLN("DOME Released the up button");
            }

            if (event.button_down.right)
            {
                DEBUG_PRINTLN("DOME Started pressing the right button");
            }
            else if (event.button_up.right)
            {
                DEBUG_PRINTLN("DOME Released the right button");
            }

            if (event.button_down.down)
            {
                DEBUG_PRINTLN("DOME Started pressing the down button");
            }
            else if (event.button_up.down)
            {
                DEBUG_PRINTLN("DOME Released the down button");
            }

            if (event.button_down.left)
            {
                DEBUG_PRINTLN("DOME Started pressing the left button");
            }
            else if (event.button_up.left)
            {
                DEBUG_PRINTLN("DOME Released the left button");
            }

            if (event.button_down.ps)
            {
                DEBUG_PRINTLN("DOME PS DOWN");
            }
            else if (event.button_up.ps)
            {
                DEBUG_PRINTLN("DOME PS UP");
            }
            return;
        }
        else if (fGestureTimeOut < millis())
        {
            DEBUG_PRINTLN("GESTURE TIMEOUT\n");
            enableDomeController();
            fGesturePtr = fGestureBuffer;
            fGestureCollect = false;
        }
        else
        {
            if (event.button_up.l3)
            {
                // delete trailing '5' from gesture
                unsigned glen = strlen(fGestureBuffer);
                if (glen > 0 && fGestureBuffer[glen-1] == '5')
                    fGestureBuffer[glen-1] = 0;
                DEBUG_PRINT("GESTURE: "); DEBUG_PRINTLN(fGestureBuffer);
                enableDomeController();
                fGestureCollect = false;
            }
            if (event.button_up.cross)
                addGesture('X');
            if (event.button_up.circle)
                addGesture('O');
            if (event.button_up.up)
                addGesture('U');
            if (event.button_up.right)
                addGesture('R');
            if (event.button_up.down)
                addGesture('D');
            if (event.button_up.left)
                addGesture('L');
            if (event.button_up.ps)
                addGesture('P');
            if (!fGestureAxis)
            {
                if (abs(state.analog.stick.lx) > 50 && abs(state.analog.stick.ly) > 50)
                {
                    // Diagonal
                    if (state.analog.stick.lx < 0)
                        fGestureAxis = (state.analog.stick.ly < 0) ? '1' : '7';
                    else
                        fGestureAxis = (state.analog.stick.ly < 0) ? '3' : '9';
                    addGesture(fGestureAxis);
                }
                else if (abs(state.analog.stick.lx) > 100)
                {
                    // Horizontal
                    fGestureAxis = (state.analog.stick.lx < 0) ? '4' : '6';
                    addGesture(fGestureAxis);
                }
                else if (abs(state.analog.stick.ly) > 100)
                {
                    // Vertical
                    fGestureAxis = (state.analog.stick.ly < 0) ? '2' : '8';
                    addGesture(fGestureAxis);
                }
            }
            if (fGestureAxis && abs(state.analog.stick.lx) < 10 && abs(state.analog.stick.ly) < 10)
            {
                addGesture('5');
                fGestureAxis = 0;   
            }
        }
    }

    virtual void onConnect() override
    {
        DEBUG_PRINTLN("Dome Stick Connected");
        setPlayer(2);
        enableDomeController();
        fLastDataTime = millis();
    }
    
    virtual void onDisconnect() override
    {
        DEBUG_PRINTLN("Dome Stick Disconnected");
        disableDomeController();
        fLastDataTime = 0;
    }

protected:
    uint32_t fLastDataTime = 0;
    bool fGestureCollect = false;
    char fGestureBuffer[MAX_GESTURE_LENGTH+1];
    char* fGesturePtr = fGestureBuffer;
    char fGestureAxis = 0;
    uint32_t fGestureTimeOut = 0;

    void addGesture(char ch)
    {
        if (fGesturePtr-fGestureBuffer < sizeof(fGestureBuffer)-1)
        {
            *fGesturePtr++ = ch;
            *fGesturePtr = '\0';
            fGestureTimeOut = millis() + GESTURE_TIMEOUT_MS;
        }
    }
};

DomeController domeStick(DOME_STICK_BT_ADDR);
#elif DOME_DRIVE != DOME_DRIVE_NONE
// Dome Drive enabled using either PS3 or PS4 controller
// the right side of the controller will be used to control the dome (left/right)
#define domeStick driveStick
#endif

#ifdef USE_RADIO
RadioController radioStick(Serial2);
#endif

#if DRIVE_SYSTEM == DRIVE_SYSTEM_SABER
// Tank Drive assign:
//    Serial1 for Sabertooth packet serial commands
TankDriveSabertooth tankDrive(TANK_DRIVE_ID, Serial1, driveStick);
#elif DRIVE_SYSTEM == DRIVE_SYSTEM_PWM
// Tank Drive assign:
//    servo index 0 (LEFT_MOTOR_PWM)
//    servo index 1 (RIGHT_MOTOR_PWM)
//    servo index 2 (THROTTLE_MOTOR_PWM)
TankDrivePWM tankDrive(servoDispatch, DRIVE_PWM_SETTINGS, driveStick);
#elif DRIVE_SYSTEM == DRIVE_SYSTEM_ROBOTEQ_PWM
// Tank Drive assign:
//    servo index 0 (LEFT_MOTOR_PWM)
//    servo index 1 (RIGHT_MOTOR_PWM)
//    servo index 2 (THROTTLE_MOTOR_PWM)
TankDriveRoboteq tankDrive(servoDispatch, DRIVE_PWM_SETTINGS, driveStick);
#elif DRIVE_SYSTEM == DRIVE_SYSTEM_ROBOTEQ_SERIAL
// Tank Drive assign:
//    servo index 0 (LEFT_MOTOR_PWM)
//    servo index 1 (RIGHT_MOTOR_PWM)
//    servo index 2 (THROTTLE_MOTOR_PWM)
TankDriveRoboteq tankDrive(Serial1, driveStick);
#elif DRIVE_SYSTEM == DRIVE_SYSTEM_ROBOTEQ_PWM_SERIAL
// Tank Drive assign:
//    servo index 0 (LEFT_MOTOR_PWM)
//    servo index 1 (RIGHT_MOTOR_PWM)
//    servo index 2 (THROTTLE_MOTOR_PWM)
//    Serial1 for Roboteq serial commands
TankDriveRoboteq tankDrive(Serial1, servoDispatch, DRIVE_PWM_SETTINGS, driveStick);
#else
#error Unsupported DRIVE_SYSTEM
#endif

#ifdef NEED_DOME_PWM_PINS
DomeDrivePWM domeDrive(servoDispatch, DOME_PWM_SETTINGS, domeStick);
#elif DOME_DRIVE == DOME_DRIVE_SABER
DomeDriveSabertooth domeDrive(DOME_DRIVE_ID, DOME_DRIVE_SERIAL, domeStick);
#endif

void enableController()
{
    tankDrive.setEnable(true);
}

void disableController()
{
    emergencyStop();
    tankDrive.setEnable(false);
}

void emergencyStop()
{
    tankDrive.stop();
}

bool driveControlsEnabled()
{
    return tankDrive.getEnable();
}

void enableDomeController()
{
#if DOME_DRIVE != DOME_DRIVE_NONE
    domeDrive.setEnable(true);
#endif
}

void disableDomeController()
{
#if DOME_DRIVE != DOME_DRIVE_NONE
    domeEmergencyStop();
    domeDrive.setEnable(false);
#endif
}

void domeEmergencyStop()
{
#if DOME_DRIVE != DOME_DRIVE_NONE
    domeDrive.stop();
#endif
}

#ifdef USE_WIFI
WifiAccess wifiAccess;
#endif

#ifdef USE_WIFI_WEB
 #include "WebPages.h"
#endif

#ifndef USE_BLUEPAD
void showBluetoothAddress()
{
    const uint8_t* addr = esp_bt_dev_get_address();
    printf("%02X:%02X:%02X:%02X:%02X:%02X\n", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);

#ifdef USE_USB
    Btd.my_bdaddr[0] = addr[5];
    Btd.my_bdaddr[1] = addr[4];
    Btd.my_bdaddr[2] = addr[3];
    Btd.my_bdaddr[3] = addr[2];
    Btd.my_bdaddr[4] = addr[1];
    Btd.my_bdaddr[5] = addr[0];
#endif
}
#endif

void setup()
{
    REELTWO_READY();

    if (!preferences.begin("rseries", false))
    {
        DEBUG_PRINTLN("Failed to init prefs");
    }

#ifdef USE_WIFI_WEB
    // In preparation for adding WiFi settings web page
    wifiAccess.setNetworkCredentials(
        preferences.getString(PREFERENCE_WIFI_SSID, WIFI_AP_NAME),
        preferences.getString(PREFERENCE_WIFI_PASS, WIFI_AP_PASSPHRASE),
        preferences.getBool(PREFERENCE_WIFI_AP, WIFI_ACCESS_POINT),
        preferences.getBool(PREFERENCE_WIFI_ENABLED, WIFI_ENABLED));
#endif

#ifdef DRIVE_BAUD_RATE
    // Serial1 is used for drive system if using Sabertooth or Roboteq serial commands, otherwise it is unused and can be used for other purposes.
    #if (DRIVE_SYSTEM != DRIVE_SYSTEM_PWM) && (DRIVE_SYSTEM != DRIVE_SYSTEM_ROBOTEQ_PWM)
        Serial1.begin(DRIVE_BAUD_RATE, SERIAL_8N1, SERIAL1_RX_PIN, SERIAL1_TX_PIN);
    #endif

    // Serial2 is used for the dome drive if Sabertooth is not being used for the main drive, so start here.
    #if (DOME_DRIVE == DOME_DRIVE_SABER) && (DRIVE_SYSTEM != DRIVE_SYSTEM_SABER)
        Serial2.begin(DRIVE_BAUD_RATE, SERIAL_8N1, SERIAL2_RX_PIN, SERIAL2_TX_PIN);
    #endif
#endif
#ifdef USE_RADIO
    Serial2.begin(57600, SERIAL_8N1, SERIAL2_RX_PIN, SERIAL2_TX_PIN);
#endif
#ifdef SERIAL_MARCDUINO_TX_PIN
    // Transmit only software serial on SERIAL_MARCDUINO_TX_PIN
    marcSerial.begin(MARCDUINO_BAUD_RATE, SWSERIAL_8N1, -1, SERIAL_MARCDUINO_TX_PIN, false, 0);
#endif
#ifdef MARC_SOUND
    SOUND_SERIAL_INIT(SOUND_SERIAL_BAUD);
    MarcSound::Module soundPlayer = (MarcSound::Module)preferences.getInt(PREFERENCE_MARCSOUND, MARC_SOUND_PLAYER);
    int soundStartup = preferences.getInt(PREFERENCE_MARCSOUND_STARTUP, MARC_SOUND_STARTUP);
    if (!sMarcSound.begin(soundPlayer, SOUND_SERIAL, soundStartup))
    {
        DEBUG_PRINTLN("FAILED TO INITIALIZE SOUND MODULE");
    }
    sMarcSound.setVolume(preferences.getInt(PREFERENCE_MARCSOUND_VOLUME, MARC_SOUND_VOLUME) / 1000.0);
    sMarcSound.playStartSound();
    sMarcSound.setRandomMin(preferences.getInt(PREFERENCE_MARCSOUND_RANDOM_MIN, MARC_SOUND_RANDOM_MIN));
    sMarcSound.setRandomMax(preferences.getInt(PREFERENCE_MARCSOUND_RANDOM_MAX, MARC_SOUND_RANDOM_MAX));
    if (preferences.getBool(PREFERENCE_MARCSOUND_RANDOM, MARC_SOUND_RANDOM))
        sMarcSound.startRandomInSeconds(13);
#endif

    SetupEvent::ready();

    // Initialize controller
    #ifdef USE_BLUEPAD
        // Bluepad controller initialization
        BluepadController::startup();
        BluepadController::registerBluepadController(&driveStick);
        #if DOME_DRIVE != DOME_DRIVE_NONE
            if ((BluepadController*) &domeStick != (BluepadController*) &driveStick)
                BluepadController::registerBluepadController(&domeStick);
        #endif
    #else
        #ifdef MY_BT_ADDR
            PSController::startListening(MY_BT_ADDR);
        #else
            PSController::startListening();
        #endif

        showBluetoothAddress();
    #endif
    

#ifdef USE_WIFI_WEB
    wifiAccess.notifyWifiConnected([](WifiAccess &wifi) {
        Serial.print("Connect to http://"); Serial.println(wifi.getIPAddress());
    #ifdef USE_MDNS
        // No point in setting up mDNS if R2 is the access point
        if (!wifi.isSoftAP())
        {
            String mac = wifi.getMacAddress();
            String hostName = mac.substring(mac.length()-5, mac.length());
            hostName.remove(2, 1);
            hostName = "RSeries-"+hostName;
            if (webServer.enabled())
            {
                Serial.print("Host name: "); Serial.println(hostName);
                if (!MDNS.begin(hostName.c_str()))
                {
                    DEBUG_PRINTLN("Error setting up MDNS responder!");
                }
            }
        }
    #endif
    });
#endif

#ifdef USE_OTA
    ArduinoOTA.onStart([]()
    {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH)
        {
            type = "sketch";
        }
        else // U_SPIFFS
        {
            type = "filesystem";
        }
        DEBUG_PRINTLN("OTA START");
        // Kill the motors
        tankDrive.setEnable(false);
    })
    .onEnd([]()
    {
        DEBUG_PRINTLN("OTA END");
    })
    .onProgress([](unsigned int progress, unsigned int total)
    {
        float range = (float)progress / (float)total;
    })
    .onError([](ota_error_t error)
    {
        String desc;
        if (error == OTA_AUTH_ERROR) desc = "Auth Failed";
        else if (error == OTA_BEGIN_ERROR) desc = "Begin Failed";
        else if (error == OTA_CONNECT_ERROR) desc = "Connect Failed";
        else if (error == OTA_RECEIVE_ERROR) desc = "Receive Failed";
        else if (error == OTA_END_ERROR) desc = "End Failed";
        else desc = "Error: "+String(error);
        DEBUG_PRINTLN(desc);
    });
#endif

    tankDrive.setMaxSpeed(preferences.getFloat(PREFERENCE_DRIVE_SPEED, MAXIMUM_SPEED));
    tankDrive.setThrottleAccelerationScale(preferences.getFloat(PREFERENCE_DRIVE_THROTTLE_ACC_SCALE, ACCELERATION_SCALE));
    tankDrive.setThrottleDecelerationScale(preferences.getFloat(PREFERENCE_DRIVE_THROTTLE_DEC_SCALE, DECELRATION_SCALE));
    tankDrive.setTurnAccelerationScale(preferences.getFloat(PREFERENCE_DRIVE_TURN_ACC_SCALE, ACCELERATION_SCALE*2));
    tankDrive.setTurnDecelerationScale(preferences.getFloat(PREFERENCE_DRIVE_TURN_DEC_SCALE, DECELRATION_SCALE));
#ifdef USE_RADIO
    tankDrive.setGuestStick(radioStick);
#endif
    tankDrive.setGuestSpeedModifier(preferences.getFloat(PREFERENCE_DRIVE_GUEST_SPEED, MAXIMUM_GUEST_SPEED));
    tankDrive.setScaling(preferences.getBool(PREFERENCE_DRIVE_SCALING, SCALING));
    tankDrive.setChannelMixing(preferences.getBool(PREFERENCE_DRIVE_MIXING, CHANNEL_MIXING));
    tankDrive.setThrottleInverted(preferences.getBool(PREFERENCE_DRIVE_THROTTLE_INVERT, THROTTLE_INVERTED));
    tankDrive.setTurnInverted(preferences.getBool(PREFERENCE_DRIVE_TURN_INVERT, TURN_INVERTED));
#ifdef ENABLE_TANK_DRIVE_THROOTLE_BOOST_MODE
    tankDrive.setUseThrottle(true);
#else
    tankDrive.setUseThrottle(false);
#endif
#ifdef TANK_DRIVE_LEFT_STICK
    tankDrive.setUseLeftStick();
#elif defined(TANK_DRIVE_RIGHT_STICK)
    tankDrive.setUseRightStick();
#endif
#ifdef TANK_DRIVE_USE_HARD_STOP
    tankDrive.setUseHardStop(true);
#else
    tankDrive.setUseHardStop(false);
#endif

#if DOME_DRIVE != DOME_DRIVE_NONE
 #ifdef DOME_DRIVE_LEFT_STICK
    domeDrive.setUseLeftStick();
 #elif defined(DOME_DRIVE_RIGHT_STICK)
    domeDrive.setUseRightStick();
 #endif
 #ifdef ENABLE_DOME_DRIVE_THROOTLE_BOOST_MODE
    domeDrive.setUseThrottle(true);
 #else
    domeDrive.setUseThrottle(true);
 #endif
 #ifdef DOME_DRIVE_USE_HARD_STOP
    domeDrive.setUseHardStop(true);
 #else
    domeDrive.setUseHardStop(false);
 #endif
    domeDrive.setInverted(preferences.getBool(PREFERENCE_DOME_DRIVE_INVERT, DOME_INVERTED));
#endif

#ifdef USE_WIFI_WEB
    // For safety we will stop the motors if the web client is connected
    webServer.setConnect([]() {
        // Callback for each connected web client
        // DEBUG_PRINTLN("Hello");
    });
#endif
    // tankDrive.enterCommandMode();

    xTaskCreatePinnedToCore(
          eventLoopTask,
          "Events",
          5000,    // shrink stack size?
          NULL,
          1,
          &eventTask,
          0);

    DEBUG_PRINT("Total heap:  "); DEBUG_PRINTLN(ESP.getHeapSize());
    DEBUG_PRINT("Free heap:   "); DEBUG_PRINTLN(ESP.getFreeHeap());
    DEBUG_PRINT("Total PSRAM: "); DEBUG_PRINTLN(ESP.getPsramSize());
    DEBUG_PRINT("Free PSRAM:  "); DEBUG_PRINTLN(ESP.getFreePsram());
    DEBUG_PRINTLN();

#ifdef USE_RADIO
    radioStick.start();
#endif

    DEBUG_PRINTLN("READY");
#ifdef USE_USB
    if (Usb.Init() == -1) {
        printf("OSC did not start\n");
        while (1); //halt
    }
#endif
}

void eventLoopTask(void* arg)
{
    for (;;)
    {
        AnimatedEvent::process();
        vTaskDelay(1);
    }
}

void loop()
{
#ifdef USE_USB
    Usb.Task();
#endif
#ifdef USE_OTA
    ArduinoOTA.handle();
#endif
#ifdef USE_WIFI_WEB
    webServer.handle();
#endif
#ifdef MARC_SOUND
    sMarcSound.idle();
    processSoundSerial();
#endif

    #ifdef USE_BLUEPAD
        BluepadController::update();
    #endif

    driveStick.updateSafety();
    #if DOME_DRIVE != DOME_DRIVE_NONE && DRIVE_CONTROLLER_TYPE == 1
        domeStick.updateSafety();
    #endif
    delay(1);
}

#ifdef MARC_SOUND
void processSoundSerial()
{
    static char buffer[64];
    static uint8_t pos = 0;

    while (Serial.available())
    {
        char ch = Serial.read();
        if (ch == '\n' || ch == '\r')
        {
            if (pos != 0)
            {
                buffer[pos] = '\0';
                processSoundSerialCommand(buffer);
                pos = 0;
            }
        }
        else if (pos < sizeof(buffer) - 1)
        {
            buffer[pos++] = ch;
        }
    }
}

void processSoundSerialCommand(char* cmd)
{
    if (strncmp(cmd, "#SMVOLUME", 9) == 0)
    {
        int val = atoi(cmd + 9);
        if (val < 0 || val > 1000)
        {
            DEBUG_PRINTLN("Sound Volume value out of range. Use 0 - 1000.");
            return;
        }
        preferences.putInt(PREFERENCE_MARCSOUND_VOLUME, val);
        sMarcSound.setVolume(val / 1000.0f);
        DEBUG_PRINT("Sound Volume: "); DEBUG_PRINTLN(val);
    }
    else if (strncmp(cmd, "#SMSOUND", 8) == 0)
    {
        MarcSound::Module soundPlayer = (MarcSound::Module)atoi(cmd + 8);
        switch (soundPlayer)
        {
            case MarcSound::kDisabled:
            case MarcSound::kMP3Trigger:
            case MarcSound::kDFMini:
            case MarcSound::kHCR:
                preferences.putInt(PREFERENCE_MARCSOUND, soundPlayer);
                sMarcSound.begin(soundPlayer, SOUND_SERIAL, preferences.getInt(PREFERENCE_MARCSOUND_STARTUP, MARC_SOUND_STARTUP));
                sMarcSound.setVolume(preferences.getInt(PREFERENCE_MARCSOUND_VOLUME, MARC_SOUND_VOLUME) / 1000.0f);
                DEBUG_PRINT("Sound Module: "); DEBUG_PRINTLN((int)soundPlayer);
                break;
            default:
                DEBUG_PRINTLN("Unknown sound module. Use 0, 1, 2, or 3.");
                break;
        }
    }
    else if (strncmp(cmd, "#SMSTARTUP", 10) == 0)
    {
        int val = atoi(cmd + 10);
        preferences.putInt(PREFERENCE_MARCSOUND_STARTUP, val);
        DEBUG_PRINT("Startup Sound: "); DEBUG_PRINTLN(val);
    }
    else if (strncmp(cmd, "#SMRANDMIN", 10) == 0)
    {
        int val = atoi(cmd + 10);
        preferences.putInt(PREFERENCE_MARCSOUND_RANDOM_MIN, val);
        sMarcSound.setRandomMin(val);
        DEBUG_PRINT("Random Min: "); DEBUG_PRINTLN(val);
    }
    else if (strncmp(cmd, "#SMRANDMAX", 10) == 0)
    {
        int val = atoi(cmd + 10);
        preferences.putInt(PREFERENCE_MARCSOUND_RANDOM_MAX, val);
        sMarcSound.setRandomMax(val);
        DEBUG_PRINT("Random Max: "); DEBUG_PRINTLN(val);
    }
    else if (strcmp(cmd, "#SMRAND0") == 0)
    {
        preferences.putBool(PREFERENCE_MARCSOUND_RANDOM, false);
        sMarcSound.stopRandom();
        DEBUG_PRINTLN("Random Sound Disabled");
    }
    else if (strcmp(cmd, "#SMRAND1") == 0)
    {
        preferences.putBool(PREFERENCE_MARCSOUND_RANDOM, true);
        sMarcSound.startRandom();
        DEBUG_PRINTLN("Random Sound Enabled");
    }
    else if (cmd[0] == '$')
    {
        sMarcSound.handleCommand(cmd);
    }
}
#endif
