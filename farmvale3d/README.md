# 🌾 FarmVale 3D — La Ferme des Quatre Saisons

Jeu de **simulation de ferme en 3D**, moderne et stylisé, écrit avec **Godot 4.7** (GDScript).
UI et commentaires entièrement en **français**. Rendu `forward_plus`, shading **toon**
intégré (`diffuse_toon`, `specular_toon`), éclairage chaud, bloom, SSAO, tonemap ACES,
ciel procédural avec cycle jour/nuit complet.

## ▶️ Lancer le jeu

1. Télécharger **Godot 4.7.2** (Standard, 64-bit) : https://godotengine.org/download
2. Ouvrir le dossier **`farmvale3d/`** (bouton « Importer » → sélectionner le dossier contenant `project.godot`)
3. Appuyer sur **F5** (ou ▶ « Lancer le projet »)

## 🎮 Contrôles

| Touche | Action |
|---|---|
| ZQSD / WASD | Se déplacer (relatif à la caméra) |
| Maj (Shift) | Courir |
| Souris | Orbite de la caméra (3e personne) |
| Molette | Zoom |
| E / F | Interagir |
| 1 à 9 | Barre d'objets rapide (hotbar) |
| Échap | Pause / reprendre |
| Clic droit (bouton relâché) | Libérer la souris |

## ✨ Ce qui est déjà dans le jeu

- **Monde 3D procédural complet** : maison avec toit en tuiles et fenêtres lumineuses la nuit,
  2 planches de 12 parcelles labourées (24 cultures 3D : blé, fraises, citrouilles, maïs, tomates),
  12 arbres fruitiers, clôture complète, étang avec nénuphars et roseaux, 150 fleurs,
  6 nuages, 6 animaux (poules, vache, moutons) avec IA d'errance, papillons, oiseaux,
  feuilles mortes en particules et **lucioles la nuit**.
- **Cycle jour/nuit** : 1 heure de jeu = 15 secondes réelles, 24 h, 4 saisons de 28 jours,
  soleil qui tourne, ciel procédural (shader) avec aube/crépuscule, **étoiles et lune la nuit**.
- **Joueur 3D** en primitives stylisées (toon), animé (marche, balancement).
- **HUD moderne** : monnaie, saison/jour, horloge, météo avec icônes, barre d'énergie,
  hotbar 9 slots, toasts, croix de visée, menu pause.
- **Menu principal** animé avec fond artistique, options (volumes, plein écran), sauvegarde.
- **Sauvegarde JSON** (`user://farmvale_save.json`) + réglages persistants.
- **Effets sonores 100 % synthétisés** (aucun asset externe requis) sur 3 bus audio
  (Master / Musique / SFX) réglables séparément.
- **Test automatique headless (58 vérifications)** : `scenes/tools/self_test.tscn` vérifie tout le projet
  sans fenêtre (monde, props, mouvement, temps, HUD, sauvegarde).

## 🧪 Lancer le test automatique (sans fenêtre)

```bash
godot --headless --path farmvale3d res://scenes/tools/self_test.tscn
```

Code de sortie **0** = tout est vert, **1** = un échec est détecté.

## 📁 Structure

```
farmvale3d/
├── project.godot              # config (scène principale, autoload, InputMap, couches)
├── icon.png                   # icône du jeu
├── shaders/                   # sky, ground (collines), toon, foliage (vent), water
├── scripts/
│   ├── autoload/game_manager.gd   # transitions, sauvegarde, audio, réglages
│   ├── ui/                    # menu principal, HUD, thème UI
│   ├── world/                 # joueur, caméra, temps, props, animaux, cultures
│   └── tools/self_test.gd     # test headless complet
├── scenes/                    # main_menu, world, player, camera_rig, hud, self_test
└── assets/                    # textures (fond menu), police DejaVu
```

## 🪟 Exporter pour Windows (à faire sur ton PC)

1. **Project → Export** → ajouter le preset **Windows Desktop**
2. Télécharger les **templates d'export** officiels (bouton « Gérer les templates
   d'export » → « Télécharger et installer ») — nécessite Internet sur ton PC
3. Cocher « Application autonome » (embed), renseigner l'icône `icon.png`
4. **Exporter** → un vrai `.exe` Windows 64-bit est produit

## 🗺️ Prochaines étapes (gameplay)

- [ ] Plantation/arrosage/récolte des cultures (interaction E sur une parcelle)
- [ ] Outils (houe, arrosoir, faux) dans la hotbar
- [ ] Élevage : nourrir les animaux, ramasser œufs/lait/laine
- [ ] PNJ villageois, quêtes et commerce
- [ ] Maisons/améliorations de ferme débloquables
- [ ] Sauvegarde des cultures entre les sessions
- [ ] Musique d'ambiance (générée ou assets libres)
- [ ] Localisation FR/EN complète
- [ ] Page Steam (Steamworks, captures d'écran, bande-annonce)

## 📜 Licence

Code et contenu générés pour ce projet. Police : DejaVu (libre).
