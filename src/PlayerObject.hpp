#pragma once

#include "Includes.hpp"

#include <Geode/modify/PlayerObject.hpp>

class $modify(ProPlayerObject, PlayerObject) {

    struct Fields {
        Fields();

        bool megahackLoaded = false;

        HardStreak* newTrail = nullptr;
        HardStreak* fakeTrail = nullptr;

        bool didSetColor = false;
        ccColor3B originalColor;

        bool didSetSolid = false;
        ccBlendFunc originalBlendFunc;
        bool wasSolid = false;

        bool didSetSize = false;
        float originalSize = 1.f;

        bool didHideTrail = false;
        bool wasGoingLeft = false;

        float rgbTime = 0.f;
        bool didScheduleUpdate = false;
    };

    bool isCube();
    bool isTrailEnabled(bool = false);
    bool isPointOffscreen(const CCPoint&);

    void copyTrailProperties(HardStreak*);
    void spawnFakeTrail(float, CCPoint = {0, 0});
    void killFakeTrail();
    void justDied();

    void updateSettings();
    void updateTrailColor();
    void updateSolidTrail();
    void updateTrailSize();
    void updateTrailPulse();
    void updateRegularTrail();
    void updateParticles();

    void updateNewTrail(float = 0.f);
    void updateTrailRGB(float);

    $override
    void update(float);
    void updateStreakBlend(bool);
    void setupStreak();
    void resetStreak();
    void togglePlayerScale(bool, bool);
    void switchedToMode(GameObjectType);

};