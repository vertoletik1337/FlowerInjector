#include <windows.h>
#include <iostream>
#include <tlhelp32.h>
#include <string>

// Функция для поиска ID процесса приватки по её имени
DWORD GetProcessIdByName(const char* processName) {
    DWORD processId = 0;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    
    if (snapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 processEntry;
        processEntry.dwSize = sizeof(processEntry);

        if (Process32First(snapshot, &processEntry)) {
            do {
                if (strcmp(processEntry.szExeFile, processName) == 0) {
                    processId = processEntry.th32ProcessID;
                    break;
                }
            } while (Process32Next(snapshot, &processEntry));
        }
        CloseHandle(snapshot);
    }
    return processId;
}

int main() {
    // Четко прописываем процесс приватки и имя нашей DLL
    const char* targetProcess = "StandChillow.exe";
    const char* dllName = "kotofeywin.dll";

    // Получаем абсолютный путь к нашей DLL-ке
    char fullDllPath[MAX_PATH];
    GetFullPathNameA(dllName, MAX_PATH, fullDllPath, nullptr);

    std::cout << "[+] Waiting for StandChillow.exe..." << std::endl;
    
    // Ищем процесс в цикле (инжектор будет висеть и ждать, пока ты не запустишь игру)
    DWORD processId = 0;
    while (!processId) {
        processId = GetProcessIdByName(targetProcess);
        Sleep(200); 
    }

    std::cout << "[+] Found StandChillow! ID: " << processId << std::endl;

    // Открываем процесс игры
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
    if (!hProcess) {
        std::cout << "[-] Failed to open process! Please run injector as Administrator." << std::endl;
        std::cin.get();
        return 1;
    }

    // Выделяем память внутри приватки
    LPVOID allocatedMemory = VirtualAllocEx(hProcess, nullptr, strlen(fullDllPath) + 1, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!allocatedMemory) {
        std::cout << "[-] Failed to allocate memory in StandChillow." << std::endl;
        CloseHandle(hProcess);
        std::cin.get();
        return 1;
    }

    // Пишем путь DLL в память игры
    if (!WriteProcessMemory(hProcess, allocatedMemory, fullDllPath, strlen(fullDllPath) + 1, nullptr)) {
        std::cout << "[-] Failed to write path into memory." << std::endl;
        VirtualFreeEx(hProcess, allocatedMemory, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        std::cin.get();
        return 1;
    }

    // Берём адрес LoadLibraryA
    LPVOID loadLibraryAddress = (LPVOID)GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA");
    if (!loadLibraryAddress) {
        std::cout << "[-] Failed to get LoadLibraryA address." << std::endl;
        VirtualFreeEx(hProcess, allocatedMemory, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        std::cin.get();
        return 1;
    }

    std::cout << "[+] Injecting kotofeywin.dll..." << std::endl;

    // Создаем поток, который загрузит наш чит прямо в приватку
    HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0, (LPTHREAD_START_ROUTINE)loadLibraryAddress, allocatedMemory, 0, nullptr);
    if (!hThread) {
        std::cout << "[-] CreateRemoteThread failed. Anticheat bypass needed?" << std::endl;
        VirtualFreeEx(hProcess, allocatedMemory, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        std::cin.get();
        return 1;
    }

    // Ждем окончания загрузки
    WaitForSingleObject(hThread, INFINITE);

    std::cout << "[+] Successfully injected into StandChillow! Menu key: INSERT" << std::endl;

    // Чистим за собой мусор в памяти
    VirtualFreeEx(hProcess, allocatedMemory, 0, MEM_RELEASE);
    CloseHandle(hThread);
    CloseHandle(hProcess);

    Sleep(3000);
    return 0;
}
