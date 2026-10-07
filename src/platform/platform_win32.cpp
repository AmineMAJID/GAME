// ============================================================
// FarmVale — plateforme Windows (Win32 + GDI, natif)
// C'est CE backend qui produit l'exécutable vendu sur Steam.
// Aucune dépendance externe : uniquement l'API Windows (gdi32, winmm, user32).
// ============================================================
#include "platform.h"
#include <windows.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
#include <vector>
#include <deque>

namespace fe {

class Win32Platform : public IPlatform {
public:
    HWND hwnd = nullptr;
    HDC memdc = nullptr;
    HBITMAP dib = nullptr;
    uint32_t* dibBits = nullptr;
    int dibW = 0, dibH = 0;
    int fbW = 0, fbH = 0;
    bool quit = false;
    bool fullscreen = false;
    DWORD winStyle = 0;
    RECT winRect{0, 0, 0, 0};
    std::deque<std::pair<uint64_t, std::vector<uint8_t>>> playing; // WAV en cours de lecture

    ~Win32Platform() override {
        if (dib) DeleteObject(dib);
        if (hwnd) DestroyWindow(hwnd);
    }

    static int mapKey(WPARAM vk) {
        switch (vk) {
            case VK_LEFT: return K_LEFT;
            case VK_RIGHT: return K_RIGHT;
            case VK_UP: return K_UP;
            case VK_DOWN: return K_DOWN;
            case 'W': return K_W;
            case 'A': return K_A;
            case 'S': return K_S;
            case 'D': return K_D;
            case VK_SPACE: return K_SPACE;
            case VK_RETURN: return K_ENTER;
            case VK_ESCAPE: return K_ESC;
            case VK_TAB: return K_TAB;
            case 'B': return K_B;
            case 'E': return K_E;
            case 'M': return K_M;
            case 'F': return K_F;
            case 'P': return K_P;
            case 'O': return K_O;
            case '1': return K_1; case '2': return K_2; case '3': return K_3;
            case '4': return K_4; case '5': return K_5; case '6': return K_6;
            case '7': return K_7; case '8': return K_8; case '9': return K_9;
            case '0': return K_0;
            case VK_OEM_MINUS: return K_MINUS;
            case VK_OEM_PLUS: return K_EQUALS;
            case VK_ADD: return K_EQUALS;
            default: return K_NONE;
        }
    }

    void ensureWindow(int w, int h) {
        if (hwnd) return;
        fbW = w; fbH = h;
        HINSTANCE hInst = GetModuleHandle(nullptr);

        WNDCLASSEXA wc{};
        wc.cbSize = sizeof(wc);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = &Win32Platform::wndProc;
        wc.hInstance = hInst;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
        wc.lpszClassName = "FarmValeWindowClass";
        RegisterClassExA(&wc);

        // Taille de fenêtre = framebuffer x2 (ou ajusté à l'écran si trop grand)
        int scale = 2;
        int sw = GetSystemMetrics(SM_CXSCREEN), sh = GetSystemMetrics(SM_CYSCREEN);
        while (scale > 1 && (w * scale > sw - 80 || h * scale > sh - 80)) scale--;
        windowScale = scale;
        int ww = w * scale, wh = h * scale;

        RECT rc{0, 0, ww, wh};
        AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_MAXIMIZEBOX), FALSE);
        hwnd = CreateWindowExA(0, wc.lpszClassName, "FarmVale — La Ferme des Quatre Saisons",
                              WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_MAXIMIZEBOX),
                              (sw - (rc.right - rc.left)) / 2, (sh - (rc.bottom - rc.top)) / 2,
                              rc.right - rc.left, rc.bottom - rc.top,
                              nullptr, nullptr, hInst, this);
        if (!hwnd) return;
        winStyle = WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
        GetWindowRect(hwnd, &winRect);

        // DIB section 32bpp à la taille du framebuffer (le StretchBlt agrandit)
        HDC hdc = GetDC(hwnd);
        BITMAPINFO bi{};
        bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bi.bmiHeader.biWidth = w;
        bi.bmiHeader.biHeight = -h; // top-down
        bi.bmiHeader.biPlanes = 1;
        bi.bmiHeader.biBitCount = 32;
        bi.bmiHeader.biCompression = BI_RGB;
        dib = CreateDIBSection(hdc, &bi, DIB_RGB_COLORS, (void**)&dibBits, nullptr, 0);
        dibW = w; dibH = h;
        memdc = CreateCompatibleDC(hdc);
        SelectObject(memdc, dib);
        ReleaseDC(hwnd, hdc);

        ShowWindow(hwnd, SW_SHOW);
        UpdateWindow(hwnd);

        // Dossier de sauvegarde : %APPDATA%\FarmVale
        const char* appdata = getenv("APPDATA");
        if (appdata) {
            saveDir = std::string(appdata) + "\\FarmVale";
            CreateDirectoryA(saveDir.c_str(), nullptr);
        } else {
            saveDir = ".";
        }
    }

    static LRESULT CALLBACK wndProc(HWND h, UINT msg, WPARAM wp, LPARAM lp) {
        Win32Platform* self = nullptr;
        if (msg == WM_NCCREATE) {
            auto* cs = (CREATESTRUCT*)lp;
            self = (Win32Platform*)cs->lpCreateParams;
            SetWindowLongPtr(h, GWLP_USERDATA, (LONG_PTR)self);
        } else {
            self = (Win32Platform*)GetWindowLongPtr(h, GWLP_USERDATA);
        }
        if (!self) return DefWindowProc(h, msg, wp, lp);
        return self->handleMessage(h, msg, wp, lp);
    }

    LRESULT handleMessage(HWND h, UINT msg, WPARAM wp, LPARAM lp) {
        switch (msg) {
            case WM_KEYDOWN: {
                int k = mapKey(wp);
                if (k != K_NONE) {
                    bool wasDown = (lp & 0x40000000) != 0; // bit 30 : état précédent
                    if (!wasDown) input.pressed[k] = true;
                    input.down[k] = true;
                }
                return 0;
            }
            case WM_KEYUP: {
                int k = mapKey(wp);
                if (k != K_NONE) input.down[k] = false;
                return 0;
            }
            case WM_LBUTTONDOWN:
                input.mousePressed = true;
                input.mouseDown = true;
                input.mouseX = (int)(short)LOWORD(lp);
                input.mouseY = (int)(short)HIWORD(lp);
                return 0;
            case WM_LBUTTONUP:
                input.mouseReleased = true;
                input.mouseDown = false;
                input.mouseX = (int)(short)LOWORD(lp);
                input.mouseY = (int)(short)HIWORD(lp);
                return 0;
            case WM_MOUSEMOVE:
                input.mouseX = (int)(short)LOWORD(lp);
                input.mouseY = (int)(short)HIWORD(lp);
                return 0;
            case WM_ERASEBKGND:
                return 1; // évite le scintillement (on redessine tout à chaque frame)
            case WM_CLOSE:
                quit = true;
                DestroyWindow(h);
                return 0;
            case WM_DESTROY:
                PostQuitMessage(0);
                return 0;
            default:
                return DefWindowProc(h, msg, wp, lp);
        }
    }

    bool poll() override {
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) { quit = true; return false; }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        // Nettoie les WAV terminés (gardés ~3 s en mémoire pour PlaySound)
        uint64_t now = GetTickCount64();
        while (!playing.empty() && now - playing.front().first > 3000)
            playing.pop_front();
        return !quit;
    }

    void present(const Framebuffer& fb) override {
        if (!hwnd) ensureWindow(fb.w, fb.h);
        if (!hwnd) return;
        // Conversion 0xRRGGBB -> 0x00BBGGRR (DIB 32bpp BI_RGB)
        size_t n = (size_t)fb.w * fb.h;
        for (size_t i = 0; i < n; i++) {
            uint32_t c = fb.px[i];
            dibBits[i] = (c & 0xFF00) | ((c & 0xFF) << 16) | ((c >> 16) & 0xFF);
        }
        HDC hdc = GetDC(hwnd);
        RECT rc;
        GetClientRect(hwnd, &rc);
        SetStretchBltMode(hdc, COLORONCOLOR); // pixel-art : pas d'interpolation
        StretchBlt(hdc, 0, 0, rc.right - rc.left, rc.bottom - rc.top,
                   memdc, 0, 0, fb.w, fb.h, SRCCOPY);
        ReleaseDC(hwnd, hdc);
    }

    void setTitle(const std::string& t) override {
        if (hwnd) SetWindowTextA(hwnd, t.c_str());
    }

    void beep(int hz, int ms) override {
        if (hz > 0 && ms > 0) Beep((DWORD)hz, (DWORD)ms);
    }

    void playPcm(const int16_t* data, int samples, int rate) override {
        if (samples <= 0) return;
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
        PlaySoundA((LPCSTR)wav.data(), nullptr, SND_MEMORY | SND_ASYNC | SND_NODEFAULT);
        playing.push_back({GetTickCount64(), std::move(wav)});
    }

    double timeMs() override {
        return (double)GetTickCount64();
    }

    void sleepMs(int ms) override {
        if (ms > 0) Sleep((DWORD)ms);
    }

    void setFullscreen(bool on) override {
        if (!hwnd || on == fullscreen) return;
        fullscreen = on;
        if (on) {
            GetWindowRect(hwnd, &winRect);
            SetWindowLong(hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
            SetWindowPos(hwnd, HWND_TOP, 0, 0,
                         GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
                         SWP_FRAMECHANGED | SWP_SHOWWINDOW);
        } else {
            SetWindowLong(hwnd, GWL_STYLE, winStyle);
            SetWindowPos(hwnd, HWND_NOTOPMOST, winRect.left, winRect.top,
                         winRect.right - winRect.left, winRect.bottom - winRect.top,
                         SWP_FRAMECHANGED | SWP_SHOWWINDOW);
        }
    }

    bool isFullscreen() const override { return fullscreen; }
    void requestQuit() override { quit = true; }
};

} // namespace fe

// Point d'entrée Windows (MSVC / MinGW avec -mwindows)
extern int farmvale_main(int argc, char** argv);
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    return farmvale_main(__argc, __argv);
}

fe::IPlatform* createPlatform(int argc, char** argv) {
    (void)argc; (void)argv;
    return new fe::Win32Platform();
}
