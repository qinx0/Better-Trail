#include "PlayLayer.hpp"
#include "PlayerObject.hpp"

void ProPlayLayer::updateSettings() {
    static_cast<ProPlayerObject*>(m_player1)->updateSettings();
    static_cast<ProPlayerObject*>(m_player2)->updateSettings();
}

void ProPlayLayer::setupHasCompleted() {
    PlayLayer::setupHasCompleted();
    static_cast<ProPlayerObject*>(m_player1)->m_fields->isRealPlayer = true;
    static_cast<ProPlayerObject*>(m_player2)->m_fields->isRealPlayer = true;
    updateSettings();
}

void ProPlayLayer::resetLevel() {
    PlayLayer::resetLevel();
    static_cast<ProPlayerObject*>(m_player1)->m_fields->isRealPlayer = true;
    static_cast<ProPlayerObject*>(m_player2)->m_fields->isRealPlayer = true;
    updateSettings();
}

void ProPlayLayer::destroyPlayer(PlayerObject* p0, GameObject* p1) {
if (!m_player1->m_isDead && p1 != m_anticheatSpike) {
        PlayLayer::destroyPlayer(p0, p1);

        static_cast<ProPlayerObject*>(m_player1)->justDied();
        static_cast<ProPlayerObject*>(m_player2)->justDied();

        return;
    }

    PlayLayer::destroyPlayer(p0, p1);
}