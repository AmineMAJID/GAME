# FarmVale — Guide de publication Steam (Steam Direct)

Ce document explique comment transformer ce projet en produit Steam.
Le code est **déjà prêt** : les succès passent par `steam::unlock(id)`
(`src/game/achievements.h`) — sans Steam, ils sont juste journalisés.

---

## 1. Créer le compte et payer la taxe Steam Direct

1. Va sur https://partner.steamgames.com — connecte-toi / crée un compte **Steamworks**.
2. Steam Direct demande un **paiement unique de 100 $ USD** par produit
   (remboursé après 1 000 $ de revenus générés par le jeu).
3. Remplis les informations fiscales (TIN / W-8BEN selon ton pays) et bancaires.
4. Une fois approuvé (quelques jours), ton **AppID** est créé.

## 2. Télécharger le Steamworks SDK

1. Sur Steamworks, va dans **Help → Steamworks SDK** (ou https://partner.steamgames.com/downloads/list).
2. Télécharge **Steamworks SDK** (dernière version, ex. 1.62).
3. Décompresse-le, par ex. dans `C:\steamworks_sdk`.
4. Le jeu n'a **pas besoin** du SDK pour compiler sans Steam :
   `achievements.h` fait des appels no-op par défaut.

## 3. Activer Steam dans le jeu (5 minutes)

Dans `src/game/achievements.h`, remplace les blocs `TODO` marqué
`#ifdef FARMVALE_STEAM` :

```cpp
#include "steam_api.h"   // ajoute en haut du fichier

inline void init() {
#ifdef FARMVALE_STEAM
    if (SteamAPI_Init()) {
        SteamUserStats()->RequestCurrentStats();
    }
#endif
    enabledFlag() = true;
}

inline void unlock(const char* id) {
#ifdef FARMVALE_STEAM
    if (SteamUtils()->IsSteamRunning() /* optionnel */) {
        SteamUserStats()->SetAchievement(id);
        SteamUserStats()->StoreStats();
    }
#endif
    printf("[succès] %s débloqué !\n", id);
}

inline void shutdown() {
#ifdef FARMVALE_STEAM
    SteamAPI_Shutdown();
#endif
}
```

Compile ensuite avec `-DFARMVALE_STEAM` et lie `steam_api64.lib`
(fourni dans le SDK : `sdk/redistributable_bin/win64/steam_api64.dll` +
`steam_api64.lib`). Voir `build_windows.bat` (section STEAM).

**Important** : place `steam_api64.dll` **à côté de FarmVale.exe** dans le build
Steam. Sans ce fichier, le jeu ne démarre pas (ou refuse de se lancer hors Steam
selon tes réglages).

## 4. Les succès à configurer dans Steamworks

Va dans **Steamworks → Apps → [ton jeu] → Stats & Achievements → Achievements**
et crée exactement ces 8 succès (API Name = la clé, Display Name = le texte FR) :

| API Name (clé)      | Display Name (FR)              | Déclencheur dans le jeu                     |
|---------------------|--------------------------------|---------------------------------------------|
| ACH_FIRST_HARVEST   | « Ma première récolte »        | Première récolte                            |
| ACH_GREEN_THUMB     | « Main verte »                 | 100 récoltes au total                       |
| ACH_FIRST_ANIMAL    | « Premier compagnon »          | Premier animal acheté                       |
| ACH_FIRST_BREED     | « Éleveur »                    | Premier bébé né                             |
| ACH_FARMER_10       | « Agriculteur confirmé »       | Niveau 10 en Agriculture                    |
| ACH_FISHERMAN       | « Pêcheur »                    | 50 poissons pêchés                          |
| ACH_RICH            | « Prospère »                   | 50 000 pièces                               |
| ACH_ALL_SEASONS     | « Quatre saisons »             | Avoir survécu à l'hiver                     |

Chaque succès peut avoir une icône (72×72 px, PNG) — voir `steam/` pour
les artworks déjà générés.

## 5. Artworks de la page magasin

Les fichiers sont générés par `python3 tools/gen_art.py` dans `steam/` :

| Fichier                | Taille    | Où l'uploader (Steamworks)          |
|------------------------|-----------|-------------------------------------|
| `header_capsule.png`   | 460×215   | Store Page → Graphic Assets → Header capsule |
| `main_capsule.png`     | 616×353   | Store Page → Main capsule           |
| `small_capsule.png`    | 231×87    | Store Page → Small capsule          |
| `vertical_capsule.png` | 374×448   | Library → Vertical capsule          |
| `library_hero.png`     | 1920×620  | Library → Hero artwork              |
| `library_logo.png`     | 960×360   | Library → Logo (transparent)        |

Tu peux remplacer ces visuels par des illustrations professionnelles :
garde **exactement les mêmes tailles**.

## 6. Construire la version à uploader

### Windows (recommandé pour Steam)
```bat
:: Avec MinGW-w64 installé (https://www.mingw-w64.org/downloads/)
build_windows.bat
```
Produit `build\FarmVale.exe`. Copie aussi `steam_api64.dll` à côté si Steam
est activé.

### Cross-compilation depuis Linux (Zig, sans rien installer d'autre)
```sh
pip install ziglang            # une seule fois
make windows                   # produit build/FarmVale.exe
# ou : bash tools/build_windows.sh
```

## 7. Uploader le build (SteamPipe)

1. Télécharge **SteamPipe** (Steamworks → Downloads → SteamPipe).
2. Crée un script `app_build.vdf` (exemple dans `steam/app_build.vdf`).
3. Lance :
   ```
   steamcmd.exe +login <ton_compte_steam> +run_app_build <chemin_app_build.vdf> +quit
   ```
   (Steam Guard peut demander un code — normal.)
4. Le build apparaît dans l'onglet **Builds** de Steamworks.

## 8. Dépôt Steam (branches)

- Branche `default` : build **public** (ce que les joueurs téléchargent).
- Branche `beta` : build de test (mot de passe « betatest » à partager).

Mets à jour via : Steamworks → Builds → « Définir comme build par défaut ».

## 9. Checklist avant mise en vente

- [ ] Compte Steam Direct payé (100 $) et AppID créé
- [ ] Page magasin remplie (description FR/EN, tags, prix)
- [ ] 8 succès créés et testés en jeu (Steamworks → teste avec le client Steam)
- [ ] Capsules uploadées (toutes tailles)
- [ ] Build Windows uploadé via SteamPipe, testé sur la branche beta
- [ ] `steam_api64.dll` embarqué dans le dossier du build
- [ ] Bande-annonce (30–60 s) + 5–10 screenshots (voir `build/shot_*.png`)
- [ ] Classification (PEGI/ESRB) remplie dans le questionnaire
- [ ] Prix régional conseillé : 4,99 € – 9,99 € (ajuste selon ton marché)

## 10. Liens utiles

- Steamworks : https://partner.steamgames.com
- Doc SteamPipe : https://partner.steamgames.com/doc/features/builds
- Doc succès : https://partner.steamgames.com/doc/features/achievements
- Steam Direct (100 $) : https://partner.steamgames.com/doc/getting_started

---

*Note : FarmVale est un projet indépendant. Aucun contenu sous copyright
n'est utilisé ; tous les visuels, sons et musiques sont générés/libres.*
