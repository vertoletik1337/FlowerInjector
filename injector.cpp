#include <windows.h>
#include <iostream>
#include <string>

// Специальная директива для линкера, чтобы исправить ошибку LNK2019
#pragma comment(lib, "user32.lib")

int main() {
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);
    SetConsoleTitleA("kotofey.win standchillow fuckeed! t.me/pikmizs dsc.gg/kotofey t.me/whitelotosteams t.me/kotofeymods ya ebal mamu veIvet_pulls");

    std::string dllName = "kotofeywin.dll";
    char fullDllPath[MAX_PATH];

    std::cout << "[~] Waiting Standchillow" << std::endl;

    if (!GetFullPathNameA(dllName.c_str(), MAX_PATH, fullDllPath, nullptr)) {
        std::cout << "[-] tak stop a gde kolbaska?" << std::endl;
        Sleep(3000);
        return 1;
    }

    HWND hWindow = nullptr;
    DWORD pId = 0;

    while (!hWindow) {
        hWindow = FindWindowA(nullptr, "StandChillow");
        if (hWindow) {
            GetWindowThreadProcessId(hWindow, &pId);
            if (pId == 0) {
                hWindow = nullptr;
            }
        }
        Sleep(500);
    }

    std::cout << "[+] Game detected! PID: " << pId << std::endl;
    std::cout << "[~] Injecting DLL..." << std::endl;

    HANDLE hProcess = OpenProcess(
        PROCESS_CREATE_THREAD | 
        PROCESS_QUERY_INFORMATION | 
        PROCESS_VM_OPERATION | 
        PROCESS_VM_WRITE | 
        PROCESS_VM_READ, 
        FALSE, 
        pId
    );

    if (!hProcess) {
        std::cout << "[-] Ошибка доступа: " << GetLastError() << std::endl;
        std::cout << "[!] Если код 5, запусти инжектор от Админа." << std::endl;
        system("pause");
        return 1;
    }

    void* allocatedMemory = VirtualAllocEx(hProcess, nullptr, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!allocatedMemory) {
        std::cout << "[-] Ошибка VirtualAllocEx: " << GetLastError() << std::endl;
        CloseHandle(hProcess);
        system("pause");
        return 1;
    }

    if (!WriteProcessMemory(hProcess, allocatedMemory, fullDllPath, strlen(fullDllPath) + 1, nullptr)) {
        std::cout << "[-] Ошибка WriteProcessMemory: " << GetLastError() << std::endl;
        VirtualFreeEx(hProcess, allocatedMemory, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        system("pause");
        return 1;
    }

    LPVOID loadLibraryAddress = (LPVOID)GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA");
    HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0, (LPTHREAD_START_ROUTINE)loadLibraryAddress, allocatedMemory, 0, nullptr);
    
    if (!hThread) {
        std::cout << "[-] Ошибка CreateRemoteThread: " << GetLastError() << std::endl;
        VirtualFreeEx(hProcess, allocatedMemory, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        system("pause");
        return 1;
    }

    std::cout << "[+] Injected! t.me/pikmizs dsc.gg/kotofey t.me/whitelotosteams t.me/kotofeymods" << std::endl;
    
    WaitForSingleObject(hThread, INFINITE);
    VirtualFreeEx(hProcess, allocatedMemory, 0, MEM_RELEASE);
    CloseHandle(hThread);
    CloseHandle(hProcess);

    Sleep(2000);
    return 0;
}
