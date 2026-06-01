#include "PlayerObject.hpp"

ProPlayerObject::Fields::Fields() {
    megahackLoaded = Loader::get()->isModLoaded("absolllute.megahack");
}

bool ProPlayerObject::isCube() {
    return !m_isDart
        && !m_isBall
        && !m_isBird
        && !m_isShip
        && !m_isSwing
        && !m_isRobot
        && !m_isSpider;
}

bool ProPlayerObject::isTrailEnabled(bool ignoreWave) {
    return (m_isDart && !ignoreWave)
        || (m_isShip && getSetting<"enable-ship-trail", bool>())
        || (m_isBird && getSetting<"enable-ufo-trail", bool>())
        || (m_isSwing && getSetting<"enable-swing-trail", bool>())
        || (m_isRobot && getSetting<"enable-robot-trail", bool>())
        || (m_isSpider && getSetting<"enable-spider-trail", bool>())
        || (m_isBall && getSetting<"enable-ball-trail", bool>())
        || (isCube() && getSetting<"enable-cube-trail", bool>());
    }

bool ProPlayerObject::isPointOffscreen(const CCPoint& point) {
    auto winSize = CCDirector::get()->getWinSize();
    auto pos = m_gameLayer->m_objectLayer->convertToWorldSpaceAR(point);
    auto cameraCenter = m_gameLayer->m_cameraObb2->m_center;
    auto angle = CC_DEGREES_TO_RADIANS(-m_gameLayer->m_gameState.m_cameraAngle);
    auto cosA = cosf(angle);
    auto sinA = sinf(angle);
    auto offsetX = pos.x - cameraCenter.x;
    auto offsetY = pos.y - cameraCenter.y;
    auto rotatedX = offsetX * cosA - offsetY * sinA;
    auto rotatedY = offsetX * sinA + offsetY * cosA;

    pos = ccp(cameraCenter.x + rotatedX, cameraCenter.y + rotatedY);

    return pos.x < 0 || pos.y < 0 || pos.x > winSize.width || pos.y > winSize.height;
}

void ProPlayerObject::copyTrailProperties(HardStreak* trail) {
    if (auto fakeTrail = m_fields->fakeTrail; fakeTrail && trail != fakeTrail) {
        copyTrailProperties(fakeTrail);
    }

    if (!trail) {
        return;
    }

    trail->setColor(m_waveTrail->getColor());
    trail->setBlendFunc(m_waveTrail->getBlendFunc());
    trail->m_waveSize = m_waveTrail->m_waveSize;
    trail->m_isSolid = m_waveTrail->m_isSolid;
}

void ProPlayerObject::spawnFakeTrail(float fadeOut, CCPoint extraPoint) {
    auto f = m_fields.self();
    auto trail = f->newTrail;

    if (!trail || trail->m_pointArray->count() <= 0) {
        if (f->fakeTrail && fadeOut > 0.f) {
            f->fakeTrail->schedule(schedule_selector(HardStreak::updateStroke));
            f->fakeTrail->runAction(CCSequence::create(
                CCFadeOut::create(fadeOut),
                CallFuncExt::create([this] {
                    killFakeTrail();
                }),
                nullptr
            ));
        }

        return;
    }

    if (f->fakeTrail) {
        killFakeTrail();
    }

    auto newTrail = HardStreak::create();
    newTrail->setID("fake-trail"_spr);
    newTrail->setTag(m_isSecondPlayer ? 2 : 1);
    newTrail->m_drawStreak = true;

    m_parentLayer->addChild(newTrail, 3);

    copyTrailProperties(newTrail);

    trail->setVisible(false);

    CCPoint lastPoint;

    for (auto pointNode : CCArrayExt<PointNode*>(trail->m_pointArray)) {
        newTrail->addPoint(pointNode->m_point);
        lastPoint = pointNode->m_point;
    }

    if (extraPoint != CCPoint{0, 0}) {
        newTrail->addPoint(extraPoint);
        lastPoint = extraPoint;
    }

    newTrail->m_currentPoint = lastPoint;
    newTrail->updateStroke(0.f);

    f->fakeTrail = newTrail;

    if (fadeOut > 0.f) {
        newTrail->schedule(schedule_selector(HardStreak::updateStroke));
        newTrail->runAction(CCSequence::create(
            CCFadeOut::create(fadeOut),
            CallFuncExt::create([this] {
                killFakeTrail();
            }),
            nullptr
        ));
    }
}

void ProPlayerObject::killFakeTrail() {
    auto f = m_fields.self();

    if (f->fakeTrail){
        f->fakeTrail->m_pointArray->removeAllObjects();
        f->fakeTrail->clear();
        f->fakeTrail->removeFromParent();
        f->fakeTrail = nullptr;
    }
}

void ProPlayerObject::justDied() {
    auto trail = m_fields->newTrail;

    if (!trail) {
        return;
    }

    spawnFakeTrail(0.2f);
}

void ProPlayerObject::updateSettings() {
    auto f = m_fields.self();
    
    if (getSetting<"enable-trail-rgb", bool>() && !f->didScheduleUpdate) {
        schedule(schedule_selector(ProPlayerObject::updateTrailRGB));
        f->didScheduleUpdate = true;
    } else if (!getSetting<"enable-trail-rgb", bool>() && f->didScheduleUpdate) {
        unschedule(schedule_selector(ProPlayerObject::updateTrailRGB));
        f->didScheduleUpdate = false;
    }

    updateSolidTrail();
    updateTrailSize();
    updateRegularTrail();
    updateParticles();
    updateNewTrail();
    updateTrailColor();

    m_waveTrail->setVisible(getSetting<"enable-wave-trail", bool>());

    copyTrailProperties(f->newTrail);

    f->rgbTime = 0.f;
}

void ProPlayerObject::updateTrailColor() {
    if (getSetting<"enable-trail-rgb", bool>()) {
        return;
    }

    auto f = m_fields.self();

    if (
        getSetting<"enable-trail-color", bool>()
        || getSetting<"use-main-player-color", bool>()
        || getSetting<"use-secondary-player-color", bool>()
        || getSetting<"use-glow-player-color", bool>()
    ) {
        if (!f->didSetColor) {
            f->didSetColor = true;
            f->originalColor = m_waveTrail->getColor();
        }
        
        auto gm = GameManager::get();
        auto color = ccColor3B{};

        if (getSetting<"enable-trail-color", bool>()) {
            color = getSetting<"trail-color", ccColor3B>();
        } else if (getSetting<"use-main-player-color", bool>()) {
            color = gm->colorForIdx(m_isSecondPlayer ? gm->getPlayerColor2() : gm->getPlayerColor());
        } else if (getSetting<"use-secondary-player-color", bool>()) {
            color = gm->colorForIdx(m_isSecondPlayer ? gm->getPlayerColor() : gm->getPlayerColor2());
        } else if (getSetting<"use-glow-player-color", bool>()) {
            color = gm->colorForIdx(gm->getPlayerGlowColor());
        }

        m_waveTrail->setColor(color);
    } else if (f->didSetColor) {
        m_waveTrail->setColor(f->originalColor);
    }
}

void ProPlayerObject::updateSolidTrail() {
    auto f = m_fields.self();

    if (getSetting<"solid-wave-trail", bool>()) {
        if (!f->didSetSolid) {
            f->didSetSolid = true;
            f->originalBlendFunc = m_waveTrail->getBlendFunc();
            f->wasSolid = m_waveTrail->m_isSolid;
        }

        m_waveTrail->setBlendFunc({ GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA });
        m_waveTrail->m_isSolid = true;
    } else if (f->didSetSolid) {
        f->didSetSolid = false;
        m_waveTrail->setBlendFunc(f->originalBlendFunc);
        m_waveTrail->m_isSolid = f->wasSolid;
    } else if (getSetting<"enable-trail-color", bool>() && m_waveTrail->getColor() == ccColor3B{0, 0, 0}) {
        m_waveTrail->setBlendFunc({ GL_SRC_ALPHA, GL_ONE });
        m_waveTrail->m_isSolid = false;
    }

    copyTrailProperties(f->newTrail);
}

void ProPlayerObject::updateTrailSize() {
    auto f = m_fields.self();
    auto value = getSetting<"trail-size", float>();

    if (value != 10.f) {
        if (!f->didSetSize) {
            f->didSetSize = true;
            f->originalSize = m_waveTrail->m_waveSize;
        }

        m_waveTrail->m_waveSize = value / 10.f * m_vehicleSize;
    } else if (f->didSetSize) {
        f->didSetSize = false;
        m_waveTrail->m_waveSize = f->originalSize;
    }
    
    copyTrailProperties(f->newTrail);
}

void ProPlayerObject::updateTrailPulse() {
    if (getSetting<"disable-pulse", bool>()) {
        m_waveTrail->m_pulseSize = 1.4f;
    }
}

void ProPlayerObject::updateRegularTrail() {
    auto doHide = getSetting<"hide-regular-trail", bool>() && isTrailEnabled();

    if (!doHide) {
        auto f = m_fields.self();

        if (f->didHideTrail) {
            f->didHideTrail = false;
            m_regularTrail->setVisible(true);
        }

        return;
    }

    if (m_regularTrail->isVisible()) {
        m_fields->didHideTrail = true;
    }

    m_regularTrail->setVisible(false);
}

void ProPlayerObject::updateParticles() {
    auto doHide = getSetting<"hide-particles", bool>() && isTrailEnabled();

    m_playerGroundParticles->setVisible(!doHide);
    m_trailingParticles->setVisible(!doHide);
    m_shipClickParticles->setVisible(!doHide);
    m_vehicleGroundParticles->setVisible(!doHide);
    m_ufoClickParticles->setVisible(!doHide);
    m_robotBurstParticles->setVisible(!doHide);
    m_dashParticles->setVisible(!doHide);
    m_swingBurstParticles1->setVisible(!doHide);
    m_swingBurstParticles2->setVisible(!doHide);
    m_landParticles0->setVisible(!doHide);
    m_landParticles1->setVisible(!doHide);
}

void ProPlayerObject::updateNewTrail(float dt) {
    auto f = m_fields.self();

    if (f->megahackLoaded) {
        updateTrailColor();
    }

    if (m_isPlatformer && !getSetting<"trail-in-platformer", bool>()) {
        if (f->fakeTrail) {
            killFakeTrail();
        }

        if (f->newTrail && f->newTrail->isVisible()) {
            f->newTrail->clear();
            f->newTrail->m_pointArray->removeAllObjects();
            f->newTrail->m_drawStreak = false;
            f->newTrail->setVisible(false);
        }

        return;
    }

    auto showTrail = isTrailEnabled(true) && !m_isDead;

    if (f->fakeTrail) {
        f->fakeTrail->m_pulseSize = m_waveTrail->m_pulseSize;

        auto pointArray = CCArrayExt<PointNode*>(f->fakeTrail->m_pointArray);

        for (int i = 0; i < pointArray.size() - 1; i++) {
            auto pointNode = pointArray[i];
            if (isPointOffscreen(pointNode->m_point) && isPointOffscreen(pointArray[i + 1]->m_point)) {
                f->fakeTrail->m_pointArray->removeObject(pointNode, true);
            }
        }
        
        if (f->fakeTrail->m_pointArray->count() <= 0) {
            killFakeTrail();
        } else {
            f->fakeTrail->updateStroke(dt);
        }
    }

    if (!showTrail || m_isHidden) {
        if (f->newTrail && f->newTrail->isVisible()) {
            spawnFakeTrail(m_isDart ? 0.f : 0.2f, m_isDart ? getPosition() : CCPoint{0, 0});

            f->newTrail->setVisible(false);
            f->newTrail->m_drawStreak = false;
            f->newTrail->m_pointArray->removeAllObjects();
            f->newTrail->clear();
        }

        return;
    }

    if (!f->newTrail) {
        f->newTrail = HardStreak::create();
        f->newTrail->setID("new-trail"_spr);
        f->newTrail->setTag(m_isSecondPlayer ? 2 : 1);
        f->newTrail->m_drawStreak = true;
        f->newTrail->setVisible(false);

        m_parentLayer->addChild(f->newTrail, 3);

        copyTrailProperties(f->newTrail);

        f->wasGoingLeft = m_isGoingLeft;
    }

    f->newTrail->m_pulseSize = m_waveTrail->m_pulseSize;

    if (f->wasGoingLeft != m_isGoingLeft) {
        f->newTrail->m_pointArray->removeAllObjects();
        f->newTrail->clear();
    }

    f->wasGoingLeft = m_isGoingLeft;

    if (!f->newTrail->isVisible()) {
        f->newTrail->setVisible(true);
        f->newTrail->m_drawStreak = true;
        f->newTrail->m_pointArray->removeAllObjects();
        f->newTrail->clear();

        if (getSetting<"enable-wave-trail", bool>()) {
            for (auto pointNode : CCArrayExt<PointNode*>(m_waveTrail->m_pointArray)) {
                f->newTrail->addPoint(pointNode->m_point);
            }
        }

        m_waveTrail->stopStroke();
    }

    CCPoint position = getPosition();

    if ((m_isShip || m_isBird) && getSetting<"do-offset", bool>()) {
        position = m_gameLayer->m_objectLayer->convertToNodeSpaceAR(
            m_vehicleSprite->convertToWorldSpaceAR(
                (m_isShip ? CCPoint{-6.f, -4.f} : CCPoint{0, -2.45f}) * m_vehicleSize
            )
        );
    }

    auto doAdd = true;

    if (f->newTrail->m_pointArray->count() > 0) {
        auto lastPoint = static_cast<PointNode*>(m_isGoingLeft ? f->newTrail->m_pointArray->firstObject() : f->newTrail->m_pointArray->lastObject());

        if (ccpDistance(lastPoint->m_point, position) <= getSetting<"point-threshold", float>()) {
            doAdd = false;
        }
    }

    if (m_isGoingLeft) {
        if (doAdd) {
            auto node = PointNode::create(position);
            f->newTrail->m_pointArray->insertObject(node, 0);
        }

        f->newTrail->m_currentPoint = static_cast<PointNode*>(f->newTrail->m_pointArray->lastObject())->m_point;
    } else {
        if (doAdd) {
            f->newTrail->addPoint(position);
        }

        f->newTrail->m_currentPoint = position;
    }

    while (f->newTrail->m_pointArray->count() > getSetting<"length-limit", int>()) {
        if (m_isGoingLeft) {
            f->newTrail->m_pointArray->removeFirstObject(true);
        } else {
            f->newTrail->m_pointArray->removeLastObject(true);
        }
    }
    
    auto pointArray = CCArrayExt<PointNode*>(f->newTrail->m_pointArray);

    for (int i = 0; i < pointArray.size() - 1; i++) {
        auto pointNode = pointArray[i];
        if (isPointOffscreen(pointNode->m_point) && isPointOffscreen(pointArray[i + 1]->m_point)) {
            f->newTrail->m_pointArray->removeObject(pointNode, true);
        }
    }
    
    f->newTrail->updateStroke(dt);
}

void ProPlayerObject::updateTrailRGB(float dt) {
    if (!getSetting<"enable-trail-rgb", bool>()) {
        return;
    }

    auto f = m_fields.self();
    auto duration = getSetting<"trail-rgb-duration", float>();

    f->rgbTime += dt;

    if (f->rgbTime > duration) {
        f->rgbTime = f->rgbTime - duration;
    }

    auto t = f->rgbTime / duration;

    auto r = clampf(std::abs(6.f * t - 3.f) - 1.f, 0.f, 1.f);
    auto g = clampf(2.f - std::abs(6.f * t - 2.f), 0.f, 1.f);
    auto b = clampf(2.f - std::abs(6.f * t - 4.f), 0.f, 1.f);
    auto pastel = 1.f - getSetting<"trail-rgb-saturation", float>();

    r = r + (1.f - r) * pastel;
    g = g + (1.f - g) * pastel;
    b = b + (1.f - b) * pastel;

    auto color = ccc3(
        r * 255,
        g * 255,
        b * 255
    );

    m_waveTrail->setColor(color);

    if (f->newTrail) {
        f->newTrail->setColor(color);
    }

    if (f->fakeTrail) {
        f->fakeTrail->setColor(color);
    }
}

void ProPlayerObject::update(float dt) {
    PlayerObject::update(dt);

    if (isVanillaPlayer()) {
        updateTrailPulse();
        updateNewTrail(dt);
    }
}

void ProPlayerObject::updateStreakBlend(bool p0) {
    PlayerObject::updateStreakBlend(p0);

    if (isVanillaPlayer()) {
        m_fields->didSetSolid = false;
        updateSolidTrail();
    }
}

void ProPlayerObject::setupStreak() {
    PlayerObject::setupStreak();

    if (isVanillaPlayer()) {
        m_fields->didSetSolid = false;
        updateSolidTrail();
    }
}

void ProPlayerObject::resetStreak() {
    PlayerObject::resetStreak();

    if (isVanillaPlayer()) {
        m_fields->didSetSolid = false;
        updateSolidTrail();
    }

    if (auto newTrail = m_fields->newTrail) {
        newTrail->m_pointArray->removeAllObjects();
        newTrail->clear();
        newTrail->setVisible(false);
    }

    killFakeTrail();
}

void ProPlayerObject::togglePlayerScale(bool p0, bool p1) {
    PlayerObject::togglePlayerScale(p0, p1);

    if (isVanillaPlayer()) {
        m_fields->didSetSize = false;
        updateTrailSize();
    }
}

void ProPlayerObject::switchedToMode(GameObjectType p0) {
    PlayerObject::switchedToMode(p0);

    if (isVanillaPlayer()) {
        updateRegularTrail();
        updateParticles();
    }
}