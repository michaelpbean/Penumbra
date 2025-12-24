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

        void setPlayer(int player);        
        void assignBP32Controller(int id, Controller* pController);
        void clearBP32Controller(int id);
        void updateState();

    protected:
        Type mType;
        int mBluepadControllerID;
        State fState;
};

#endif
#endif
