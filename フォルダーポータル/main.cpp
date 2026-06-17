#include <windows.h>
#include <shellapi.h>
#include <exdisp.h>     // IShellWindows や IWebBrowser2 を使うため
#include <shlguid.h>    // CLSID_ShellWindows を使うため
#include <shlwapi.h>    // パス変換関数（PathCreateFromUrlW）を使うため
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "shlwapi.lib") // パス変換用
#pragma comment(lib, "ole32.lib")   // COM用 (CoInitialize等)
#pragma comment(lib, "oleaut32.lib")// COM用 (Variant等)

// 古いWindows 10 SDK環境対策
#ifndef SH_HWND
typedef HWND SH_HWND;
#endif

// フォルダ情報を格納する構造体
struct PortalFolder {
    std::wstring name;
    std::wstring path;
};

// 登録されたフォルダの一覧
std::vector<PortalFolder> g_folders;

// UTF-8の日本語テキストを、C++用のワイド文字列（wstring）に正しく変換するヘルパー関数
std::wstring MultiByteToWide(const std::string& str) {
    if (str.empty()) return L"";
    int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.length(), NULL, 0);
    std::wstring wstr(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.length(), &wstr[0], size);
    return wstr;
}

// デフォルトの設定ファイルを自動作成する関数
void CreateDefaultConfig() {
    std::ofstream ofs("folders.txt");
    if (ofs) {
        ofs << "Documents,C:\\Users\\Public\\Documents\n";
        ofs << "Downloads,C:\\Users\\Public\\Downloads\n";
        ofs << "Windows System,C:\\Windows\n";
    }
}

// 設定ファイル（folders.txt）からフォルダ一覧を読み込む関数
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
            // メモ帳が自動付与するBOM(目印)を検知して取り除く
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
            PortalFolder item;
            item.name = MultiByteToWide(name);
            item.path = MultiByteToWide(path);
            g_folders.push_back(item);
        }
    }
}

// ★【今回のブラッシュアップ】
// 現在Windowsで開いている「一致するフォルダのエクスプローラー窓」を「すべて」最前面にする関数
// 最前面にできた窓が1つでもあれば true を、1つもなければ false を返します
bool BringExplorerWindowsToFront(const std::wstring& targetPath) {
    bool anyBroughtToFront = false;

    IShellWindows* pShellWindows = NULL;
    HRESULT hr = CoCreateInstance(CLSID_ShellWindows, NULL, CLSCTX_ALL, IID_IShellWindows, (void**)&pShellWindows);
    if (SUCCEEDED(hr) && pShellWindows) {
        long count = 0;
        pShellWindows->get_Count(&count);

        // すべてのエクスプローラーを最後まで巡回する
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

                            // パスが一致した場合
                            if (_wcsicmp(path1.c_str(), path2.c_str()) == 0) {
                                // 窓を復元して表示
                                if (IsIconic(hwnd)) {
                                    ShowWindow(hwnd, SW_RESTORE);
                                } else {
                                    ShowWindow(hwnd, SW_SHOW);
                                }
                                // 最前面に持ってくる
                                SetForegroundWindow(hwnd);
                                anyBroughtToFront = true;

                                // ★【ポイント】
                                // ここにあった break を消したことで、2枚目、3枚目の一致する窓も
                                // すべて順番に最前面に呼び出されるようになります。
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

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        LoadConfig();

        int y = 20;
        for (size_t i = 0; i < g_folders.size(); ++i) {
            CreateWindowExW(
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
            y += 50;
        }

        CreateWindowExW(
            0,
            L"BUTTON",
            L"⚙ 設定を開く (フォルダ登録)",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            20, y + 10, 260, 40,
            hwnd,
            (HMENU)999,
            (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
            NULL
        );

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
            MessageBoxW(hwnd, 
                L"folders.txt を編集・上書き保存した後は、このアプリを一度閉じて再起動してください。", 
                L"設定ファイルの編集", 
                MB_OK | MB_ICONINFORMATION
            );
        }
        break;
    }

    case WM_CONTEXTMENU: {
        HWND hwndChild = (HWND)wParam; 
        int id = GetDlgCtrlID(hwndChild); 

        if (id >= 100 && id < 100 + g_folders.size()) {
            size_t index = id - 100;

            // ★【修正ポイント】
            // 一致するフォルダをすべて最前面化。1枚も開いていなかった（falseが返った）場合のみ新規作成
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