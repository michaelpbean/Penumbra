#ifdef USE_BLUEPAD

#ifndef BluepadController_h
#define BluepadController_h

#if !defined(ESP32)
#error Only supports ESP32
#endif

#include <inttypes.h>
#include "JoystickController.h"

class Controller;

class BluepadController : public JoystickController
{
    public:

        enum Type
        {
            // PSController defines kPS3, kPS3Nav, kPS4
            kPS3,
            kPS3Nav,
            kPS4,
            kPS5,
            kSwitchJoycon
        };

        static void startup();
        static void registerBluepadController(BluepadController* pCtl);
        static void update();

        BluepadController(const char* mac, Type type = kPS5);
        BluepadController();
        virtual ~BluepadController();

        virtual void disconnect() override;
        void setPlayer(int player);        
        void assignBP32Controller(int id, Controller* pController);
        void clearBP32Controller(int id);
        void updateState();

    protected:
        void setColorLED(uint8_t red, uint8_t green, uint8_t blue);
        Type mType;
        int mBluepadControllerID;
        State fState;
};

#endif
#endif
