#pragma once
// ============================================================
// FarmVale — succès (Steam achievements)
// Implémentation par défaut : no-op (journal console).
// Pour Steam : définir FARMVALE_STEAM et lier steam_api, puis
// remplacer unlock() par SteamUserStats()->SetAchievement(id).
// Voir steam/README_STEAM.md pour la procédure complète.
// ============================================================
#include <cstdio>

namespace steam {

inline bool& enabledFlag() { static bool e = false; return e; }

inline void init() {
#ifdef FARMVALE_STEAM
    // TODO: SteamAPI_Init() + SteamUserStats()->RequestCurrentStats()
#endif
    enabledFlag() = true;
}

inline void unlock(const char* id) {
#ifdef FARMVALE_STEAM
    // TODO: SteamUserStats()->SetAchievement(id); SteamUserStats()->StoreStats();
#endif
    (void)id;
    printf("[succès] %s débloqué !\n", id);
}

inline void shutdown() {
#ifdef FARMVALE_STEAM
    // TODO: SteamAPI_Shutdown()
#endif
}

} // namespace steam
