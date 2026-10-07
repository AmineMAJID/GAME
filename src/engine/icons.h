#pragma once
// ============================================================
// FarmVale — icônes pixel-art 16x16 (procédurales)
// ============================================================
#include "engine.h"

namespace fe {

// Dessine l'icône `id` en (x,y). q = qualité (0..2) -> liseré argent/or.
void drawIcon(Framebuffer& fb, int x, int y, int id, int q = 0);

// Icônes d'interface (ids >= 100)
enum UiIcon {
    IC_HEART = 100, IC_COIN, IC_BOLT, IC_CLOCK,
    IC_SUN, IC_CLOUD, IC_RAIN, IC_STORM, IC_SNOW,
    IC_STAR, IC_QUEST, IC_SLEEP, IC_ARROW_R, IC_ARROW_D,
    IC_BAG, IC_CHEST, IC_PAUSE, IC_NOTE, IC_MUTE,
    IC_CHECK, IC_CROSS, IC_FISH, IC_LEAF, IC_HOME
};

} // namespace fe
