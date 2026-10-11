// ============================================================
// FarmVale — point d'entrée
// Sélection de la plateforme à la compilation :
//   -DFARMVALE_HEADLESS  -> headless (tests / captures)
//   -D_WIN32             -> Windows (Win32 + GDI, pour Steam)
//   sinon (Linux/macOS)  -> X11
// ============================================================
#include "engine/engine.h"
#include "game/game.h"
#include "platform/platform.h"
#include <cstdio>
#include <cstring>
#include <string>

int farmvale_main(int argc, char** argv) {
    setvbuf(stdout, nullptr, _IONBF, 0); // logs visibles même en pipe
    bool botMode = false;
    int botDays = 9;
    std::string shotPrefix = "build/shot_";

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--bot") == 0) { botMode = true; if (i + 1 < argc) botDays = atoi(argv[++i]); }
        else if (strcmp(argv[i], "--shots") == 0 && i + 1 < argc) shotPrefix = argv[++i];
        else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("FarmVale — ferme des quatre saisons\n"
                   "  --bot [jours]   mode démonstration automatique (headless)\n"
                   "  --shots <prefixe>  prefixe des captures (défaut: build/shot_)\n"
                   "  --frames <N>    nombre max de frames (headless)\n");
            return 0;
        }
    }

    fe::IPlatform* platform = createPlatform(argc, argv);
    if (!platform) {
        fprintf(stderr, "Impossible d'initialiser la plateforme graphique.\n");
        return 1;
    }
#ifdef FARMVALE_HEADLESS
    bool headlessBuild = true;
#else
    bool headlessBuild = false;
#endif
    fv::Game game;
    if (headlessBuild && !botMode) game.captureTitleShot = true;
    game.init(platform, botMode, botDays, shotPrefix);
    game.run();
    delete platform;
    return 0;
}
