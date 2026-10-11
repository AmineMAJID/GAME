#!/bin/sh
# Télécharge les en-têtes X11 officiels (miroirs GitHub libX11 + xproto) pour
# compiler sans libx11-dev. Sur une machine de dev normale, préfère :
#   sudo apt install libx11-dev
set -e
cd "$(dirname "$0")/.."
mkdir -p third_party/x11
if [ -f third_party/x11/include/X11/Xlib.h ]; then
    echo "En-têtes X11 déjà présents."
    exit 0
fi
echo "Téléchargement des en-têtes X11 (mirror/libX11 + x11proto)..."
curl -sL "https://codeload.github.com/mirror/libX11/tar.gz/refs/heads/master" -o /tmp/libX11.tgz
curl -sL "https://codeload.github.com/freedesktop-unofficial-mirror/xorg__proto__x11proto/tar.gz/refs/heads/master" -o /tmp/xproto.tgz
rm -rf /tmp/libX11-master /tmp/xproto-master
tar -xzf /tmp/libX11.tgz -C /tmp && mv /tmp/libX11-master /tmp/libx11-src
tar -xzf /tmp/xproto.tgz -C /tmp && mv /tmp/xorg__proto__x11proto-master /tmp/xproto-master
mkdir -p third_party/x11/include/X11
cp -r /tmp/libx11-src/include/X11/. third_party/x11/include/X11/
for h in X.h Xatom.h Xdefs.h Xmd.h Xosdefs.h Xproto.h Xprotostr.h Xthreads.h \
         Xwindows.h keysym.h keysymdef.h ap_keysym.h DECkeysym.h HPkeysym.h \
         Sunkeysym.h XF86keysym.h Xalloca.h Xarch.h Xos.h Xos_r.h XWDFile.h \
         Xw32defs.h Xwinsock.h Xfuncs.h; do
    cp /tmp/xproto-master/$h third_party/x11/include/X11/
done
# Xfuncproto.h et XlibConf.h sont des templates : on génère des versions minimales
cp /tmp/xproto-master/Xfuncproto.h.in third_party/x11/include/X11/Xfuncproto.h
cat > third_party/x11/include/X11/XlibConf.h << 'EOH'
#ifndef _XLIBCONF_H_
#define _XLIBCONF_H_
#define XTHREADS 1
#define XUSE_MTSAFE_API 1
#endif
EOH
echo "OK -> third_party/x11/include/X11"
