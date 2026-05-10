#include "platform/NativeFileDialog.h"

#ifdef _WIN32

#define NOMINMAX
#include <windows.h>
#include <commdlg.h>

#pragma comment(lib, "Comdlg32.lib")

std::string NativeFileDialog::openSaveDialog()
{
    char fileName[MAX_PATH] = "Novo Projeto.lcs";

    OPENFILENAMEA ofn;
    ZeroMemory(&ofn, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = nullptr;
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter =
        "Locus3D Scene (*.lcs)\0*.lcs\0"
        "JSON (*.json)\0*.json\0"
        "All Files (*.*)\0*.*\0";

    ofn.lpstrDefExt = "lcs";
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetSaveFileNameA(&ofn))
    {
        return std::string(fileName);
    }

    return "";
}

std::string NativeFileDialog::openLoadDialog()
{
    char fileName[MAX_PATH] = "";

    OPENFILENAMEA ofn;
    ZeroMemory(&ofn, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = nullptr;
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter =
        "Locus3D Scene (*.lcs)\0*.lcs\0"
        "JSON (*.json)\0*.json\0"
        "All Files (*.*)\0*.*\0";

    ofn.lpstrDefExt = "lcs";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetOpenFileNameA(&ofn))
    {
        return std::string(fileName);
    }

    return "";
}

#else

std::string NativeFileDialog::openSaveDialog()
{
    return "";
}

std::string NativeFileDialog::openLoadDialog()
{
    return "";
}

#endif