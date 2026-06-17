#include <windows.h>
#include <shellapi.h>
#include <exdisp.h>
#include <shlguid.h>
#include <shlwapi.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")

#ifndef SH_HWND
typedef HWND SH_HWND;
#endif

enum class Language {
    Japanese,
    English
};

struct PortalFolder {
    std::wstring name;
    std::wstring path;
};

Language g_lang = Language::Japanese;
std::vector<PortalFolder> g_folders;

HWND g_hCombo = NULL;
HWND g_hLangLabel = NULL;
HWND g_hSettingsButton = NULL;

std::wstring MultiByteToWide(const std::string& str) {
    if (str.empty()) return L"";
    int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.length(), NULL, 0);
    std::wstring wstr(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.length(), &wstr[0], size);
    return wstr;
}

std::string WideToMultiByte(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.length(), NULL, 0, NULL, NULL);
    std::string str(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.length(), &str[0], size, NULL, NULL);
    return str;
}

void CreateDefaultConfig() {
    std::ofstream ofs("folders.txt");
    if (ofs) {
        ofs << "Language,Japanese\n";
        ofs << "Documents,C:\\Users\\Public\\Documents\n";
        ofs << "Downloads,C:\\Users\\Public\\Downloads\n";
        ofs << "Windows System,C:\\Windows\n";
    }
}

void SaveConfig() {
    std::ofstream ofs("folders.txt");
    if (ofs) {
        ofs << "Language," << (g_lang == Language::English ? "English" : "Japanese") << "\n";
        for (const auto& folder : g_folders) {
            ofs << WideToMultiByte(folder.name) << "," << WideToMultiByte(folder.path) << "\n";
        }
    }
}

void LoadConfig() {
    g_folders.clear();
    std::ifstream ifs("folders.txt");
    if (!ifs) {
        CreateDefaultConfig();
        ifs.open("folders.txt");
    }

    std::string line;
    bool isFirstLine = true;

    while (std::getline(ifs, line)) {
        if (isFirstLine) {
            isFirstLine = false;
            if (line.size() >= 3 && 
                (unsigned char)line[0] == 0xEF && 
                (unsigned char)line[1] == 0xBB && 
                (unsigned char)line[2] == 0xBF) {
                line = line.substr(3);
            }
        }

        if (line.empty()) continue;

        std::stringstream ss(line);
        std::string name, path;
        if (std::getline(ss, name, ',') && std::getline(ss, path)) {
            if (name == "Language" || name == "language") {
                if (path == "English" || path == "english") {
                    g_lang = Language::English;
                } else {
                    g_lang = Language::Japanese;
                }
                continue;
            }
            PortalFolder item;
            item.name = MultiByteToWide(name);
            item.path = MultiByteToWide(path);
            g_folders.push_back(item);
        }
    }
}

bool BringExplorerWindowsToFront(const std::wstring& targetPath) {
    bool anyBroughtToFront = false;

    IShellWindows* pShellWindows = NULL;
    HRESULT hr = CoCreateInstance(CLSID_ShellWindows, NULL, CLSCTX_ALL, IID_IShellWindows, (void**)&pShellWindows);
    if (SUCCEEDED(hr) && pShellWindows) {
        long count = 0;
        pShellWindows->get_Count(&count);

        for (long i = 0; i < count; ++i) {
            VARIANT v;
            VariantInit(&v);
            v.vt = VT_I4;
            v.lVal = i;

            IDispatch* pDisp = NULL;
            if (SUCCEEDED(pShellWindows->Item(v, &pDisp)) && pDisp) {
                IWebBrowser2* pBrowser = NULL;
                if (SUCCEEDED(pDisp->QueryInterface(IID_IWebBrowser2, (void**)&pBrowser)) && pBrowser) {

                    SHANDLE_PTR hwndBrowser = 0;
                    pBrowser->get_HWND(&hwndBrowser);
                    HWND hwnd = (HWND)hwndBrowser;

                    BSTR bstrUrl = NULL;
                    pBrowser->get_LocationURL(&bstrUrl);
                    if (bstrUrl) {
                        std::wstring url(bstrUrl);
                        SysFreeString(bstrUrl);

                        wchar_t localPath[MAX_PATH] = { 0 };
                        DWORD localPathLen = MAX_PATH;
                        if (SUCCEEDED(PathCreateFromUrlW(url.c_str(), localPath, &localPathLen, 0))) {
                            std::wstring path1 = targetPath;
                            std::wstring path2 = localPath;
                            if (!path1.empty() && path1.back() == L'\\') path1.pop_back();
                            if (!path2.empty() && path2.back() == L'\\') path2.pop_back();

                            if (_wcsicmp(path1.c_str(), path2.c_str()) == 0) {
                                if (IsIconic(hwnd)) {
                                    ShowWindow(hwnd, SW_RESTORE);
                                } else {
                                    ShowWindow(hwnd, SW_SHOW);
                                }
                                SetForegroundWindow(hwnd);
                                anyBroughtToFront = true;
                            }
                        }
                    }
                    pBrowser->Release();
                }
                pDisp->Release();
            }
        }
        pShellWindows->Release();
    }
    return anyBroughtToFront;
}

void UpdateLanguageUI(HWND hwnd) {
    if (g_lang == Language::English) {
        SetWindowTextW(g_hLangLabel, L"Language:");
        SetWindowTextW(g_hSettingsButton, L"Open Settings (Edit folders.txt)");
        SetWindowTextW(hwnd, L"Folder Portal");
    } else {
        SetWindowTextW(g_hLangLabel, L"表示言語:");
        SetWindowTextW(g_hSettingsButton, L"設定を開く (フォルダ登録)");
        SetWindowTextW(hwnd, L"Folder Portal");
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        LoadConfig();

        HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

        g_hLangLabel = CreateWindowExW(
            0,
            L"STATIC",
            L"",
            WS_CHILD | WS_VISIBLE,
            20, 18, 80, 20,
            hwnd,
            NULL,
            (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
            NULL
        );
        SendMessageW(g_hLangLabel, WM_SETFONT, (WPARAM)hFont, TRUE);

        g_hCombo = CreateWindowExW(
            0,
            L"COMBOBOX",
            L"",
            CBS_DROPDOWNLIST | WS_CHILD | WS_VISIBLE | WS_TABSTOP,
            110, 14, 170, 100,
            hwnd,
            (HMENU)888,
            (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
            NULL
        );
        SendMessageW(g_hCombo, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessageW(g_hCombo, CB_ADDSTRING, 0, (LPARAM)L"日本語 (Japanese)");
        SendMessageW(g_hCombo, CB_ADDSTRING, 0, (LPARAM)L"English");
        SendMessageW(g_hCombo, CB_SETCURSEL, (g_lang == Language::English ? 1 : 0), 0);

        int y = 55;
        for (size_t i = 0; i < g_folders.size(); ++i) {
            HWND hButton = CreateWindowExW(
                0,
                L"BUTTON",
                g_folders[i].name.c_str(),
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                20, y, 260, 40,
                hwnd,
                (HMENU)(100 + i),
                (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
                NULL
            );
            SendMessageW(hButton, WM_SETFONT, (WPARAM)hFont, TRUE);
            y += 50;
        }

        g_hSettingsButton = CreateWindowExW(
            0,
            L"BUTTON",
            L"",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            20, y + 10, 260, 40,
            hwnd,
            (HMENU)999,
            (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
            NULL
        );
        SendMessageW(g_hSettingsButton, WM_SETFONT, (WPARAM)hFont, TRUE);

        UpdateLanguageUI(hwnd);

        RECT rect = { 0, 0, 300, y + 70 };
        AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
        SetWindowPos(hwnd, NULL, 0, 0, 
            rect.right - rect.left, 
            rect.bottom - rect.top, 
            SWP_NOMOVE | SWP_NOZORDER
        );
        break;
    }

    case WM_COMMAND: {
        int id = LOWORD(wParam);

        if (id >= 100 && id < 100 + g_folders.size()) {
            size_t index = id - 100;
            std::wstring param = L"\"" + g_folders[index].path + L"\"";
            ShellExecuteW(NULL, L"open", L"explorer.exe", param.c_str(), NULL, SW_SHOWNORMAL);
        }
        else if (id == 999) {
            ShellExecuteW(NULL, L"open", L"folders.txt", NULL, NULL, SW_SHOWNORMAL);
            if (g_lang == Language::English) {
                MessageBoxW(hwnd, 
                    L"Please restart the app after editing and saving folders.txt to apply changes.", 
                    L"Edit Settings", 
                    MB_OK | MB_ICONINFORMATION
                );
            } else {
                MessageBoxW(hwnd, 
                    L"folders.txt を編集・上書き保存した後は、このアプリを一度閉じて再起動してください。", 
                    L"設定ファイルの編集", 
                    MB_OK | MB_ICONINFORMATION
                );
            }
        }
        else if (id == 888 && HIWORD(wParam) == CBN_SELCHANGE) {
            int sel = (int)SendMessageW(g_hCombo, CB_GETCURSEL, 0, 0);
            if (sel == 0) {
                g_lang = Language::Japanese;
            } else {
                g_lang = Language::English;
            }
            SaveConfig();
            UpdateLanguageUI(hwnd);
        }
        break;
    }

    case WM_CONTEXTMENU: {
        HWND hwndChild = (HWND)wParam; 
        int id = GetDlgCtrlID(hwndChild); 

        if (id >= 100 && id < 100 + g_folders.size()) {
            size_t index = id - 100;

            if (!BringExplorerWindowsToFront(g_folders[index].path)) {
                std::wstring param = L"\"" + g_folders[index].path + L"\"";
                ShellExecuteW(NULL, L"open", L"explorer.exe", param.c_str(), NULL, SW_SHOWNORMAL);
            }
        }
        break;
    }

    case WM_DESTROY: {
        PostQuitMessage(0);
        break;
    }
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    const wchar_t CLASS_NAME[] = L"FolderPortalWindowClass";

    WNDCLASSW wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(
        0,
        CLASS_NAME,
        L"Folder Portal",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 320, 240,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (hwnd == NULL) {
        CoUninitialize();
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    CoUninitialize();

    return 0;
}
