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
                                            │  http://localhost:8000/mcp│
                                            │  (plugin officiel Epic     │
                                            │   « Unreal MCP », TON éditeur)│
                                            └──────────────────────────┘
```

- **Le relai** tourne chez moi (Arena) sur une URL publique. **Tes connexions sortantes uniquement** — aucun port ouvert chez toi.
- **Le pont** (`ue5_mcp_bridge.py`) tourne sur ton PC : il interroge le relai toutes les 20 s, exécute chaque requête sur le serveur MCP de ton éditeur, renvoie le résultat.
- Le relai a été **testé de bout en bout** (poignée de main MCP `initialize`, `tools/list`, `tools/call` avec session maintenue — `session_ok=True`).

## ✅ Option A (RECOMMANDÉE) — Plugin officiel Epic « Unreal MCP » (UE 5.8+)

Tu as déjà ce plugin dans ton éditeur — **rien à installer**.

### Étape 1 — Créer le projet UE5

Epic Launcher → Unreal Engine → **Launch** → onglet **Games** → **Create Project**
Nom : `FarmVale` — **Blueprint** (pas C++), **With Starter Content** décoché, qualité **Maximum**.
Ferme l'éditeur.

### Étape 2 — Activer les plugins officiels (2 clics)

Ouvre le projet, puis **Edit → Plugins**, recherche « MCP » :

1. ✅ **Unreal MCP** (Epic Games, Experimental) → **Enabled** → **Restart Now**
   (le plugin dépendant « Toolset Registry » s'active automatiquement)
2. ✅ **AllToolsets** → **Enabled** → **Restart Now**
   ⚠️ **Important** : c'est « AllToolsets » qui fournit les outils MCP. Sans lui, le serveur répond mais n'expose aucun outil.

> « MCP Client Toolset » (l'autre plugin de ta capture) : **pas besoin** — c'est un client MCP, pas le serveur.

### Étape 3 — Démarrer le serveur MCP

**Edit → Editor Preferences → General → Model Context Protocol** :

- ✅ **Auto Start Server** (le serveur démarre à chaque lancement de l'éditeur)
- **Server Port Number** : `8000` (défaut — le noter si tu le changes)
- **Server URL Path** : `/mcp` (défaut)

Le serveur écoute sur **http://127.0.0.1:8000/mcp** (transport Streamable HTTP, pas de token d'authentification, loopback uniquement).

> Alternative sans auto-start : laisse décoché et tape `ModelContextProtocol.StartServer` dans la console de l'éditeur (touche `²` / backtick).

### Étape 4 — Lancer le pont sur ton PC

Terminal (CMD/PowerShell) à la racine du dépôt `GAME` :

```bash
python ue5_mcp_bridge.py https://8765-ivut4ppbx3h3xx57402hb.e2b.app fv-7c3d9e2a1b http://localhost:8000/mcp
```

**Pas de 4e argument** — le serveur officiel Epic n'a pas de token (loopback only).

Vérification : le pont affiche `connecte au relai ...` puis `-> POST ... tools/list` quand je lui envoie une requête. Laisse cette fenêtre **ouverte** pendant toute la session.

> Si le port 8000 est déjà pris chez toi, change « Server Port Number » à l'étape 3 et adapte l'URL du pont en conséquence.

## 🔧 Option B (secours) — Plugin tiers ChiR24/Unreal_mcp (UE < 5.8 sans le plugin officiel)

Si ton éditeur n'a PAS le plugin « Unreal MCP » :

```bash
git clone https://github.com/ChiR24/Unreal_mcp.git Plugins/McpAutomationBridge
```

Puis : Edit → Plugins → « MCP Automation Bridge » → Enabled → redémarrer → Project Settings → Plugins → MCP Automation Bridge → ✅ « Enable Native MCP Server » (port 3000) → redémarrer → copier le **Capability Token** → pont avec `http://localhost:3000/mcp` + le token en 4e argument.

## Étape 5 — C'est parti

Dis-moi **« prêt »** quand le pont tourne. Je conduis alors ton éditeur :

1. **Niveau « Ferme »** + soleil (DirectionalLight) + SkyAtmosphere + brouillard + PostProcessVolume
2. **Terrain** + herbe
3. **Props procéduraux** (arbres, rochers, fleurs, clôtures)
4. **Blueprints** : `BP_Player`, `BP_Crop`, `BP_Animal`, `BP_HUD`, `BP_GameMode` (cycle jour/nuit)
5. **Matériaux** : herbe, bois, eau
6. **Tests en jeu (PIE)** + captures d'écran du viewport à chaque étape — je les lis pour vérifier le rendu

> Note : avec « Enable Tool Search » activé (défaut), `tools/list` renvoie 3 méta-outils (`list_toolsets`, `describe_toolset`, `call_tool`) — c'est normal, je les utilise pour découvrir puis appeler les vrais outils.

## Avertissements (honnêteté totale)

- **Latence** : ~0,5–1 s par action (pont en long-poll). Acceptable pour du dev, pas pour du jeu en temps réel.
- **Disponibilité** : ton PC + l'éditeur + le pont doivent rester **allumés** pendant la session.
- **Redémarrage de l'éditeur** : le serveur MCP change de session à chaque démarrage — préviens-moi, je relance la poignée de main `initialize`.
- **Le projet vit sur TON PC** : les `.uasset` sont binaires — tu commites le projet toi-même (`git add FarmVale` dans ton dépôt).
- **Vérification** : capture d'écran après chaque étape, je lis l'image et je corrige si besoin.
- **Sécurité** : le pont n'ouvre **aucun** port entrant chez toi ; seules des connexions sortantes HTTPS vers le relai. Le serveur MCP officiel n'écoute que sur `127.0.0.1` (loopback) et n'a pas d'authentification — ne le forward jamais.
- **Plan B** : le projet Godot 4 (`farmvale3d/`, testé 60/60, F5 en 2 clics) reste dispo si l'aventure UE5 coince.

## Fichiers

- `ue5_mcp_bridge.py` — le pont (à lancer sur ton PC, Python 3, bibliothèque standard uniquement)
- Côté Arena : `ue5_mcp_relay.py` (le relai, déjà en ligne) — ne pas lancer toi-même.
