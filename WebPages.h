#ifdef USE_WIFI_WEB

////////////////////////////////
// Web configurable parameters. Strongly advise not to change web settings while motors are running
// unless the wheels are off the ground.

#ifdef MARC_SOUND
String soundPlayer[] = {
    "Disabled",
    "MP3 Trigger",
    "DFMiniPlayer",
    "HCR"
};

int marcSoundPlayer;
int marcSoundVolume;
int marcSoundStartup;
bool marcSoundRandom;
int marcSoundRandomMin;
int marcSoundRandomMax;
#endif

WElement mainContents[] = {
    WH1("Drive Configuration"),
    WSlider("Max Speed", "maxspeed", 0, 100,
        []()->int { return tankDrive.getMaxSpeed()*100; },
        [](int val) { tankDrive.stop(); tankDrive.setMaxSpeed(val/100.0f); } ),
    WSlider("Guest Max Speed", "guestspeed", 0, 100,
        []()->int { return tankDrive.getGuestSpeedModifier()*100; },
        [](int val) { tankDrive.stop(); tankDrive.setGuestSpeedModifier(val/100.0f); } ),
    WSlider("Throttle Acceleration", "throttleAccel", 1, 400,
        []()->int { return tankDrive.getThrottleAccelerationScale(); },
        [](int val) { tankDrive.stop(); tankDrive.setThrottleAccelerationScale(val); } ),
    WSlider("Throttle Deceleration", "throttleDecel", 1, 400,
        []()->int { return tankDrive.getThrottleDecelerationScale(); },
        [](int val) { tankDrive.stop(); tankDrive.setThrottleDecelerationScale(val); } ),
    WSlider("Turn Acceleration", "turnAccel", 1, 400,
        []()->int { return tankDrive.getTurnAccelerationScale(); },
        [](int val) { tankDrive.stop(); tankDrive.setTurnAccelerationScale(val); } ),
    WSlider("Turn Deceleration", "turnDecl", 1, 400,
        []()->int { return tankDrive.getTurnDecelerationScale(); },
        [](int val) { tankDrive.stop(); tankDrive.setTurnDecelerationScale(val); } ),
    WCheckbox("Scale Throttle", "scaling",
        []() { return tankDrive.getScaling(); },
        [](bool val) { tankDrive.stop(); tankDrive.setScaling(val); } ),
    WCheckbox("Channel Mixing", "mixing",
        []() { return tankDrive.getChannelMixing(); },
        [](bool val) { tankDrive.stop(); tankDrive.setChannelMixing(val); } ),
    WCheckbox("Throttle Inverted", "throttleInvert",
        []() { return tankDrive.getThrottleInverted(); },
        [](bool val) { tankDrive.stop(); tankDrive.setThrottleInverted(val); } ),
    WCheckbox("Turn Inverted", "turnInvert",
        []() { return tankDrive.getTurnInverted(); },
        [](bool val) { tankDrive.stop(); tankDrive.setTurnInverted(val); } ),
#if DOME_DRIVE != DOME_DRIVE_NONE
    WCheckbox("Dome Inverted", "domeInvert",
        []() { return domeDrive.getInverted(); },
        [](bool val) { domeDrive.stop(); domeDrive.setInverted(val); } ),
#endif
#ifdef MARC_SOUND
    WHR(),
    WH1("Sound Configuration"),
    WSelect("Sound Player", "soundPlayer",
        soundPlayer, SizeOfArray(soundPlayer),
        []() { return (marcSoundPlayer = preferences.getInt(PREFERENCE_MARCSOUND, MARC_SOUND_PLAYER)); },
        [](int val) { marcSoundPlayer = val; } ),
    WSlider("Sound Volume", "soundVolume", 0, 1000,
        []() { return (marcSoundVolume = preferences.getInt(PREFERENCE_MARCSOUND_VOLUME, MARC_SOUND_VOLUME)); },
        [](int val) {
            marcSoundVolume = val;
            sMarcSound.setVolume(marcSoundVolume / 1000.0f);
        } ),
    WTextFieldInteger("Startup Sound", "soundStartup",
        []()->String { return String(marcSoundStartup = preferences.getInt(PREFERENCE_MARCSOUND_STARTUP, MARC_SOUND_STARTUP)); },
        [](String val) { marcSoundStartup = val.toInt(); }),
    WCheckbox("Random Sound", "soundRandom",
        []() { return (marcSoundRandom = preferences.getBool(PREFERENCE_MARCSOUND_RANDOM, MARC_SOUND_RANDOM)); },
        [](bool val) { marcSoundRandom = val; } ),
    WTextFieldInteger("Random Min Millis", "soundRandomMin",
        []()->String { return String(marcSoundRandomMin = preferences.getInt(PREFERENCE_MARCSOUND_RANDOM_MIN, MARC_SOUND_RANDOM_MIN)); },
        [](String val) { marcSoundRandomMin = val.toInt(); }),
    WTextFieldInteger("Random Max Millis", "soundRandomMax",
        []()->String { return String(marcSoundRandomMax = preferences.getInt(PREFERENCE_MARCSOUND_RANDOM_MAX, MARC_SOUND_RANDOM_MAX)); },
        [](String val) { marcSoundRandomMax = val.toInt(); }),
#endif
    WButton("Save", "save", []() {
        preferences.putFloat(PREFERENCE_DRIVE_SPEED, tankDrive.getMaxSpeed());
        preferences.putFloat(PREFERENCE_DRIVE_THROTTLE_ACC_SCALE, tankDrive.getThrottleAccelerationScale());
        preferences.putFloat(PREFERENCE_DRIVE_THROTTLE_DEC_SCALE, tankDrive.getThrottleDecelerationScale());
        preferences.putFloat(PREFERENCE_DRIVE_TURN_ACC_SCALE, tankDrive.getTurnAccelerationScale());
        preferences.putFloat(PREFERENCE_DRIVE_TURN_DEC_SCALE, tankDrive.getTurnDecelerationScale());
        preferences.putFloat(PREFERENCE_DRIVE_GUEST_SPEED, tankDrive.getGuestSpeedModifier());
        preferences.putBool(PREFERENCE_DRIVE_SCALING, tankDrive.getScaling());
        preferences.putBool(PREFERENCE_DRIVE_MIXING, tankDrive.getChannelMixing());
        preferences.putBool(PREFERENCE_DRIVE_THROTTLE_INVERT, tankDrive.getThrottleInverted());
        preferences.putBool(PREFERENCE_DRIVE_TURN_INVERT, tankDrive.getTurnInverted());
    #if DOME_DRIVE != DOME_DRIVE_NONE
        preferences.putBool(PREFERENCE_DOME_DRIVE_INVERT, domeDrive.getInverted());
    #endif
    #ifdef MARC_SOUND
        if (marcSoundRandomMin > marcSoundRandomMax)
        {
            int t = marcSoundRandomMin;
            marcSoundRandomMin = marcSoundRandomMax;
            marcSoundRandomMax = t;
        }
        preferences.putInt(PREFERENCE_MARCSOUND, marcSoundPlayer);
        preferences.putInt(PREFERENCE_MARCSOUND_VOLUME, marcSoundVolume);
        preferences.putInt(PREFERENCE_MARCSOUND_STARTUP, marcSoundStartup);
        preferences.putBool(PREFERENCE_MARCSOUND_RANDOM, marcSoundRandom);
        preferences.putInt(PREFERENCE_MARCSOUND_RANDOM_MIN, marcSoundRandomMin);
        preferences.putInt(PREFERENCE_MARCSOUND_RANDOM_MAX, marcSoundRandomMax);

        sMarcSound.begin((MarcSound::Module)marcSoundPlayer, SOUND_SERIAL, marcSoundStartup);
        sMarcSound.setVolume(marcSoundVolume / 1000.0f);
        sMarcSound.setRandomMin(marcSoundRandomMin);
        sMarcSound.setRandomMax(marcSoundRandomMax);
        if (marcSoundRandom)
            sMarcSound.startRandom();
        else
            sMarcSound.stopRandom();
    #endif
    }),
    WButton("Restore", "restore", []() {
        tankDrive.setMaxSpeed(preferences.getFloat(PREFERENCE_DRIVE_SPEED, MAXIMUM_SPEED));
        tankDrive.setThrottleAccelerationScale(preferences.getFloat(PREFERENCE_DRIVE_THROTTLE_ACC_SCALE, ACCELERATION_SCALE));
        tankDrive.setThrottleDecelerationScale(preferences.getFloat(PREFERENCE_DRIVE_THROTTLE_DEC_SCALE, DECELERATION_SCALE));
        tankDrive.setTurnAccelerationScale(preferences.getFloat(PREFERENCE_DRIVE_TURN_ACC_SCALE, ACCELERATION_SCALE*2));
        tankDrive.setTurnDecelerationScale(preferences.getFloat(PREFERENCE_DRIVE_TURN_DEC_SCALE, DECELERATION_SCALE));
        tankDrive.setGuestSpeedModifier(preferences.getFloat(PREFERENCE_DRIVE_GUEST_SPEED, MAXIMUM_GUEST_SPEED));
        tankDrive.setScaling(preferences.getBool(PREFERENCE_DRIVE_SCALING, SCALING));
        tankDrive.setChannelMixing(preferences.getBool(PREFERENCE_DRIVE_MIXING, CHANNEL_MIXING));
        tankDrive.setThrottleInverted(preferences.getBool(PREFERENCE_DRIVE_THROTTLE_INVERT, THROTTLE_INVERTED));
        tankDrive.setTurnInverted(preferences.getBool(PREFERENCE_DRIVE_TURN_INVERT, TURN_INVERTED));
    #if DOME_DRIVE != DOME_DRIVE_NONE
        domeDrive.setInverted(preferences.getBool(PREFERENCE_DOME_DRIVE_INVERT, DOME_INVERTED));
    #endif
    #ifdef MARC_SOUND
        marcSoundPlayer = preferences.getInt(PREFERENCE_MARCSOUND, MARC_SOUND_PLAYER);
        marcSoundVolume = preferences.getInt(PREFERENCE_MARCSOUND_VOLUME, MARC_SOUND_VOLUME);
        marcSoundStartup = preferences.getInt(PREFERENCE_MARCSOUND_STARTUP, MARC_SOUND_STARTUP);
        marcSoundRandom = preferences.getBool(PREFERENCE_MARCSOUND_RANDOM, MARC_SOUND_RANDOM);
        marcSoundRandomMin = preferences.getInt(PREFERENCE_MARCSOUND_RANDOM_MIN, MARC_SOUND_RANDOM_MIN);
        marcSoundRandomMax = preferences.getInt(PREFERENCE_MARCSOUND_RANDOM_MAX, MARC_SOUND_RANDOM_MAX);
        sMarcSound.begin((MarcSound::Module)marcSoundPlayer, SOUND_SERIAL, marcSoundStartup);
        sMarcSound.setVolume(marcSoundVolume / 1000.0f);
        sMarcSound.setRandomMin(marcSoundRandomMin);
        sMarcSound.setRandomMax(marcSoundRandomMax);
        if (marcSoundRandom)
            sMarcSound.startRandom();
        else
            sMarcSound.stopRandom();
    #endif
    }),
    WImage("astromech", ASTROMECH_IMAGE)
};

WPage pages[] = {
    WPage("/", mainContents, SizeOfArray(mainContents))
};

WifiWebServer<1,SizeOfArray(pages)> webServer(pages, wifiAccess);
#endif
