#!/usr/bin/env python3
# ============================================================
# FarmVale — Pont MCP UE5 (à lancer sur TON PC)
# ------------------------------------------------------------
# Petit script en Python 3 standard (AUCUNE dépendance, aucun pip)
# qui fait le lien entre :
#   - le relai de l'agent (URL publique e2b, ex: https://8765-xxx.e2b.app)
#   - le serveur MCP de TON éditeur UE5 (ex: http://localhost:3000/mcp)
#
# Le pont ne fait que des connexions SORTANTES (vers le relai) :
# ton éditeur reste sur localhost, personne ne peut s'y connecter
# directement. Le pont tourne en boucle jusqu'à CTRL+C.
#
# Utilisation :
#   python ue5_mcp_bridge.py <URL_RELAI> <TOKEN> <URL_MCP> [TOKEN_MCP]
#
# Exemple (serveur MCP UE5 par défaut, port 3000) :
#   python ue5_mcp_bridge.py https://8765-xxxx.e2b.app fv-7c3d9e2a1b http://localhost:3000/mcp
#
# Le 4e argument (optionnel) est le « capability token » du plugin UE5
# s'il est exigé (en-tête Authorization: Bearer <token>).
# ============================================================
import json
import sys
import time
import urllib.request
import urllib.error

if len(sys.argv) < 4:
    print("usage: python ue5_mcp_bridge.py <URL_RELAI> <TOKEN> <URL_MCP> [TOKEN_MCP]")
    sys.exit(1)

RELAY = sys.argv[1].rstrip("/")
TOKEN = sys.argv[2]
MCP = sys.argv[3].rstrip("/")
MCP_TOKEN = sys.argv[4] if len(sys.argv) > 4 else None

session = None  # Mcp-Session-Id renvoyé par le serveur MCP


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


print(f"pont UE5 MCP : relai={RELAY}  éditeur={MCP}")
print("CTRL+C pour arrêter. Laisse cette fenêtre ouverte pendant toute la session.")

while True:
    try:
        st, _, body = http("GET", f"{RELAY}/poll?token={TOKEN}", timeout=30)
        if st != 200 or not body:
            time.sleep(0.2 if st == 204 else 1.0)
            continue
        work = json.loads(body)
        headers = {"Accept": "application/json, text/event-stream"}
        if session:
            headers["Mcp-Session-Id"] = session
        if MCP_TOKEN:
            headers["Authorization"] = f"Bearer {MCP_TOKEN}"
        st, hdrs, raw = http("POST", MCP, raw=work["body"].encode(), headers=headers)
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
        http("POST", f"{RELAY}/result?token={TOKEN}",
             raw=json.dumps({
                 "id": work["id"],
                 "status": st,
                 "body": result_body,
                 "headers": {"content-type": ctype, "mcp-session-id": session or ""},
             }).encode())
    except Exception as e:
        print("erreur pont :", e)
        time.sleep(1.0)
