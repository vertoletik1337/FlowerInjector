#include <windows.h>
#include <iostream>
#include <tlhelp32.h>
#include <string>

// Хитрый поиск ID процесса по имени, чтобы не палить явные вызовы
DWORD GetTargetProcessId(const std::string& processName) {
    DWORD processId = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    
    if (hSnap != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 pEntry;
        pEntry.dwSize = sizeof(pEntry);
        
        if (Process32First(hSnap, &pEntry)) {
            do {
                if (processName.compare(pEntry.szExeFile) == 0) {
                    processId = pEntry.dwSize > 0 ? pEntry.th32ProcessID : 0;
                    break;
                }
            } while (Process32Next(hSnap, &pEntry));
        }
        CloseHandle(hSnap);
    }
    return processId;
}

int main() {
    // Меняем заголовок консоли, чтобы запутать систему
    SetConsoleTitleA("kotofey.win");

    std::string processName = "StandChillow.exe";
    std::string dllName = "kotofeywin.dll";
    char fullDllPath[MAX_PATH];

    std::cout << "[~] Searching for subsystem link..." << std::endl;

    // Ищем полный путь к нашей DLL, которая лежит рядом
    if (!GetFullPathNameA(dllName.c_str(), MAX_PATH, fullDllPath, nullptr)) {
        std::cout << "[-] Failed to resolve local library path!" << std::endl;
        Sleep(3000);
        return 1;
    }

    // Ждем процесс игры, если он еще не запущен
    DWORD pId = 0;
    while (pId == 0) {
        pId = GetTargetProcessId(processName);
        Sleep(500);
    }

    std::cout << "[+] Target identified! ID: " << pId << std::endl;
    std::cout << "[~] Attempting memory allocation..." << std::endl;

    // Открываем процесс с нужными правами для инжекта
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pId);
    if (!hProcess) {
        std::cout << "[-] Kernel access denied! Run as Administrator or check Defender." << std::endl;
        std::cout << "[-] Error code: " << GetLastError() << std::endl;
        system("pause");
        return 1;
    }

    // Выделяем память внутри приватки под путь к DLL
    void* allocatedMemory = VirtualAllocEx(hProcess, nullptr, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!allocatedMemory) {
        std::cout << "[-] Failed to allocate virtual cluster!" << std::endl;
        CloseHandle(hProcess);
        system("pause");
        return 1;
    }

    // Записываем путь к нашей DLL в память игры
    if (!WriteProcessMemory(hProcess, allocatedMemory, fullDllPath, strlen(fullDllPath) + 1, nullptr)) {
        std::cout << "[-] Failed to write payload into process memory!" << std::endl;
        VirtualFreeEx(hProcess, allocatedMemory, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        system("pause");
        return 1;
    }

    // Получаем адрес функции LoadLibraryA из ядра винды
    LPVOID loadLibraryAddress = (LPVOID)GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA");
    if (!loadLibraryAddress) {
        std::cout << "[-] Failed to locate system handler!" << std::endl;
        VirtualFreeEx(hProcess, allocatedMemory, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        system("pause");
        return 1;
    }

    // Создаем поток в игре, который принудительно загрузит нашу DLL
    HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0, (LPTHREAD_START_ROUTINE)loadLibraryAddress, allocatedMemory, 0, nullptr);
    if (!hThread) {
        std::cout << "[-] Thread execution blocked by host system!" << std::endl;
        VirtualFreeEx(hProcess, allocatedMemory, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        system("pause");
        return 1;
    }

    std::cout << "[+] Injection sequence complete! Subsystem linked successfully." << std::endl;
    
    // Чистим за собой следы
    WaitForSingleObject(hThread, INFINITE);
    VirtualFreeEx(hProcess, allocatedMemory, 0, MEM_RELEASE);
    CloseHandle(hThread);
    CloseHandle(hProcess);

    Sleep(2000);
    return 0;
}
