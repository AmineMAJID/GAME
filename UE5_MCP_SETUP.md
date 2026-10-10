# FarmVale — Pilotage de ton éditeur Unreal Engine 5 via MCP (par Arena)

Objectif : je développe FarmVale **dans TON éditeur UE5, à ta place**, via un serveur MCP.
Tu gardes le contrôle total : l'éditeur reste sur ton PC, en `localhost`, personne ne s'y connecte directement.

## Architecture (tunnel inverse)

```
  MOI (bac à sable Arena)                    TON PC
  ┌──────────────────┐    HTTPS sortant     ┌──────────────────────────┐
  │  Relai MCP       │◄────────────────────│  ue5_mcp_bridge.py       │
  │  (URL publique)  │   GET /poll (20 s)  │  (pont, stdlib Python)   │
  │  port 8765       │────────────────────►│        │                 │
  └──────────────────┘   POST /result      │        ▼                 │
                                            │  http://localhost:3000/mcp│
                                            │  (plugin McpAutomation-   │
                                            │   Bridge, TON éditeur)    │
                                            └──────────────────────────┘
```

- **Le relai** tourne chez moi (Arena) sur une URL publique. **Tes connexions sortantes uniquement** — aucun port ouvert chez toi.
- **Le pont** (`ue5_mcp_bridge.py`) tourne sur ton PC : il interroge le relai toutes les 20 s, exécute chaque requête sur `http://localhost:3000/mcp`, renvoie le résultat.
- Le relai a été **testé de bout en bout** (poignée de main MCP `initialize`, `tools/list`, `tools/call` avec session maintenue — `session_ok=True`).

## Étape 1 — Créer le projet UE5

1. Epic Launcher → Unreal Engine 5 → **Launch** → onglet **Games** → **Create Project**
2. Nom : `FarmVale` — **Blueprint** (pas C++), **With Starter Content** décoché, qualité **Maximum**.
3. Ferme l'éditeur.

## Étape 2 — Installer le plugin MCP (ChiR24/Unreal_mcp)

Dans le dossier du projet (`FarmVale/`) :

```bash
git clone https://github.com/ChiR24/Unreal_mcp.git Plugins/McpAutomationBridge
```

> Alternative sans compilation C++ : le serveur `sam-david/unreal-mcp` (Python, 127 outils) — me le dire si la compilation du plugin pose problème.

## Étape 3 — Activer le plugin et le serveur MCP natif

1. Ouvre `FarmVale.uproject` → l'éditeur compile le plugin (Visual Studio / Build Tools requis — si l'éditeur ne propose pas la compilation, lance « Generate Visual Studio project » via le clic droit sur le `.uproject`).
2. **Edit → Plugins** → recherche **MCP Automation Bridge** → **Enabled** → redémarre l'éditeur.
3. **Edit → Project Settings → Plugins → MCP Automation Bridge** :
   - ✅ **Enable Native MCP Server** (port **3000**, endpoint `/mcp`)
   - Laisse **Require Capability Token** ✅ (recommandé) → copie le **Capability Token** affiché, tu en auras besoin.
4. Redémarre l'éditeur, ouvre le projet.

## Étape 4 — Lancer le pont sur ton PC

Ouvre un terminal (CMD ou PowerShell) dans le dossier du dépôt `GAME` et lance :

```bash
python ue5_mcp_bridge.py https://8765-ivut4ppbx3h3xx57402hb.e2b.app fv-7c3d9e2a1b http://localhost:3000/mcp <TON_CAPABILITY_TOKEN>
```

(`<TON_CAPABILITY_TOKEN>` = le token copié à l'étape 3 — remplace-le, crochets compris. Si tu désactives « Require Capability Token », omets le 4e argument.)

Vérification : le pont affiche `connecte au relai ...` puis `-> POST ... tools/list` quand je lui envoie une requête. Laisse cette fenêtre **ouverte** pendant toute la session.

> Si l'URL publique change (relai redémarré), je te donnerai la nouvelle URL — même commande, nouvelle adresse.

## Étape 5 — C'est parti

Dis-moi **« prêt »** quand le pont tourne. Je conduis alors ton éditeur :

1. **Niveau « Ferme »** + soleil (DirectionalLight) + SkyAtmosphere + brouillard + PostProcessVolume
2. **Terrain** (outil paysage ou PCG) + herbe
3. **Props procéduraux** (PCG : arbres, rochers, fleurs, clôtures)
4. **Blueprints** : `BP_Player`, `BP_Crop`, `BP_Animal`, `BP_HUD`, `BP_GameMode` (cycle jour/nuit)
5. **Matériaux** : herbe, bois, eau
6. **Tests en jeu (PIE)** + captures d'écran du viewport à chaque étape — je les lis pour vérifier le rendu

## Ce que je peux faire via MCP (exemples d'outils)

| Outil | Ce que ça fait |
|---|---|
| `control_editor` | lancer/arrêter le PIE, prendre des **captures d'écran**, déplacer la caméra du viewport |
| `system_control` | exécuter du **Python dans l'éditeur** (créer/déplacer/supprimer n'importe quel asset) |
| `manage_asset` | créer importer des assets (meshes, matériaux, blueprints) |
| `manage_blueprint` | créer/modifier des Blueprints (nœuds, variables, fonctions) |
| `control_actor` | placer/déplacer/faire tourner des acteurs dans le niveau |
| `manage_level` | créer des niveaux, définir le monde par défaut |
| `build_environment` | **terrains, ciel, eau, lumières, feuillage** procéduraux |

## Avertissements (honnêteté totale)

- **Latence** : ~0,5–1 s par action (pont en long-poll). Acceptable pour du dev, pas pour du jeu en temps réel.
- **Disponibilité** : ton PC + l'éditeur + le pont doivent rester **allumés** pendant la session.
- **Le projet vit sur TON PC** : les `.uasset` sont binaires — tu commites le projet toi-même (`git add FarmVale` dans ton dépôt).
- **Vérification** : capture d'écran après chaque étape, je lis l'image et je corrige si besoin.
- **Sécurité** : le pont n'ouvre **aucun** port entrant chez toi ; seules des connexions sortantes HTTPS vers le relai.
- **Plan B** : le projet Godot 4 (`farmvale3d/`, testé 60/60, F5 en 2 clics) reste dispo si l'aventure UE5 coince.

## Fichiers

- `ue5_mcp_bridge.py` — le pont (à lancer sur ton PC, Python 3, bibliothèque standard uniquement)
- Côté Arena : `ue5_mcp_relay.py` (le relai, déjà en ligne) — ne pas lancer toi-même.
