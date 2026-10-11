#!/usr/bin/env python3
# ============================================================
# FarmVale — Pont MCP UE5 (à lancer sur TON PC)
# ------------------------------------------------------------
# Petit script en Python 3 standard (AUCUNE dépendance, aucun pip)
# qui fait le lien entre :
#   - le relai de l'agent (URL publique e2b, ex: https://8765-xxx.e2b.app)
#   - le serveur MCP de TON éditeur UE5
#     (plugin officiel Epic « Unreal MCP » : http://localhost:8000/mcp)
#
# Le pont ne fait que des connexions SORTANTES (vers le relai) :
# ton éditeur reste sur localhost, personne ne peut s'y connecter
# directement. Le pont tourne en boucle jusqu'à CTRL+C.
#
# Utilisation :
#   python ue5_mcp_bridge.py <URL_RELAI> <TOKEN> <URL_MCP> [TOKEN_MCP] [TRAFFIC_TOKEN]
#
# Exemple (serveur MCP officiel Epic, port 8000) :
#   python ue5_mcp_bridge.py https://8765-xxxx.e2b.app fv-7c3d9e2a1b http://localhost:8000/mcp
#
# Le 4e argument (optionnel) est un token « capability » du plugin UE5
# s'il est exigé (en-tête Authorization: Bearer <token>). Le serveur
# officiel Epic n'en a pas besoin.
#
# Le 5e argument (optionnel) est le « traffic access token » e2b : le proxy
# public du bac à sable exige l'en-tête « e2b-traffic-access-token » sur
# TOUTES les requêtes. Il peut aussi être fourni via la variable
# d'environnement E2B_TRAFFIC_TOKEN (pratique sous Windows) :
#   $env:E2B_TRAFFIC_TOKEN = "le-token"
#   python ue5_mcp_bridge.py https://8765-xxxx.e2b.app fv-7c3d9e2a1b http://localhost:8000/mcp
# ============================================================
import json
import os
import sys
import time
import urllib.request
import urllib.error

if len(sys.argv) < 4:
    print("usage: python ue5_mcp_bridge.py <URL_RELAI> <TOKEN> <URL_MCP> [TOKEN_MCP] [TRAFFIC_TOKEN]")
    sys.exit(1)

RELAY = sys.argv[1].rstrip("/")
TOKEN = sys.argv[2]
MCP = sys.argv[3].rstrip("/")
MCP_TOKEN = sys.argv[4] if len(sys.argv) > 4 else None
TRAFFIC_TOKEN = (sys.argv[5] if len(sys.argv) > 5
                 else os.environ.get("E2B_TRAFFIC_TOKEN"))

session = None  # Mcp-Session-Id renvoyé par le serveur MCP


def log(msg):
    print(f"[{time.strftime('%H:%M:%S')}] {msg}", flush=True)


def http(method, url, raw=None, headers=None, timeout=120):
    h = {"Content-Type": "application/json"}
    if headers:
        h.update(headers)
    req = urllib.request.Request(url, data=raw, headers=h, method=method)
    try:
        with urllib.request.urlopen(req, timeout=timeout) as r:
            return r.status, r.headers, r.read().decode("utf-8", "replace")
    except urllib.error.HTTPError as e:
        return e.code, e.headers, e.read().decode("utf-8", "replace")


def send_result(rid, status, body, headers=None):
    """Renvoie le résultat d'un job au relai (ne doit jamais échouer
    silencieusement : sinon l'agent attend 120 s puis reçoit un 504)."""
    payload = json.dumps({
        "id": rid, "status": status, "body": body,
        "headers": headers or {},
    }).encode()
    try:
        http("POST", f"{RELAY}/result?token={TOKEN}", raw=payload,
             timeout=30, headers=relay_headers())
    except Exception as e:
        log(f"!! impossible de renvoyer le résultat #{rid} au relai : {e}")


def relay_headers():
    """En-têtes pour les requêtes VERS LE RELAI (proxy e2b public)."""
    h = {}
    if TRAFFIC_TOKEN:
        h["e2b-traffic-access-token"] = TRAFFIC_TOKEN
    return h


log(f"pont UE5 MCP : relai={RELAY}  éditeur={MCP}")
if TRAFFIC_TOKEN:
    log("traffic access token e2b : présent (le proxy public acceptera les requêtes)")
else:
    log("ATTENTION : pas de traffic access token e2b -> le proxy public "
        "répondra 403 (fourni-le en 5e argument ou via E2B_TRAFFIC_TOKEN)")
log("CTRL+C pour arrêter. Laisse cette fenêtre ouverte pendant toute la session.")
log("En attente de requêtes de l'agent... (tes actions dans l'éditeur apparaîtront ici)")

while True:
    try:
        st, _, body = http("GET", f"{RELAY}/poll?token={TOKEN}",
                           timeout=30, headers=relay_headers())
        if st != 200 or not body:
            time.sleep(0.2 if st == 204 else 1.0)
            continue
        work = json.loads(body)
        rid = work["id"]
        try:
            req = json.loads(work["body"])
            label = req.get("method", "?")
        except Exception:
            label = "?"
        log(f"job #{rid} reçu : {label} -> POST {MCP}")

        headers = {"Accept": "application/json, text/event-stream"}
        if session:
            headers["Mcp-Session-Id"] = session
        if MCP_TOKEN:
            headers["Authorization"] = f"Bearer {MCP_TOKEN}"
        try:
            st, hdrs, raw = http("POST", MCP, raw=work["body"].encode(), headers=headers)
        except Exception as e:
            # Éditeur injoignable (serveur MCP arrêté, mauvais port...) :
            # on renvoie l'erreur à l'agent tout de suite.
            log(f"!! éditeur injoignable ({MCP}) : {e}")
            err = json.dumps({"jsonrpc": "2.0", "id": None, "error": {
                "code": -32000,
                "message": f"Éditeur UE5 injoignable ({MCP}) : {e}. "
                           f"Vérifie que l'éditeur est ouvert et que le serveur MCP "
                           f"tourne (Edit > Editor Preferences > General > Model Context "
                           f"Protocol > Auto Start Server, ou console : ModelContextProtocol.StartServer)"}})
            send_result(rid, 0, err)
            continue

        sid = hdrs.get("Mcp-Session-Id")
        if sid:
            session = sid
        # Si le serveur répond en SSE (text/event-stream), on extrait le
        # dernier événement « data: » qui contient le résultat JSON-RPC.
        result_body = raw
        ctype = hdrs.get("Content-Type") or ""
        if "text/event-stream" in ctype:
            datas = [ln[5:].strip() for ln in raw.splitlines() if ln.startswith("data:")]
            if datas:
                result_body = datas[-1]
        log(f"job #{rid} terminé : HTTP {st} ({len(result_body)} octets)")
        send_result(rid, st, result_body,
                    {"content-type": ctype, "mcp-session-id": session or ""})
    except Exception as e:
        log(f"erreur pont : {e}")
        time.sleep(1.0)
