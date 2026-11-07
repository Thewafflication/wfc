/* Grabs the window whose title starts with argv[1] to a BMP (argv[2]) by
   BitBlt from the window DC; used to see what Wine really draws. */
#include <windows.h>
#include <stdio.h>
#include <string.h>
static const char* want; static HWND found;
static BOOL CALLBACK en(HWND h, LPARAM l) {
    char t[256]; (void)l;
    if (IsWindowVisible(h) && GetWindowTextA(h, t, 256) > 0 &&
        strncmp(t, want, strlen(want)) == 0) { found = h; return FALSE; }
    return TRUE;
}
int main(int argc, char** argv) {
    RECT r; int w, h; HDC s, m; HBITMAP b; void* px; BITMAPINFO bi;
    BITMAPFILEHEADER fh; FILE* f;
    if (argc < 3) return 2;
    want = argv[1];
    for (int i = 0; i < 60 && !found; ++i) { EnumWindows(en, 0); if (!found) Sleep(250); }
    if (!found) { fprintf(stderr, "no window\n"); return 1; }
    Sleep(1500);
    GetWindowRect(found, &r); w = r.right - r.left; h = r.bottom - r.top;
    s = GetWindowDC(found); m = CreateCompatibleDC(s);
    memset(&bi, 0, sizeof bi); bi.bmiHeader.biSize = sizeof bi.bmiHeader;
    bi.bmiHeader.biWidth = w; bi.bmiHeader.biHeight = -h;
    bi.bmiHeader.biPlanes = 1; bi.bmiHeader.biBitCount = 32;
    b = CreateDIBSection(s, &bi, DIB_RGB_COLORS, &px, 0, 0);
    SelectObject(m, b);
    BitBlt(m, 0, 0, w, h, s, 0, 0, SRCCOPY | 0x40000000);
    f = fopen(argv[2], "wb");
    memset(&fh, 0, sizeof fh); fh.bfType = 0x4D42;
    fh.bfOffBits = sizeof fh + sizeof bi.bmiHeader;
    fh.bfSize = fh.bfOffBits + w * h * 4;
    bi.bmiHeader.biHeight = -h;
    fwrite(&fh, sizeof fh, 1, f); fwrite(&bi.bmiHeader, sizeof bi.bmiHeader, 1, f);
    fwrite(px, 4, (size_t)w * h, f); fclose(f);
    printf("%dx%d\n", w, h);
    return 0;
}
