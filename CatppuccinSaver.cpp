#include <windows.h>
#include <locationapi.h>
#include <winhttp.h>
#include <mmsystem.h>
#include <shellapi.h>
#include <string>
#include <vector>
#include <ctime>
#include <cmath>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "locationapi.lib")
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "winmm.lib")

static const wchar_t CLASS_NAME[] = L"CatppuccinSaverWindow";
static const COLORREF MOCHA_BASE = RGB(30, 30, 46);
static const COLORREF MOCHA_SURFACE = RGB(49, 50, 68);
static const COLORREF MOCHA_TEXT = RGB(205, 214, 244);
static const COLORREF MOCHA_SUBTEXT = RGB(166, 173, 200);
static const COLORREF MOCHA_MAUVE = RGB(203, 166, 247);
static const COLORREF MOCHA_BLUE = RGB(137, 180, 250);
static const COLORREF MOCHA_GREEN = RGB(166, 227, 161);

struct Settings {
    bool twentyFourHour = true;
    bool showSeconds = false;
    COLORREF accent = MOCHA_MAUVE;
};

struct SaverWindow {
    HWND handle = nullptr;
    bool preview = false;
    bool mouseInitialized = false;
    POINT initialMouse = {};
    Settings settings;
};

static std::vector<SaverWindow*> g_windows;
static Settings g_settings;
static bool g_configClassRegistered = false;
static std::wstring g_weather;
static std::wstring g_wind;
static std::vector<std::wstring> g_forecast;

std::wstring SettingsPath() {
    wchar_t appData[MAX_PATH] = {};
    DWORD length = GetEnvironmentVariableW(L"APPDATA", appData, MAX_PATH);
    std::wstring path = length ? std::wstring(appData, length) : L".";
    return path + L"\\CatppuccinSaver.ini";
}

void LoadSettings(Settings& settings) {
    std::wstring path = SettingsPath();
    settings.twentyFourHour = GetPrivateProfileIntW(L"Display", L"TwentyFourHour", 1, path.c_str()) != 0;
    settings.showSeconds = GetPrivateProfileIntW(L"Display", L"ShowSeconds", 0, path.c_str()) != 0;
    int accent = GetPrivateProfileIntW(L"Display", L"Accent", 0, path.c_str());
    settings.accent = accent == 1 ? MOCHA_BLUE : accent == 2 ? MOCHA_GREEN : MOCHA_MAUVE;
}

void SaveSettings(const Settings& settings) {
    std::wstring path = SettingsPath();
    WritePrivateProfileStringW(L"Display", L"TwentyFourHour", settings.twentyFourHour ? L"1" : L"0", path.c_str());
    WritePrivateProfileStringW(L"Display", L"ShowSeconds", settings.showSeconds ? L"1" : L"0", path.c_str());
    int accent = settings.accent == MOCHA_BLUE ? 1 : settings.accent == MOCHA_GREEN ? 2 : 0;
    WritePrivateProfileStringW(L"Display", L"Accent", std::to_wstring(accent).c_str(), path.c_str());
}

void FillRectColor(HDC dc, const RECT& rect, COLORREF color) {
    HBRUSH brush = CreateSolidBrush(color);
    FillRect(dc, &rect, brush);
    DeleteObject(brush);
}

void DrawCircle(HDC dc, int x, int y, int radius, COLORREF color) {
    HBRUSH brush = CreateSolidBrush(color);
    HPEN pen = CreatePen(PS_SOLID, 1, color);
    HGDIOBJ old = SelectObject(dc, brush);
    HGDIOBJ oldPen = SelectObject(dc, pen);
    Ellipse(dc, x - radius, y - radius, x + radius, y + radius);
    SelectObject(dc, old);
    SelectObject(dc, oldPen);
    DeleteObject(brush);
    DeleteObject(pen);
}

void DrawSparkle(HDC dc, int x, int y, int radius) {
    HPEN glowPen = CreatePen(PS_SOLID, 1, RGB(170, 174, 205));
    HGDIOBJ oldPen = SelectObject(dc, glowPen);
    MoveToEx(dc, x - radius, y, nullptr);
    LineTo(dc, x + radius + 1, y);
    MoveToEx(dc, x, y - radius, nullptr);
    LineTo(dc, x, y + radius + 1);
    MoveToEx(dc, x - radius / 2, y - radius / 2, nullptr);
    LineTo(dc, x + radius / 2 + 1, y + radius / 2 + 1);
    MoveToEx(dc, x + radius / 2, y - radius / 2, nullptr);
    LineTo(dc, x - radius / 2 - 1, y + radius / 2 + 1);
    HPEN corePen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
    SelectObject(dc, corePen);
    MoveToEx(dc, x - radius / 2, y, nullptr);
    LineTo(dc, x + radius / 2 + 1, y);
    MoveToEx(dc, x, y - radius / 2, nullptr);
    LineTo(dc, x, y + radius / 2 + 1);
    SelectObject(dc, oldPen);
    DeleteObject(corePen);
    DeleteObject(glowPen);
}

void DrawWeatherSkeleton(HDC dc, int centerX, int centerY) {
    int pulse = static_cast<int>((std::sin(GetTickCount64() / 450.0) + 1.0) * 10.0);
    COLORREF color = RGB(75 + pulse, 68 + pulse, 94 + pulse);
    HBRUSH brush = CreateSolidBrush(color);
    HGDIOBJ old = SelectObject(dc, brush);
    RoundRect(dc, centerX - 92, centerY + 4, centerX + 92, centerY + 24, 10, 10);
    RoundRect(dc, centerX - 54, centerY + 34, centerX + 54, centerY + 45, 6, 6);
    SelectObject(dc, old);
    DeleteObject(brush);
}

void DrawTextCentered(HDC dc, const std::wstring& text, int x, int y, COLORREF color, int height, bool bold) {
    HFONT font = CreateFontW(height, 0, 0, 0, bold ? FW_SEMIBOLD : FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        VARIABLE_PITCH, L"Segoe UI");
    HGDIOBJ oldFont = SelectObject(dc, font);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, color);
    RECT bounds = { x - 800, y, x + 800, y + height + 20 };
    DrawTextW(dc, text.c_str(), -1, &bounds, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
    SelectObject(dc, oldFont);
    DeleteObject(font);
}

std::wstring CurrentTime(const Settings& settings) {
    std::time_t now = std::time(nullptr);
    std::tm local = {};
    localtime_s(&local, &now);
    wchar_t buffer[32] = {};
    wcsftime(buffer, 32, settings.showSeconds ? L"%#I:%M:%S %p" : L"%#I:%M %p", &local);
    return buffer;
}

std::wstring CurrentDate() {
    std::time_t now = std::time(nullptr);
    std::tm local = {};
    localtime_s(&local, &now);
    wchar_t buffer[64] = {};
    wcsftime(buffer, 64, L"%A, %B %d", &local);
    return buffer;
}

std::wstring Utf8ToWide(const std::string& value) {
    int length = MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0);
    std::wstring result(length, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), result.data(), length);
    return result;
}

std::string JsonValue(const std::string& json, const std::string& key, size_t start = 0) {
    std::string marker = "\"" + key + "\"";
    size_t keyStart = json.find(marker, start);
    if (keyStart == std::string::npos) return {};
    size_t valueStart = json.find(':', keyStart + marker.size());
    if (valueStart == std::string::npos) return {};
    ++valueStart;
    while (valueStart < json.size() && (json[valueStart] == ' ' || json[valueStart] == '\t' || json[valueStart] == '\r' || json[valueStart] == '\n')) ++valueStart;
    if (valueStart < json.size() && json[valueStart] == '"') ++valueStart;
    size_t valueEnd = json.find('"', valueStart);
    return valueEnd == std::string::npos ? std::string() : json.substr(valueStart, valueEnd - valueStart);
}

std::wstring FetchWeatherFromWindowsLocation(HWND parent) {
    ILocation* location = nullptr;
    ILocationReport* baseReport = nullptr;
    ILatLongReport* report = nullptr;
    HINTERNET session = nullptr;
    HINTERNET connection = nullptr;
    HINTERNET request = nullptr;
    std::wstring result;
    std::string json;
    char buffer[4096] = {};

    DOUBLE latitude = 0;
    DOUBLE longitude = 0;
    wchar_t path[160] = {};
    IID reportType = IID_ILatLongReport;
    if (SUCCEEDED(CoCreateInstance(CLSID_Location, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&location)))) {
        location->RequestPermissions(parent, &reportType, 1, TRUE);
        if (SUCCEEDED(location->GetReport(IID_ILatLongReport, &baseReport)) &&
            SUCCEEDED(baseReport->QueryInterface(IID_PPV_ARGS(&report))) &&
            SUCCEEDED(report->GetLatitude(&latitude)) && SUCCEEDED(report->GetLongitude(&longitude))) {
            swprintf_s(path, L"/%0.5f,%0.5f?format=j1", latitude, longitude);
        }
    }
    if (path[0] == L'\0') wcscpy_s(path, L"/?format=j1");
    session = WinHttpOpen(L"CatppuccinSaver/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, nullptr, nullptr, 0);
    if (!session) goto cleanup;
    connection = WinHttpConnect(session, L"wttr.in", INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!connection) goto cleanup;
    request = WinHttpOpenRequest(connection, L"GET", path, nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (!request || !WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0, nullptr, 0, 0, 0) || !WinHttpReceiveResponse(request, nullptr)) goto cleanup;

    DWORD bytesRead = 0;
    while (WinHttpReadData(request, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        json.append(buffer, bytesRead);
    }
    if (!json.empty()) {
        size_t conditionStart = json.find("\"weatherDesc\"");
        std::string condition = conditionStart == std::string::npos ? std::string() : JsonValue(json, "value", conditionStart);
        std::string temperature = JsonValue(json, "temp_F");
        std::string wind = JsonValue(json, "windspeedMiles");
        if (!condition.empty() && !temperature.empty()) result = Utf8ToWide(condition + "  " + temperature + " F");
        if (!wind.empty()) g_wind = Utf8ToWide("Wind " + wind + " mph");
        g_forecast.clear();
        size_t position = 0;
        while (g_forecast.size() < 7) {
            size_t datePosition = json.find("\"date\":\"", position);
            if (datePosition == std::string::npos) break;
            std::string date = JsonValue(json, "date", datePosition);
            std::string maximum = JsonValue(json, "maxtempF", datePosition);
            std::string minimum = JsonValue(json, "mintempF", datePosition);
            if (date.empty() || maximum.empty() || minimum.empty()) break;
            g_forecast.push_back(Utf8ToWide(date.substr(5) + "  " + maximum + "/" + minimum + " F"));
            position = datePosition + 10;
        }
    }

cleanup:
    if (request) WinHttpCloseHandle(request);
    if (connection) WinHttpCloseHandle(connection);
    if (session) WinHttpCloseHandle(session);
    if (report) report->Release();
    if (baseReport) baseReport->Release();
    if (location) location->Release();
    return result;
}

void PaintSaver(HWND hwnd, HDC dc, const Settings& settings) {
    RECT client = {};
    GetClientRect(hwnd, &client);
    int width = client.right;
    int height = client.bottom;
    FillRectColor(dc, client, MOCHA_BASE);

    // Soft, clipped color fields create depth without external assets.
    DrawCircle(dc, width - 90, 95, 260, RGB(42, 36, 61));
    DrawCircle(dc, width - 125, 75, 175, RGB(57, 45, 76));
    DrawCircle(dc, 110, height - 25, 250, RGB(35, 46, 65));
    DrawCircle(dc, 75, height - 10, 145, RGB(43, 58, 73));

    double animationTime = GetTickCount64() / 1000.0;
    double sparkleDuration = 1;
    double sparklePeriod = 2.6;
    int sparkleCycle = static_cast<int>(animationTime / sparklePeriod);
    int sparkleIndex = (sparkleCycle * 37) % 96;
    double sparklePhase = std::fmod(animationTime, sparklePeriod);
    double sparkleStrength = sparklePhase < sparkleDuration
        ? std::sin((sparklePhase / sparkleDuration) * 3.1415926535)
        : 0.0;
    int drawableWidth = width > 40 ? width - 40 : 1;
    int drawableHeight = height > 40 ? height - 40 : 1;
    for (int index = 0; index < 96; ++index) {
        double horizontalSeed = std::fmod(index * 0.6180339887, 1.0);
        double verticalSeed = std::fmod(index * 0.7320508075, 1.0);
        double drift = animationTime * (0.025 + (index % 5) * 0.004) + index;
        int x = static_cast<int>(horizontalSeed * drawableWidth + std::sin(drift) * 10.0) + 20;
        int y = static_cast<int>(verticalSeed * drawableHeight + std::cos(drift * 0.73) * 7.0) + 20;
        DrawCircle(dc, x, y, index % 11 == 0 ? 2 : 1, RGB(255, 255, 255));
        if (index == sparkleIndex && sparkleStrength > 0.08) {
            DrawSparkle(dc, x, y, 2 + static_cast<int>(sparkleStrength * 5.0));
        }
    }

    int centerX = width / 2;
    int centerY = height / 2;
    if (g_weather.empty()) DrawWeatherSkeleton(dc, centerX, centerY - 190);
    else DrawTextCentered(dc, g_weather, centerX, centerY - 190, settings.accent, 32, true);
    DrawTextCentered(dc, CurrentTime(settings), centerX, centerY - 128, MOCHA_TEXT, 94, false);
    DrawTextCentered(dc, CurrentDate(), centerX, centerY - 18, MOCHA_SUBTEXT, 26, false);
    if (!g_wind.empty()) DrawTextCentered(dc, g_wind, centerX, centerY + 74, MOCHA_SUBTEXT, 21, false);
    if (!g_forecast.empty()) {
        int spacing = width / static_cast<int>(g_forecast.size());
        for (size_t index = 0; index < g_forecast.size(); ++index) {
            DrawTextCentered(dc, g_forecast[index], spacing * static_cast<int>(index) + spacing / 2,
                centerY + 124, MOCHA_SUBTEXT, 14, false);
        }
    }
}

void CloseSaver(HWND hwnd) {
    DestroyWindow(hwnd);
}

LRESULT CALLBACK SaverProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    SaverWindow* state = reinterpret_cast<SaverWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    switch (message) {
    case WM_NCCREATE:
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(reinterpret_cast<CREATESTRUCTW*>(lParam)->lpCreateParams));
        timeBeginPeriod(1);
        SetTimer(hwnd, 1, 7, nullptr);
        return TRUE;
    case WM_TIMER:
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT: {
        PAINTSTRUCT paint = {};
        HDC dc = BeginPaint(hwnd, &paint);
        RECT client = {};
        GetClientRect(hwnd, &client);
        int width = client.right - client.left;
        int height = client.bottom - client.top;
        HDC buffer = CreateCompatibleDC(dc);
        HBITMAP bitmap = CreateCompatibleBitmap(dc, width, height);
        HGDIOBJ oldBitmap = SelectObject(buffer, bitmap);
        PaintSaver(hwnd, buffer, state ? state->settings : g_settings);
        BitBlt(dc, 0, 0, width, height, buffer, 0, 0, SRCCOPY);
        SelectObject(buffer, oldBitmap);
        DeleteObject(bitmap);
        DeleteDC(buffer);
        EndPaint(hwnd, &paint);
        return 0;
    }
    case WM_KEYDOWN:
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_MOUSEMOVE:
        if (state && !state->preview) {
            POINT current = { static_cast<short>(LOWORD(lParam)), static_cast<short>(HIWORD(lParam)) };
            if (!state->mouseInitialized) {
                state->initialMouse = current;
                state->mouseInitialized = true;
                return 0;
            }
            if (abs(current.x - state->initialMouse.x) > 4 || abs(current.y - state->initialMouse.y) > 4) CloseSaver(hwnd);
        }
        return 0;
    case WM_DESTROY:
        KillTimer(hwnd, 1);
        timeEndPeriod(1);
        delete state;
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

bool RegisterSaverClass(HINSTANCE instance) {
    static bool registered = false;
    if (registered) return true;
    WNDCLASSW wc = {};
    wc.hInstance = instance;
    wc.lpfnWndProc = SaverProc;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;
    registered = RegisterClassW(&wc) != 0;
    return registered;
}

void RunSaver(HINSTANCE instance, HWND previewParent) {
    if (!RegisterSaverClass(instance)) return;
    bool preview = previewParent != nullptr;
    if (preview) {
        RECT bounds = {};
        GetClientRect(previewParent, &bounds);
        SaverWindow* state = new SaverWindow{ nullptr, true, false, {}, g_settings };
        state->handle = CreateWindowExW(0, CLASS_NAME, L"Catppuccin Screensaver", WS_CHILD | WS_VISIBLE,
            0, 0, bounds.right, bounds.bottom, previewParent, nullptr, instance, state);
        if (!state->handle) delete state;
        else g_weather = FetchWeatherFromWindowsLocation(state->handle);
        MSG message = {};
        while (GetMessageW(&message, nullptr, 0, 0) > 0) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
        return;
    }

    ShowCursor(FALSE);
    EnumDisplayMonitors(nullptr, nullptr, [](HMONITOR monitor, HDC, LPRECT, LPARAM data) -> BOOL {
        HINSTANCE instance = reinterpret_cast<HINSTANCE>(data);
        MONITORINFO info = { sizeof(info) };
        GetMonitorInfoW(monitor, &info);
        SaverWindow* state = new SaverWindow{ nullptr, false, false, {}, g_settings };
        state->handle = CreateWindowExW(WS_EX_TOPMOST, CLASS_NAME, L"Catppuccin Screensaver",
            WS_POPUP | WS_VISIBLE, info.rcMonitor.left, info.rcMonitor.top,
            info.rcMonitor.right - info.rcMonitor.left, info.rcMonitor.bottom - info.rcMonitor.top,
            nullptr, nullptr, instance, state);
        if (state->handle) g_windows.push_back(state);
        else delete state;
        return TRUE;
    }, reinterpret_cast<LPARAM>(instance));

    if (!g_windows.empty()) g_weather = FetchWeatherFromWindowsLocation(g_windows.front()->handle);

    MSG message = {};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    ShowCursor(TRUE);
}

LRESULT CALLBACK ConfigProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    Settings* settings = reinterpret_cast<Settings*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    switch (message) {
    case WM_NCCREATE:
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(reinterpret_cast<CREATESTRUCTW*>(lParam)->lpCreateParams));
        return TRUE;
    case WM_CREATE: {
        CreateWindowW(L"STATIC", L"Catppuccin Screensaver", WS_CHILD | WS_VISIBLE, 24, 20, 300, 28, hwnd, nullptr, nullptr, nullptr);
        CreateWindowW(L"BUTTON", L"Use 24-hour clock", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 24, 65, 220, 25, hwnd, reinterpret_cast<HMENU>(101), nullptr, nullptr);
        CreateWindowW(L"BUTTON", L"Show seconds", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 24, 98, 220, 25, hwnd, reinterpret_cast<HMENU>(102), nullptr, nullptr);
        CreateWindowW(L"STATIC", L"Accent", WS_CHILD | WS_VISIBLE, 24, 143, 80, 22, hwnd, nullptr, nullptr, nullptr);
        HWND combo = CreateWindowW(L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 100, 140, 170, 100, hwnd, reinterpret_cast<HMENU>(103), nullptr, nullptr);
        SendMessageW(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Mauve"));
        SendMessageW(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Blue"));
        SendMessageW(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Green"));
        SendMessageW(combo, CB_SETCURSEL, settings->accent == MOCHA_BLUE ? 1 : settings->accent == MOCHA_GREEN ? 2 : 0, 0);
        SendMessageW(GetDlgItem(hwnd, 101), BM_SETCHECK, settings->twentyFourHour ? BST_CHECKED : BST_UNCHECKED, 0);
        SendMessageW(GetDlgItem(hwnd, 102), BM_SETCHECK, settings->showSeconds ? BST_CHECKED : BST_UNCHECKED, 0);
        CreateWindowW(L"BUTTON", L"Save", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 106, 190, 82, 28, hwnd, reinterpret_cast<HMENU>(IDOK), nullptr, nullptr);
        CreateWindowW(L"BUTTON", L"Cancel", WS_CHILD | WS_VISIBLE, 196, 190, 82, 28, hwnd, reinterpret_cast<HMENU>(IDCANCEL), nullptr, nullptr);
        return 0;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK) {
            settings->twentyFourHour = SendMessageW(GetDlgItem(hwnd, 101), BM_GETCHECK, 0, 0) == BST_CHECKED;
            settings->showSeconds = SendMessageW(GetDlgItem(hwnd, 102), BM_GETCHECK, 0, 0) == BST_CHECKED;
            int accent = static_cast<int>(SendMessageW(GetDlgItem(hwnd, 103), CB_GETCURSEL, 0, 0));
            settings->accent = accent == 1 ? MOCHA_BLUE : accent == 2 ? MOCHA_GREEN : MOCHA_MAUVE;
            SaveSettings(*settings);
            DestroyWindow(hwnd);
        } else if (LOWORD(wParam) == IDCANCEL) {
            DestroyWindow(hwnd);
        }
        return 0;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

void ShowConfig(HINSTANCE instance) {
    const wchar_t configClass[] = L"CatppuccinSaverConfig";
    WNDCLASSW wc = {};
    wc.hInstance = instance;
    wc.lpfnWndProc = ConfigProc;
    wc.lpszClassName = configClass;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    RegisterClassW(&wc);
    Settings working = g_settings;
    HWND window = CreateWindowExW(WS_EX_DLGMODALFRAME, configClass, L"Catppuccin Screensaver Settings",
        WS_CAPTION | WS_SYSMENU | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 320, 270,
        nullptr, nullptr, instance, &working);
    if (!window) return;
    EnableWindow(GetConsoleWindow(), FALSE);
    MSG message = {};
    while (IsWindow(window) && GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    g_settings = working;
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR commandLine, int) {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    LoadSettings(g_settings);
    int argc = 0;
    LPWSTR* args = CommandLineToArgvW(GetCommandLineW(), &argc);
    wchar_t mode = L's';
    HWND preview = nullptr;
    if (argc > 1) {
        std::wstring argument = args[1];
        if (!argument.empty()) mode = static_cast<wchar_t>(towlower(argument[1]));
        if (mode == L'p' && argc > 2) preview = reinterpret_cast<HWND>(static_cast<ULONG_PTR>(_wcstoui64(args[2], nullptr, 10)));
    }
    if (args) LocalFree(args);

    if (mode == L'c') ShowConfig(instance);
    else {
        if (mode == L'p') RunSaver(instance, preview);
        else RunSaver(instance, nullptr);
    }
    CoUninitialize();
    return 0;
}
