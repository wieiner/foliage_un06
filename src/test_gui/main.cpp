// foliage_un06 — standalone test GUI.
// Win32 + GDI application for interactive foliage scatter testing.
// No engine dependency; links against shared logic in ../common/.
//
// Build: cl ... /Fe:foliage_test_gui.exe /subsystem:windows

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include "../common/foliage_type.h"
#include "../common/foliage_scatter.h"
#include "../common/foliage_paint.h"
#include "../common/foliage_cluster.h"
#include "../common/foliage_prng.h"
#include "../common/foliage_terrain.h"

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <cstdio>
#include <cmath>
#include <string>
#include <vector>
#include <cstdlib>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// -----------------------------------------------------------------------
// Application state
// -----------------------------------------------------------------------
struct AppState {
    // Foliage parameters
    double   areaWidth   = 200.0;
    double   areaDepth   = 200.0;
    double   density     = 0.04;
    double   scaleMin    = 0.6;
    double   scaleMax    = 1.4;
    double   slopeMax    = 35.0;
    double   heightMin   = -100.0;
    double   heightMax   = 100.0;
    int      seed        = 1337;
    bool     alignNormal = true;
    bool     randomRot   = true;
    bool     uniformScale = true;
    int      meshType    = 0; // 0=cross_billboard, 1=cube
    int      terrainMode = 0; // 0=synthetic hills, 1=flat

    // Scatter results
    std::vector<foliage::FoliageInstance> instances;
    std::uint32_t candidates = 0;
    std::uint32_t kept       = 0;
    double       avgScale    = 1.0;
    int          clusterCount = 0;
    bool         hasResult   = false;

    // Brush
    double brushCenterX = 100.0;
    double brushCenterZ = 100.0;
    double brushRadius  = 25.0;
    double brushDensityMul = 1.5;
    bool   eraseMode    = false;

    // View
    double viewOffsetX  = 0.0;
    double viewOffsetZ  = 0.0;
    double viewZoom     = 1.0;
    int    hoveredIdx   = -1;

    // Window
    HWND hwnd       = nullptr;
    HWND hwndScatterBtn = nullptr;
    HWND hwndPaintBtn   = nullptr;
    HWND hwndEraseBtn   = nullptr;
    HWND hwndClearBtn   = nullptr;
    HWND hwndExportBtn  = nullptr;
    HWND hwndStatusBar  = nullptr;
    HWND hwndDensityEdit    = nullptr;
    HWND hwndScaleMinEdit   = nullptr;
    HWND hwndScaleMaxEdit   = nullptr;
    HWND hwndSlopeEdit      = nullptr;
    HWND hwndHeightMinEdit  = nullptr;
    HWND hwndHeightMaxEdit  = nullptr;
    HWND hwndSeedEdit       = nullptr;
    HWND hwndAreaWEdit      = nullptr;
    HWND hwndAreaDEdit      = nullptr;
    HWND hwndBrushSizeEdit  = nullptr;
    HWND hwndAlignCb        = nullptr;
    HWND hwndRotCb          = nullptr;

    RECT canvasRect{};
};

static AppState g_state;

// -----------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------
static void RunScatter(AppState& s) {
    foliage::FoliageType ft;
    ft.name           = "gui_type";
    ft.meshId         = s.meshType == 1 ? "cube" : "cross_billboard";
    ft.density        = s.density;
    ft.scaleMin       = s.scaleMin;
    ft.scaleMax       = s.scaleMax;
    ft.uniformScale   = s.uniformScale;
    ft.randomRotation = s.randomRot;
    ft.alignToNormal  = s.alignNormal;
    ft.slopeMax       = s.slopeMax;
    ft.heightMin      = s.heightMin;
    ft.heightMax      = s.heightMax;
    ft.seed           = static_cast<std::uint32_t>(s.seed);

    foliage::ScatterArea area;
    area.originX = 0.0;
    area.originZ = 0.0;
    area.width   = s.areaWidth;
    area.depth   = s.areaDepth;

    foliage::SyntheticTerrain synthTerrain;
    foliage::FlatTerrain flatTerrain;
    foliage::TerrainSampler* terrain = (s.terrainMode == 1)
        ? static_cast<foliage::TerrainSampler*>(&flatTerrain)
        : static_cast<foliage::TerrainSampler*>(&synthTerrain);

    auto result = foliage::PoissonDiscScatter::ScatterWithStats(*terrain, ft, area, 50000, 30);

    s.instances    = std::move(result.instances);
    s.candidates   = result.candidates;
    s.kept         = result.kept;
    s.avgScale     = result.avgScale;
    s.clusterCount = foliage::ClusterBuilder::ClusterCount(s.instances, 16.0);
    s.hasResult    = true;
    s.hoveredIdx   = -1;
}

static void RunPaint(AppState& s) {
    if (!s.hasResult) { RunScatter(s); }

    foliage::FoliageType ft;
    ft.name           = "gui_type";
    ft.density        = s.density;
    ft.scaleMin       = s.scaleMin;
    ft.scaleMax       = s.scaleMax;
    ft.uniformScale   = s.uniformScale;
    ft.randomRotation = s.randomRot;
    ft.alignToNormal  = s.alignNormal;
    ft.slopeMax       = s.slopeMax;
    ft.heightMin      = s.heightMin;
    ft.heightMax      = s.heightMax;
    ft.seed           = static_cast<std::uint32_t>(s.seed);

    foliage::BrushParams brush;
    brush.centerX    = s.brushCenterX;
    brush.centerZ    = s.brushCenterZ;
    brush.radius     = s.brushRadius;
    brush.densityMul = s.brushDensityMul;

    foliage::SyntheticTerrain synthTerrain;
    foliage::FlatTerrain flatTerrain;
    foliage::TerrainSampler* terrain = (s.terrainMode == 1)
        ? static_cast<foliage::TerrainSampler*>(&flatTerrain)
        : static_cast<foliage::TerrainSampler*>(&synthTerrain);

    if (s.eraseMode) {
        s.instances = foliage::FoliagePaint::EraseBrush(brush, s.instances);
    } else {
        auto added = foliage::FoliagePaint::PaintBrush(*terrain, ft, brush, s.instances);
        s.instances.insert(s.instances.end(), added.begin(), added.end());
    }
    s.kept = static_cast<std::uint32_t>(s.instances.size());
    s.clusterCount = foliage::ClusterBuilder::ClusterCount(s.instances, 16.0);
}

static void UpdateStatusText(AppState& s) {
    wchar_t buf[256];
    if (s.hasResult) {
        double fillPct = 100.0 * s.kept / (s.areaWidth * s.areaDepth * 0.01);
        swprintf_s(buf, L"Instances: %u  |  Candidates: %u  |  Avg Scale: %.2f  |  Clusters: %d  |  Fill: %.1f%%  |  %s",
            s.kept, s.candidates, s.avgScale, s.clusterCount, fillPct,
            s.eraseMode ? L"ERASE MODE" : L"PAINT MODE");
    } else {
        swprintf_s(buf, L"Ready — click Scatter to generate foliage");
    }
    SetWindowTextW(s.hwndStatusBar, buf);
}

static void SetEditFloat(HWND hwnd, double v) {
    wchar_t buf[32];
    swprintf_s(buf, L"%.2f", v);
    SetWindowTextW(hwnd, buf);
}

static double GetEditFloat(HWND hwnd) {
    wchar_t buf[64];
    GetWindowTextW(hwnd, buf, 64);
    return _wtof(buf);
}

static int GetEditInt(HWND hwnd) {
    wchar_t buf[64];
    GetWindowTextW(hwnd, buf, 64);
    return _wtoi(buf);
}

// -----------------------------------------------------------------------
// 2D Scatter View Rendering
// -----------------------------------------------------------------------
static COLORREF HeightColor(double y, double yMin, double yMax) {
    double range = (yMax - yMin) > 0.01 ? (yMax - yMin) : 1.0;
    double t = (y - yMin) / range;
    if (t < 0.0) t = 0.0;
    if (t > 1.0) t = 1.0;
    // Green (low) → Yellow → Red (high)
    int r = static_cast<int>(t * 255.0);
    int g = static_cast<int>((1.0 - t) * 200.0 + 55.0);
    int b = static_cast<int>((1.0 - t) * 150.0);
    return RGB(r, g, b);
}

static void RenderCanvas(HDC hdc, RECT& rc, AppState& s) {
    // Background
    HBRUSH bgBrush = CreateSolidBrush(RGB(20, 22, 28));
    FillRect(hdc, &rc, bgBrush);
    DeleteObject(bgBrush);

    if (!s.hasResult) {
        SetTextColor(hdc, RGB(100, 100, 110));
        SetBkMode(hdc, TRANSPARENT);
        DrawTextW(hdc, L"(no scatter data — click Scatter)", -1, &rc,
                  DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        return;
    }

    int cx = (rc.left + rc.right) / 2;
    int cy = (rc.top + rc.bottom) / 2;

    double scaleX = s.viewZoom * (rc.right - rc.left) / s.areaWidth;
    double scaleZ = s.viewZoom * (rc.bottom - rc.top) / s.areaDepth;
    double scale  = (scaleX < scaleZ) ? scaleX : scaleZ;
    scale *= 0.9;

    double offX = cx - s.areaWidth  * 0.5 * scale + s.viewOffsetX;
    double offZ = cy - s.areaDepth * 0.5 * scale + s.viewOffsetZ;

    // Find height range
    double yMin = 1e30, yMax = -1e30;
    for (const auto& inst : s.instances) {
        if (inst.y < yMin) yMin = inst.y;
        if (inst.y > yMax) yMax = inst.y;
    }
    if (yMax - yMin < 0.01) { yMin -= 1.0; yMax += 1.0; }

    // Draw terrain as background grid
    HPEN gridPen = CreatePen(PS_SOLID, 1, RGB(40, 42, 50));
    HPEN oldPen = (HPEN)SelectObject(hdc, gridPen);
    int gridStep = static_cast<int>(10.0 * scale);
    if (gridStep < 20) gridStep = 20;
    for (int gx = static_cast<int>(offX) % gridStep; gx < rc.right; gx += gridStep) {
        MoveToEx(hdc, gx, rc.top, nullptr);
        LineTo(hdc, gx, rc.bottom);
    }
    for (int gy = static_cast<int>(offZ) % gridStep; gy < rc.bottom; gy += gridStep) {
        MoveToEx(hdc, rc.left, gy, nullptr);
        LineTo(hdc, rc.right, gy);
    }

    // Draw instances
    for (size_t i = 0; i < s.instances.size(); ++i) {
        const auto& inst = s.instances[i];
        int px = static_cast<int>(offX + inst.x * scale);
        int pz = static_cast<int>(offZ + inst.z * scale);

        // Skip if off-screen
        if (px < rc.left - 5 || px > rc.right + 5 || pz < rc.top - 5 || pz > rc.bottom + 5)
            continue;

        double r = inst.scaleX * 2.5;
        if (r < 1.5) r = 1.5;
        int radius = static_cast<int>(r);

        COLORREF col = HeightColor(inst.y, yMin, yMax);

        // Highlight hovered
        if (static_cast<int>(i) == s.hoveredIdx) {
            HBRUSH hBr = CreateSolidBrush(RGB(255, 255, 100));
            RECT dot = {px - radius - 2, pz - radius - 2, px + radius + 3, pz + radius + 3};
            FillRect(hdc, &dot, hBr);
            DeleteObject(hBr);
        }

        HBRUSH hBrush = CreateSolidBrush(col);
        RECT dot = {px - radius, pz - radius, px + radius + 1, pz + radius + 1};
        FillRect(hdc, &dot, hBrush);
        DeleteObject(hBrush);

        // Direction indicator (yaw)
        if (radius >= 3) {
            int dx = static_cast<int>(std::cos(inst.yaw) * radius);
            int dz = static_cast<int>(std::sin(inst.yaw) * radius);
            HPEN dirPen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
            SelectObject(hdc, dirPen);
            MoveToEx(hdc, px, pz, nullptr);
            LineTo(hdc, px + dx, pz + dz);
            DeleteObject(dirPen);
        }
    }

    // Draw brush circle
    {
        int bpx = static_cast<int>(offX + s.brushCenterX * scale);
        int bpz = static_cast<int>(offZ + s.brushCenterZ * scale);
        int br  = static_cast<int>(s.brushRadius * scale);

        HPEN brushPen = CreatePen(PS_SOLID, 2,
            s.eraseMode ? RGB(255, 80, 80) : RGB(80, 255, 80));
        HBRUSH nullBr = (HBRUSH)GetStockObject(NULL_BRUSH);
        SelectObject(hdc, brushPen);
        SelectObject(hdc, nullBr);
        Ellipse(hdc, bpx - br, bpz - br, bpx + br, bpz + br);
        DeleteObject(brushPen);

        // Crosshair at center
        HPEN chPen = CreatePen(PS_SOLID, 1,
            s.eraseMode ? RGB(255, 100, 100) : RGB(100, 255, 100));
        SelectObject(hdc, chPen);
        MoveToEx(hdc, bpx - 8, bpz, nullptr); LineTo(hdc, bpx + 8, bpz);
        MoveToEx(hdc, bpx, bpz - 8, nullptr); LineTo(hdc, bpx, bpz + 8);
        DeleteObject(chPen);
    }

    // Restore pen
    SelectObject(hdc, oldPen);
    DeleteObject(gridPen);
}

// -----------------------------------------------------------------------
// Window procedure
// -----------------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    AppState& s = g_state;

    switch (msg) {
    case WM_CREATE: {
        s.hwnd = hwnd;

        // Create controls
        int xLeft = 10, xRight = 220, y = 5, btnH = 26;
        int editW = 160, labelW = 100;
        HINSTANCE hi = (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE);

        auto makeLabel = [&](const wchar_t* text, int x, int yy, int ww) {
            CreateWindowW(L"STATIC", text, WS_CHILD | WS_VISIBLE,
                          x, yy, ww, 20, hwnd, nullptr, hi, nullptr);
        };
        auto makeEdit = [&](int yy, double val) -> HWND {
            HWND e = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT,
                                    xLeft + labelW, yy, editW, 22, hwnd, nullptr, hi, nullptr);
            SetEditFloat(e, val);
            return e;
        };

        // Parameter panel
        makeLabel(L"Area Width:",    xLeft, y, labelW);    s.hwndAreaWEdit = makeEdit(y, s.areaWidth); y += 26;
        makeLabel(L"Area Depth:",    xLeft, y, labelW);    s.hwndAreaDEdit = makeEdit(y, s.areaDepth); y += 26;
        makeLabel(L"Density:",       xLeft, y, labelW);    s.hwndDensityEdit = makeEdit(y, s.density); y += 26;
        makeLabel(L"Scale Min:",     xLeft, y, labelW);    s.hwndScaleMinEdit = makeEdit(y, s.scaleMin); y += 26;
        makeLabel(L"Scale Max:",     xLeft, y, labelW);    s.hwndScaleMaxEdit = makeEdit(y, s.scaleMax); y += 26;
        makeLabel(L"Slope Max (°):", xLeft, y, labelW);    s.hwndSlopeEdit = makeEdit(y, s.slopeMax); y += 26;
        makeLabel(L"Height Min:",    xLeft, y, labelW);    s.hwndHeightMinEdit = makeEdit(y, s.heightMin); y += 26;
        makeLabel(L"Height Max:",    xLeft, y, labelW);    s.hwndHeightMaxEdit = makeEdit(y, s.heightMax); y += 26;
        makeLabel(L"Seed:",          xLeft, y, labelW);    s.hwndSeedEdit = makeEdit(y, (double)s.seed); y += 26;

        s.hwndAlignCb = CreateWindowW(L"BUTTON", L"Align to Normal", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                                       xLeft, y, 300, 20, hwnd, nullptr, hi, nullptr);
        Button_SetCheck(s.hwndAlignCb, s.alignNormal ? BST_CHECKED : BST_UNCHECKED);
        y += 24;

        s.hwndRotCb = CreateWindowW(L"BUTTON", L"Random Rotation", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                                     xLeft, y, 300, 20, hwnd, nullptr, hi, nullptr);
        Button_SetCheck(s.hwndRotCb, s.randomRot ? BST_CHECKED : BST_UNCHECKED);
        y += 28;

        // Action buttons
        int btnW = 200;
        s.hwndScatterBtn = CreateWindowW(L"BUTTON", L"🔀  Scatter (F5)",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            xLeft, y, btnW, btnH, hwnd, (HMENU)101, hi, nullptr);
        y += 30;

        s.hwndPaintBtn = CreateWindowW(L"BUTTON", L"🖌  Paint Brush (F6)",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            xLeft, y, btnW, btnH, hwnd, (HMENU)102, hi, nullptr);
        y += 30;

        s.hwndEraseBtn = CreateWindowW(L"BUTTON", L"🧹  Toggle Erase Mode (F7)",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            xLeft, y, btnW, btnH, hwnd, (HMENU)103, hi, nullptr);
        y += 30;

        s.hwndClearBtn = CreateWindowW(L"BUTTON", L"🗑  Clear (F8)",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            xLeft, y, btnW, btnH, hwnd, (HMENU)104, hi, nullptr);
        y += 30;

        s.hwndExportBtn = CreateWindowW(L"BUTTON", L"📋  Export JSON (F9)",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            xLeft, y, btnW, btnH, hwnd, (HMENU)105, hi, nullptr);
        y += 34;

        // Brush params
        makeLabel(L"Brush Size:",     xLeft, y, labelW);
        s.hwndBrushSizeEdit = makeEdit(y, s.brushRadius); y += 26;
        makeLabel(L"Click on canvas to set brush center", xLeft, y, 300); y += 22;

        // Mesh type radio
        makeLabel(L"Terrain:", xLeft, y, 60);
        CreateWindowW(L"BUTTON", L"Hills", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
                       xLeft + 65, y, 60, 20, hwnd, (HMENU)200, hi, nullptr);
        Button_SetCheck(GetDlgItem(hwnd, 200), s.terrainMode == 0 ? BST_CHECKED : BST_UNCHECKED);
        CreateWindowW(L"BUTTON", L"Flat", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
                       xLeft + 130, y, 60, 20, hwnd, (HMENU)201, hi, nullptr);
        Button_SetCheck(GetDlgItem(hwnd, 201), s.terrainMode == 1 ? BST_CHECKED : BST_UNCHECKED);
        y += 24;

        makeLabel(L"Mesh:", xLeft, y, 60);
        CreateWindowW(L"BUTTON", L"Cross", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
                       xLeft + 65, y, 60, 20, hwnd, (HMENU)202, hi, nullptr);
        Button_SetCheck(GetDlgItem(hwnd, 202), s.meshType == 0 ? BST_CHECKED : BST_UNCHECKED);
        CreateWindowW(L"BUTTON", L"Cube", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
                       xLeft + 130, y, 60, 20, hwnd, (HMENU)203, hi, nullptr);
        Button_SetCheck(GetDlgItem(hwnd, 203), s.meshType == 1 ? BST_CHECKED : BST_UNCHECKED);
        y += 28;

        // Legend
        makeLabel(L"Colors: green=low  red=high", xLeft, y, 300);

        // Status bar
        s.hwndStatusBar = CreateWindowW(L"STATIC", L"Ready — click Scatter to generate foliage",
            WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE,
            0, 0, 0, 0, hwnd, nullptr, hi, nullptr);

        // Default font for all controls
        HFONT hFont = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                   DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                   DEFAULT_QUALITY, DEFAULT_PITCH, L"Consolas");
        EnumChildWindows(hwnd, [](HWND c, LPARAM lp) -> BOOL {
            SendMessageW(c, WM_SETFONT, (WPARAM)lp, TRUE);
            return TRUE;
        }, (LPARAM)hFont);

        return 0;
    }

    case WM_SIZE: {
        int w = LOWORD(lp), hh = HIWORD(lp);
        // Layout: left panel 320px, rest = canvas
        s.canvasRect = {320, 0, w, hh - 28};
        SetWindowPos(s.hwndStatusBar, nullptr, 0, hh - 28, w, 28, SWP_NOZORDER);
        InvalidateRect(hwnd, nullptr, TRUE);
        return 0;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        // Paint left panel background
        RECT leftPanel = {0, 0, 320, ps.rcPaint.bottom};
        HBRUSH panelBg = CreateSolidBrush(RGB(35, 37, 45));
        FillRect(hdc, &leftPanel, panelBg);
        DeleteObject(panelBg);

        // Panel title
        SetTextColor(hdc, RGB(200, 200, 210));
        SetBkMode(hdc, TRANSPARENT);
        HFONT titleFont = CreateFontW(18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                       DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                       DEFAULT_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        HFONT oldF = (HFONT)SelectObject(hdc, titleFont);
        TextOutW(hdc, 10, 8, L"Foliage Scatter", 15);
        SelectObject(hdc, oldF);
        DeleteObject(titleFont);

        HFONT verFont = CreateFontW(11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                     DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                     DEFAULT_QUALITY, DEFAULT_PITCH, L"Consolas");
        SelectObject(hdc, verFont);
        SetTextColor(hdc, RGB(120, 120, 130));
        TextOutW(hdc, 10, 28, L"v0.2.0 — standalone test GUI", 28);
        SelectObject(hdc, oldF);
        DeleteObject(verFont);

        // Render canvas
        if (s.canvasRect.right > s.canvasRect.left + 10) {
            RenderCanvas(hdc, s.canvasRect, s);
        }

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_COMMAND: {
        int id = LOWORD(wp);
        switch (id) {
        case 101: // Scatter
            s.density     = GetEditFloat(s.hwndDensityEdit);
            s.scaleMin    = GetEditFloat(s.hwndScaleMinEdit);
            s.scaleMax    = GetEditFloat(s.hwndScaleMaxEdit);
            s.slopeMax    = GetEditFloat(s.hwndSlopeEdit);
            s.heightMin   = GetEditFloat(s.hwndHeightMinEdit);
            s.heightMax   = GetEditFloat(s.hwndHeightMaxEdit);
            s.seed        = GetEditInt(s.hwndSeedEdit);
            s.areaWidth   = GetEditFloat(s.hwndAreaWEdit);
            s.areaDepth   = GetEditFloat(s.hwndAreaDEdit);
            s.alignNormal = Button_GetCheck(s.hwndAlignCb) == BST_CHECKED;
            s.randomRot   = Button_GetCheck(s.hwndRotCb) == BST_CHECKED;
            RunScatter(s);
            UpdateStatusText(s);
            InvalidateRect(hwnd, nullptr, FALSE);
            break;
        case 102: // Paint
            s.brushRadius = GetEditFloat(s.hwndBrushSizeEdit);
            RunPaint(s);
            UpdateStatusText(s);
            InvalidateRect(hwnd, nullptr, FALSE);
            break;
        case 103: // Toggle erase
            s.eraseMode = !s.eraseMode;
            SetWindowTextW(s.hwndEraseBtn, s.eraseMode ? L"🧹  Erase Mode ON (F7 to toggle)" : L"🧹  Toggle Erase Mode (F7)");
            UpdateStatusText(s);
            InvalidateRect(hwnd, nullptr, FALSE);
            break;
        case 104: // Clear
            s.instances.clear();
            s.kept = 0;
            s.candidates = 0;
            s.hasResult = false;
            s.clusterCount = 0;
            s.hoveredIdx = -1;
            UpdateStatusText(s);
            InvalidateRect(hwnd, nullptr, FALSE);
            break;
        case 105: // Export JSON
            {
                // Write to file
                wchar_t path[MAX_PATH];
                swprintf_s(path, L"foliage_scatter_export.json");
                FILE* f = _wfopen(path, L"w");
                if (f) {
                    fprintf(f, "{\"plugin\":\"foliage_un06\",\"version\":\"0.2.0\",\n");
                    fprintf(f, " \"area\":{\"width\":%.1f,\"depth\":%.1f},\n", s.areaWidth, s.areaDepth);
                    fprintf(f, " \"params\":{\"density\":%.4f,\"scaleRange\":[%.2f,%.2f],\"slopeMax\":%.1f,\"seed\":%d},\n",
                            s.density, s.scaleMin, s.scaleMax, s.slopeMax, s.seed);
                    fprintf(f, " \"summary\":{\"kept\":%u,\"candidates\":%u,\"clusters\":%d,\"avgScale\":%.3f},\n",
                            s.kept, s.candidates, s.clusterCount, s.avgScale);
                    fprintf(f, " \"instances\":[\n");
                    int count = static_cast<int>(s.instances.size());
                    int step = count > 500 ? count / 500 + 1 : 1;
                    for (int i = 0; i < count; i += step) {
                        const auto& inst = s.instances[i];
                        fprintf(f, "  [%.2f,%.2f,%.2f,%.3f,%.3f,%.3f,%.4f]%s\n",
                                inst.x, inst.y, inst.z,
                                inst.scaleX, inst.scaleY, inst.scaleZ,
                                inst.yaw,
                                (i + step < count) ? "," : "");
                    }
                    fprintf(f, " ]}\n");
                    fclose(f);
                    wchar_t msg[512];
                    swprintf_s(msg, L"Exported %zu instances (sampled) to foliage_scatter_export.json", s.instances.size());
                    MessageBoxW(hwnd, msg, L"Export", MB_OK | MB_ICONINFORMATION);
                }
            }
            break;
        case 200: s.terrainMode = 0; break;
        case 201: s.terrainMode = 1; break;
        case 202: s.meshType = 0; break;
        case 203: s.meshType = 1; break;
        }
        return 0;
    }

    case WM_KEYDOWN: {
        bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        switch (wp) {
        case VK_F5: // Scatter
            SendMessageW(hwnd, WM_COMMAND, 101, 0);
            break;
        case VK_F6: // Paint
            SendMessageW(hwnd, WM_COMMAND, 102, 0);
            break;
        case VK_F7: // Toggle erase
            SendMessageW(hwnd, WM_COMMAND, 103, 0);
            break;
        case VK_F8: // Clear
            SendMessageW(hwnd, WM_COMMAND, 104, 0);
            break;
        case VK_F9: // Export
            SendMessageW(hwnd, WM_COMMAND, 105, 0);
            break;
        case VK_ADD: case VK_OEM_PLUS:
            s.viewZoom *= 1.2;
            InvalidateRect(hwnd, nullptr, FALSE);
            break;
        case VK_SUBTRACT: case VK_OEM_MINUS:
            s.viewZoom /= 1.2;
            if (s.viewZoom < 0.1) s.viewZoom = 0.1;
            InvalidateRect(hwnd, nullptr, FALSE);
            break;
        case VK_UP:
            if (ctrl) { s.seed++; SetEditFloat(s.hwndSeedEdit, (double)s.seed); }
            else s.viewOffsetZ += 20.0;
            InvalidateRect(hwnd, nullptr, FALSE);
            break;
        case VK_DOWN:
            if (ctrl) { s.seed--; SetEditFloat(s.hwndSeedEdit, (double)s.seed); }
            else s.viewOffsetZ -= 20.0;
            InvalidateRect(hwnd, nullptr, FALSE);
            break;
        case VK_LEFT:
            s.viewOffsetX += 20.0;
            InvalidateRect(hwnd, nullptr, FALSE);
            break;
        case VK_RIGHT:
            s.viewOffsetX -= 20.0;
            InvalidateRect(hwnd, nullptr, FALSE);
            break;
        case 'R':
            if (ctrl) { s.viewOffsetX = 0.0; s.viewOffsetZ = 0.0; s.viewZoom = 1.0; InvalidateRect(hwnd, nullptr, FALSE); }
            break;
        }
        return 0;
    }

    case WM_LBUTTONDOWN: {
        int mx = GET_X_LPARAM(lp);
        int my = GET_Y_LPARAM(lp);

        // Check if click is in canvas
        if (mx >= s.canvasRect.left && mx < s.canvasRect.right &&
            my >= s.canvasRect.top && my < s.canvasRect.bottom)
        {
            // Map to world coordinates
            int ww = s.canvasRect.right - s.canvasRect.left;
            int wh = s.canvasRect.bottom - s.canvasRect.top;
            int cx = (s.canvasRect.left + s.canvasRect.right) / 2;
            int cy = (s.canvasRect.top + s.canvasRect.bottom) / 2;
            double scaleX = s.viewZoom * ww / s.areaWidth;
            double scaleZ = s.viewZoom * wh / s.areaDepth;
            double scale  = (scaleX < scaleZ) ? scaleX : scaleZ;
            scale *= 0.9;
            double offX = cx - s.areaWidth  * 0.5 * scale + s.viewOffsetX;
            double offZ = cy - s.areaDepth * 0.5 * scale + s.viewOffsetZ;

            s.brushCenterX = (mx - offX) / scale;
            s.brushCenterZ = (my - offZ) / scale;

            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;
    }

    case WM_MOUSEMOVE: {
        int mx = GET_X_LPARAM(lp);
        int my = GET_Y_LPARAM(lp);

        if (mx >= s.canvasRect.left && mx < s.canvasRect.right &&
            my >= s.canvasRect.top && my < s.canvasRect.bottom)
        {
            int ww = s.canvasRect.right - s.canvasRect.left;
            int wh = s.canvasRect.bottom - s.canvasRect.top;
            int cx = (s.canvasRect.left + s.canvasRect.right) / 2;
            int cy = (s.canvasRect.top + s.canvasRect.bottom) / 2;
            double scaleX = s.viewZoom * ww / s.areaWidth;
            double scaleZ = s.viewZoom * wh / s.areaDepth;
            double scale  = (scaleX < scaleZ) ? scaleX : scaleZ;
            scale *= 0.9;
            double offX = cx - s.areaWidth  * 0.5 * scale + s.viewOffsetX;
            double offZ = cy - s.areaDepth * 0.5 * scale + s.viewOffsetZ;

            double wx = (mx - offX) / scale;
            double wz = (my - offZ) / scale;

            // Find nearest instance
            int bestIdx = -1;
            double bestDist = 25.0; // pixel threshold
            for (size_t i = 0; i < s.instances.size(); ++i) {
                double dx = s.instances[i].x - wx;
                double dz = s.instances[i].z - wz;
                double d2 = dx*dx + dz*dz;
                double threshold = (bestDist / scale);
                if (d2 < threshold * threshold) {
                    bestDist = std::sqrt(d2) * scale;
                    bestIdx = static_cast<int>(i);
                }
            }

            if (bestIdx != s.hoveredIdx) {
                s.hoveredIdx = bestIdx;
                // Update status
                if (bestIdx >= 0 && bestIdx < static_cast<int>(s.instances.size())) {
                    const auto& inst = s.instances[bestIdx];
                    wchar_t buf[200];
                    swprintf_s(buf, L"Instance #%d: pos=(%.1f, %.1f, %.1f) scale=(%.2f, %.2f, %.2f) yaw=%.1f°",
                        bestIdx, inst.x, inst.y, inst.z,
                        inst.scaleX, inst.scaleY, inst.scaleZ,
                        inst.yaw * 57.29578);
                    SetWindowTextW(s.hwndStatusBar, buf);
                } else {
                    UpdateStatusText(s);
                }
                InvalidateRect(hwnd, nullptr, FALSE);
            }
        }
        return 0;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

// -----------------------------------------------------------------------
// WinMain
// -----------------------------------------------------------------------
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow)
{
    // Init common controls
    INITCOMMONCONTROLSEX icc = {sizeof(icc), ICC_STANDARD_CLASSES};
    InitCommonControlsEx(&icc);

    // Register window class
    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"FoliageTestGUI";
    RegisterClassExW(&wc);

    // Create window
    HWND hwnd = CreateWindowExW(0, L"FoliageTestGUI",
        L"Foliage Scatter — Test GUI  |  F5=Scatter  F6=Paint  F7=Erase  F8=Clear  F9=Export  +/-=Zoom  Arrows=Pan  Ctrl+R=Reset View",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT, 1400, 900,
        nullptr, nullptr, hInstance, nullptr);

    if (!hwnd) return 1;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Auto-run first scatter
    PostMessageW(hwnd, WM_COMMAND, 101, 0);

    // Message loop
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return (int)msg.wParam;
}
