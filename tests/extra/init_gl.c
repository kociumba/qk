// init_gl.c
// Minimal Win32 + classic OpenGL context example
// Build with MSVC: cl /nologo /W3 init_gl.c user32.lib gdi32.lib opengl32.lib
// Build with MinGW: gcc -O2 -Wall init_gl.c -lgdi32 -lopengl32 -luser32 -o init_gl.exe

#include <windows.h>

#include <GL/gl.h>
#include <stdio.h>

// Simple WindowProc
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    if (msg == WM_DESTROY) {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, w, l);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrev, LPSTR lpCmdLine, int nShowCmd) {
    const char* cls = "MinimalGLClass";
    WNDCLASS wc = {0};
    wc.style = CS_OWNDC;  // important: gives a unique DC
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = cls;
    RegisterClass(&wc);

    HWND hwnd = CreateWindowExA(
        0, cls, "Minimal OpenGL init", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT,
        CW_USEDEFAULT, 640, 480, NULL, NULL, hInstance, NULL
    );
    if (!hwnd) return 1;

    HDC hdc = GetDC(hwnd);

    PIXELFORMATDESCRIPTOR pfd = {0};
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 24;
    pfd.cDepthBits = 16;
    pfd.iLayerType = PFD_MAIN_PLANE;

    int pf = ChoosePixelFormat(hdc, &pfd);
    if (!pf) {
        MessageBoxA(NULL, "ChoosePixelFormat failed", "Error", MB_ICONERROR);
        return 1;
    }
    if (!SetPixelFormat(hdc, pf, &pfd)) {
        MessageBoxA(NULL, "SetPixelFormat failed", "Error", MB_ICONERROR);
        return 1;
    }

    // Create classical OpenGL context
    HGLRC hglrc = wglCreateContext(hdc);
    if (!hglrc) {
        MessageBoxA(NULL, "wglCreateContext failed", "Error", MB_ICONERROR);
        return 1;
    }
    if (!wglMakeCurrent(hdc, hglrc)) {
        MessageBoxA(NULL, "wglMakeCurrent failed", "Error", MB_ICONERROR);
        wglDeleteContext(hglrc);
        return 1;
    }

    // Query GL version (requires a current context)
    const GLubyte* ver = glGetString(GL_VERSION);
    char msg[128];
    if (ver) {
        sprintf_s(msg, sizeof(msg), "OpenGL version: %s", (const char*)ver);
    } else {
        sprintf_s(msg, sizeof(msg), "glGetString returned NULL");
    }
    MessageBoxA(NULL, msg, "Init OK", MB_OK);

    // Clean up
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(hglrc);
    ReleaseDC(hwnd, hdc);
    DestroyWindow(hwnd);
    UnregisterClassA(cls, hInstance);

    return 0;
}
