#ifndef HCRSoundMap_h
#define HCRSoundMap_h

#include <Arduino.h>

enum HCRSoundType
{
    kHCRSoundLeia,
    kHCRSoundSystem,
    kHCRSoundMusic
};

enum HCRSound
{
    kHCRLeiaHologram,
    kHCRTHX,
    kHCRStarTours,
    kHCRFoxFanfare,
    kHCRMainTitleTheme,
    kHCRCantina1,
    kHCRCantina2,
    kHCRImperialMarch,
    kHCRMedalCeremony
};

struct HCRSoundConfig
{
    HCRSound sound;
    uint16_t number;
    HCRSoundType type;
    const char* name;
};

static constexpr HCRSoundConfig HCR_SOUND_MAP[] = {
    { kHCRLeiaHologram, 0,   kHCRSoundLeia,   "LeiaHologram" },
    { kHCRTHX,          1,   kHCRSoundSystem, "THX" },
    { kHCRStarTours,    2,   kHCRSoundSystem, "StarTours" },
    { kHCRFoxFanfare,   100, kHCRSoundMusic,  "FoxFanfare" },
    { kHCRMainTitleTheme, 101, kHCRSoundMusic, "MainTitleTheme" },
    { kHCRCantina1,     102, kHCRSoundMusic,  "Cantina1" },
    { kHCRCantina2,     103, kHCRSoundMusic,  "Cantina2" },
    { kHCRImperialMarch,104, kHCRSoundMusic,  "ImperialMarch" },
    { kHCRMedalCeremony,105, kHCRSoundMusic,  "MedalCeremony" }
};

inline const HCRSoundConfig* getHCRSoundConfig(HCRSound sound)
{
    for (unsigned i = 0; i < sizeof(HCR_SOUND_MAP) / sizeof(HCR_SOUND_MAP[0]); i++)
    {
        if (HCR_SOUND_MAP[i].sound == sound)
            return &HCR_SOUND_MAP[i];
    }
    return nullptr;
}

#endif
