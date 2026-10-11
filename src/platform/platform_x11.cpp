// ============================================================
// FarmVale — plateforme Linux/X11 (natif, rendu logiciel)
// Fenêtrage X11 + XPutImage (mise à l'échelle x2 par le CPU).
// Aucune bibliothèque externe requise à l'exécution (libX11 standard).
// ============================================================
#include "platform.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <chrono>
#include <thread>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
#include <vector>
#include <filesystem>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

namespace fe {

class X11Platform : public IPlatform {
public:
    Display* dpy = nullptr;
    Window win = 0;
    GC gc = nullptr;
    XImage* img = nullptr;
    std::vector<uint32_t> winBuf;
    int winW = 0, winH = 0;
    Atom wmDelete = 0;
    bool quit = false;
    bool created = false;
    bool fullscreen = false;
    std::chrono::steady_clock::time_point t0 = std::chrono::steady_clock::now();

    ~X11Platform() override {
        if (img) XDestroyImage(img);
        if (gc && dpy) XFreeGC(dpy, gc);
        if (dpy && win) XDestroyWindow(dpy, win);
        if (dpy) XCloseDisplay(dpy);
    }

    bool init() {
        dpy = XOpenDisplay(nullptr);
        if (!dpy) {
            fprintf(stderr, "[X11] Impossible d'ouvrir l'affichage. DISPLAY=%s\n",
                    getenv("DISPLAY") ? getenv("DISPLAY") : "(non défini)");
            fprintf(stderr, "[X11] Lancez le jeu depuis un bureau graphique, ou en headless: make headless\n");
            return false;
        }
        signal(SIGCHLD, SIG_IGN); // pas de zombies avec les lecteurs audio
        // Dossier de sauvegarde
        const char* home = getenv("HOME");
        if (home) {
            saveDir = std::string(home) + "/.local/share/FarmVale";
            std::error_code ec;
            std::filesystem::create_directories(saveDir, ec);
        } else {
            saveDir = ".";
        }
        return true;
    }

    void createWindow(int w, int h) {
        fbW = w / windowScale;
        fbH = h / windowScale;
        int screen = DefaultScreen(dpy);
        Window root = RootWindow(dpy, screen);
        winW = w; winH = h;
        win = XCreateSimpleWindow(dpy, root, 0, 0, w, h, 0,
                                  BlackPixel(dpy, screen), BlackPixel(dpy, screen));
        XStoreName(dpy, win, "FarmVale — La Ferme des Quatre Saisons");
        XSelectInput(dpy, win, KeyPressMask | KeyReleaseMask | ButtonPressMask |
                               ButtonReleaseMask | PointerMotionMask | StructureNotifyMask);
        wmDelete = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
        XSetWMProtocols(dpy, win, &wmDelete, 1);
        gc = XCreateGC(dpy, win, 0, nullptr);
        winBuf.assign((size_t)w * h, 0);
        img = XCreateImage(dpy, DefaultVisual(dpy, screen), DefaultDepth(dpy, screen),
                           ZPixmap, 0, (char*)winBuf.data(), w, h, 32, 0);
        XMapWindow(dpy, win);
        XFlush(dpy);
        created = true;
    }

    static int mapKey(KeySym ks) {
        switch (ks) {
            case XK_Left: return K_LEFT;
            case XK_Right: return K_RIGHT;
            case XK_Up: return K_UP;
            case XK_Down: return K_DOWN;
            case XK_w: case XK_W: return K_W;
            case XK_a: case XK_A: return K_A;
            case XK_s: case XK_S: return K_S;
            case XK_d: case XK_D: return K_D;
            case XK_space: return K_SPACE;
            case XK_Return: return K_ENTER;
            case XK_Escape: return K_ESC;
            case XK_Tab: return K_TAB;
            case XK_b: case XK_B: return K_B;
            case XK_e: case XK_E: return K_E;
            case XK_m: case XK_M: return K_M;
            case XK_f: case XK_F: return K_F;
            case XK_p: case XK_P: return K_P;
            case XK_o: case XK_O: return K_O;
            case XK_1: return K_1; case XK_2: return K_2; case XK_3: return K_3;
            case XK_4: return K_4; case XK_5: return K_5; case XK_6: return K_6;
            case XK_7: return K_7; case XK_8: return K_8; case XK_9: return K_9;
            case XK_0: return K_0;
            case XK_minus: return K_MINUS;
            case XK_equal: return K_EQUALS;
            default: return K_NONE;
        }
    }

    void updateMouse(int x, int y) {
        if (winW > 0 && fbW > 0) {
            input.mouseX = x * fbW / winW;
            input.mouseY = y * fbH / winH;
        }
    }

    bool poll() override {
        if (!dpy) return false;
        while (XPending(dpy)) {
            XEvent ev;
            XNextEvent(dpy, &ev);
            switch (ev.type) {
                case KeyPress:
                case KeyRelease: {
                    // Filtre l'auto-répétition clavier
                    if (ev.type == KeyPress && XPending(dpy)) {
                        XEvent next;
                        XPeekEvent(dpy, &next);
                        if (next.type == KeyRelease && next.xkey.keycode == ev.xkey.keycode &&
                            next.xkey.time == ev.xkey.time) {
                            XNextEvent(dpy, &next);
                            break;
                        }
                    }
                    KeySym ks = XLookupKeysym(&ev.xkey, 0);
                    int k = mapKey(ks);
                    if (k != K_NONE) {
                        if (ev.type == KeyPress) {
                            if (!input.down[k]) input.pressed[k] = true;
                            input.down[k] = true;
                        } else {
                            input.down[k] = false;
                        }
                    }
                    break;
                }
                case ButtonPress:
                    if (ev.xbutton.button == 1) { input.mousePressed = true; input.mouseDown = true; }
                    updateMouse(ev.xbutton.x, ev.xbutton.y);
                    break;
                case ButtonRelease:
                    if (ev.xbutton.button == 1) { input.mouseReleased = true; input.mouseDown = false; }
                    updateMouse(ev.xbutton.x, ev.xbutton.y);
                    break;
                case MotionNotify:
                    updateMouse(ev.xmotion.x, ev.xmotion.y);
                    break;
                case ClientMessage:
                    if ((Atom)ev.xclient.data.l[0] == wmDelete) quit = true;
                    break;
                default:
                    break;
            }
        }
        return !quit;
    }

    void present(const Framebuffer& fb) override {
        if (!dpy) return;
        if (!created) createWindow(fb.w * windowScale, fb.h * windowScale);
        // Mise à l'échelle au plus proche (x2) dans le buffer fenêtre
        for (int y = 0; y < fb.h; y++) {
            const uint32_t* src = &fb.px[(size_t)y * fb.w];
            for (int x = 0; x < fb.w; x++) {
                uint32_t c = src[x];
                for (int sy = 0; sy < windowScale; sy++) {
                    uint32_t* dst = &winBuf[(size_t)(y * windowScale + sy) * winW + (size_t)x * windowScale];
                    for (int sx = 0; sx < windowScale; sx++) dst[sx] = c;
                }
            }
        }
        XPutImage(dpy, win, gc, img, 0, 0, 0, 0, winW, winH);
        XFlush(dpy);
    }

    void setTitle(const std::string& t) override {
        if (dpy && win) XStoreName(dpy, win, t.c_str());
    }

    void beep(int hz, int ms) override {
        (void)hz; (void)ms;
        if (dpy) { XBell(dpy, 100); XFlush(dpy); }
    }

    void playPcm(const int16_t* data, int samples, int rate) override {
        if (!dpy || samples <= 0) return;
        // Construit un WAV en mémoire
        int dataSize = samples * 2;
        int wavSize = 44 + dataSize;
        std::vector<uint8_t> wav(wavSize);
        auto w32 = [&](int off, uint32_t v) {
            wav[off] = v & 255; wav[off + 1] = (v >> 8) & 255; wav[off + 2] = (v >> 16) & 255; wav[off + 3] = (v >> 24) & 255;
        };
        auto w16 = [&](int off, uint16_t v) { wav[off] = v & 255; wav[off + 1] = (v >> 8) & 255; };
        memcpy(wav.data(), "RIFF", 4); w32(4, wavSize - 8); memcpy(wav.data() + 8, "WAVE", 4);
        memcpy(wav.data() + 12, "fmt ", 4); w32(16, 16); w16(20, 1); w16(22, 1);
        w32(24, rate); w32(28, rate * 2); w16(32, 2); w16(34, 16);
        memcpy(wav.data() + 36, "data", 4); w32(40, dataSize);
        memcpy(wav.data() + 44, data, dataSize);

        char path[] = "/tmp/farmvale_sfx_XXXXXX.wav";
        int fd = mkstemps(path, 4);
        if (fd < 0) return;
        ssize_t wr = write(fd, wav.data(), wav.size());
        (void)wr;
        close(fd);

        pid_t pid = fork();
        if (pid == 0) {
            // Enfant : ouvre le fichier, supprime son nom, puis joue via /proc/self/fd
            int ffd = open(path, O_RDONLY);
            if (ffd >= 0) {
                unlink(path);
                char fdpath[64];
                snprintf(fdpath, sizeof(fdpath), "/proc/self/fd/%d", ffd);
                execlp("aplay", "aplay", "-q", fdpath, (char*)nullptr);
                execlp("paplay", "paplay", fdpath, (char*)nullptr);
                execlp("ffplay", "ffplay", "-nodisp", "-autoexit", "-loglevel", "quiet", fdpath, (char*)nullptr);
            }
            _exit(127);
        }
        // Le parent continue (SIGCHLD ignoré)
    }

    double timeMs() override {
        return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    }

    void sleepMs(int ms) override {
        if (ms > 0) std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }

    void setFullscreen(bool on) override {
        if (!dpy || !win) return;
        fullscreen = on;
        Atom netState = XInternAtom(dpy, "_NET_WM_STATE", False);
        Atom fs = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);
        XEvent e;
        memset(&e, 0, sizeof(e));
        e.xclient.type = ClientMessage;
        e.xclient.window = win;
        e.xclient.message_type = netState;
        e.xclient.format = 32;
        e.xclient.data.l[0] = on ? 1 : 0;
        e.xclient.data.l[1] = fs;
        e.xclient.data.l[2] = 0;
        e.xclient.data.l[3] = 1;
        XSendEvent(dpy, DefaultRootWindow(dpy), False, SubstructureRedirectMask | SubstructureNotifyMask, &e);
        XFlush(dpy);
    }

    bool isFullscreen() const override { return fullscreen; }

    void requestQuit() override { quit = true; }

    int fbW = 0, fbH = 0; // dimensions du framebuffer (mémorisées au premier present)
};

} // namespace fe

fe::IPlatform* createPlatform(int argc, char** argv) {
    (void)argc; (void)argv;
    auto* p = new fe::X11Platform();
    p->windowScale = 2;
    if (!p->init()) {
        delete p;
        return nullptr;
    }
    return p;
}

// Point d'entrée Linux
extern int farmvale_main(int argc, char** argv);
int main(int argc, char** argv) {
    return farmvale_main(argc, argv);
}
