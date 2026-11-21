#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#include <vector>
#include <string>
#include <cstring>
#include <cstdio>
#include <filesystem>
#include "../arctool2/ArchiveBuilder.h"

static HWND gList;
static HWND gOutput;
static HWND gStatus;
static std::vector<std::string> gFiles;

enum { ID_ADD = 1001, ID_REMOVE = 1002, ID_CLEAR = 1003, ID_OUTPUT = 1004, ID_BUILD = 1005 };

static void SetStatus(const char* value) {
    SetWindowTextA(gStatus, value);
}

static void RefreshList() {
    SendMessageA(gList, LB_RESETCONTENT, 0, 0);
    for (const auto& file : gFiles)
        SendMessageA(gList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(file.c_str()));
    char status[128];
    sprintf_s(status, "%zu file(s) selected", gFiles.size());
    SetStatus(status);
}

static void AddFiles(HWND window) {
    static char buffer[65536] = {};
    OPENFILENAMEA ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = window;
    ofn.lpstrFilter = "All files\0*.*\0\0";
    ofn.lpstrFile = buffer;
    ofn.nMaxFile = sizeof(buffer);
    ofn.Flags = OFN_EXPLORER | OFN_ALLOWMULTISELECT | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    if (!GetOpenFileNameA(&ofn))
        return;

    char* next = buffer + strlen(buffer) + 1;
    if (*next == 0) {
        gFiles.emplace_back(buffer);
    } else {
        std::string directory(buffer);
        while (*next) {
            gFiles.push_back(directory + "\\" + next);
            next += strlen(next) + 1;
        }
    }

    if (gFiles.size() > 255) {
        gFiles.resize(255);
        MessageBoxA(window, "SAA2 supports at most 255 files in this builder.", "Archive limit", MB_ICONWARNING);
    }
    RefreshList();
}

static void RemoveSelected() {
    LRESULT index = SendMessageA(gList, LB_GETCURSEL, 0, 0);
    if (index == LB_ERR)
        return;
    gFiles.erase(gFiles.begin() + index);
    RefreshList();
}

static void ChooseOutput(HWND window) {
    char path[MAX_PATH] = "samp.saa";
    OPENFILENAMEA ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = window;
    ofn.lpstrFilter = "SA-MP Archive (*.saa)\0*.saa\0All files\0*.*\0\0";
    ofn.lpstrFile = path;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = "saa";
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    if (GetSaveFileNameA(&ofn))
        SetWindowTextA(gOutput, path);
}

static void BuildArchive(HWND window) {
    if (gFiles.empty()) {
        MessageBoxA(window, "Add at least one file first.", "No input files", MB_ICONINFORMATION);
        return;
    }

    char output[MAX_PATH]{};
    GetWindowTextA(gOutput, output, MAX_PATH);
    if (!output[0]) {
        MessageBoxA(window, "Choose an output archive path.", "Missing output", MB_ICONWARNING);
        return;
    }

    if (GetFileAttributesA("pkey.bin") == INVALID_FILE_ATTRIBUTES ||
        GetFileAttributesA("skey.bin") == INVALID_FILE_ATTRIBUTES) {
        MessageBoxA(window, "pkey.bin or skey.bin is missing beside the application.", "Archive keys missing", MB_ICONERROR);
        return;
    }

    EnableWindow(GetDlgItem(window, ID_BUILD), FALSE);
    SetStatus("Building archive...");
    UpdateWindow(window);

    try {
        CArchiveBuilder builder;
        for (const auto& file : gFiles) {
            std::string path = file;
            builder.AddFile(path.data());
        }
        builder.WriteArchive(output);
        char message[MAX_PATH + 96];
        sprintf_s(message, "Archive created successfully:\n%s", output);
        MessageBoxA(window, message, "SAA2 Builder", MB_ICONINFORMATION);
        SetStatus("Build completed successfully.");
    } catch (const char* error) {
        MessageBoxA(window, error, "Archive build failed", MB_ICONERROR);
        SetStatus("Build failed.");
    } catch (...) {
        MessageBoxA(window, "An unexpected error occurred while building the archive.", "Archive build failed", MB_ICONERROR);
        SetStatus("Build failed.");
    }

    EnableWindow(GetDlgItem(window, ID_BUILD), TRUE);
}

static HWND AddControl(HWND parent, const char* type, const char* text, DWORD style, int x, int y, int w, int h, int id) {
    return CreateWindowExA(0, type, text, WS_CHILD | WS_VISIBLE | style,
        x, y, w, h, parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
        GetModuleHandleA(nullptr), nullptr);
}

static LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:
        AddControl(window, "STATIC", "SA-MP SAA2 Archive Builder", SS_LEFT, 18, 14, 460, 28, 0);
        gList = AddControl(window, "LISTBOX", "", WS_BORDER | WS_VSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT, 18, 50, 544, 250, 0);
        AddControl(window, "BUTTON", "Add files...", BS_PUSHBUTTON, 18, 312, 112, 30, ID_ADD);
        AddControl(window, "BUTTON", "Remove", BS_PUSHBUTTON, 138, 312, 92, 30, ID_REMOVE);
        AddControl(window, "BUTTON", "Clear", BS_PUSHBUTTON, 238, 312, 82, 30, ID_CLEAR);
        AddControl(window, "STATIC", "Output archive", SS_LEFT, 18, 360, 120, 20, 0);
        gOutput = AddControl(window, "EDIT", "samp.saa", WS_BORDER | ES_AUTOHSCROLL, 18, 384, 438, 26, 0);
        AddControl(window, "BUTTON", "Browse...", BS_PUSHBUTTON, 466, 382, 96, 30, ID_OUTPUT);
        AddControl(window, "BUTTON", "Build SAA2", BS_DEFPUSHBUTTON, 424, 432, 138, 36, ID_BUILD);
        gStatus = AddControl(window, "STATIC", "Ready", SS_LEFT, 18, 440, 380, 22, 0);
        RefreshList();
        return 0;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_ADD: AddFiles(window); return 0;
        case ID_REMOVE: RemoveSelected(); return 0;
        case ID_CLEAR: gFiles.clear(); RefreshList(); return 0;
        case ID_OUTPUT: ChooseOutput(window); return 0;
        case ID_BUILD: BuildArchive(window); return 0;
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(window, message, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int show) {
    char module[MAX_PATH]{};
    GetModuleFileNameA(nullptr, module, MAX_PATH);
    std::filesystem::path appPath(module);
    SetCurrentDirectoryA(appPath.parent_path().string().c_str());

    INITCOMMONCONTROLSEX controls{ sizeof(controls), ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&controls);

    WNDCLASSEXA wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = "SAA2ArchiveBuilderWindow";
    if (!RegisterClassExA(&wc))
        return 1;

    HWND window = CreateWindowExA(0, wc.lpszClassName, "SA-MP SAA2 Archive Builder",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 600, 525, nullptr, nullptr, instance, nullptr);
    if (!window)
        return 1;

    ShowWindow(window, show);
    UpdateWindow(window);

    MSG message{};
    while (GetMessageA(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageA(&message);
    }
    return static_cast<int>(message.wParam);
}
