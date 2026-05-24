#include <windows.h>
#include <iostream>
#include <tlhelp32.h>
#include <string>

// Поиск ID процесса без лишнего палева
DWORD GetTargetProcessId(const std::string& processName) {
    DWORD processId = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    
    if (hSnap != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 pEntry;
        pEntry.dwSize = sizeof(pEntry);
        
        if (Process32First(hSnap, &pEntry)) {
            do {
                if (processName.compare(pEntry.szExeFile) == 0) {
                    processId = pEntry.th32ProcessID;
                    break;
                }
            } while (Process32Next(hSnap, &pEntry));
        }
        CloseHandle(hSnap);
    }
    return processId;
}

int main() {
    // Ставим обычный гражданский заголовок окна
    SetConsoleTitleA("Data Linker Subsystem");

    std::string processName = "StandChillow.exe";
    std::string dllName = "kotofeywin.dll";
    char fullDllPath[MAX_PATH];

    std::cout << "[~] Looking for game process..." << std::endl;

    // Проверяем путь к нашей DLL-ке
    if (!GetFullPathNameA(dllName.c_str(), MAX_PATH, fullDllPath, nullptr)) {
        std::cout << "[-] Failed to get DLL path!" << std::endl;
        Sleep(3000);
        return 1;
    }

    // Ждем, пока приватка запустится
    DWORD pId = 0;
    while (pId == 0) {
        pId = GetTargetProcessId(processName);
        Sleep(500);
    }

    std::cout << "[+] Found game! Process ID: " << pId << std::endl;
    std::cout << "[~] Connecting to memory link..." << std::endl;

    // ВНИМАНИЕ: Запрашиваем только точечные права, которые винда разрешает без админки!
    // Убрали PROCESS_ALL_ACCESS, чтобы не триггерить ошибку 5
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
        std::cout << "[-] Пёс хуйню сморозил! Ошибка доступа: " << GetLastError() << std::endl;
        std::cout << "[!] Убедись, что игра и этот инжектор запущены от ОДНОГО пользователя." << std::endl;
        system("pause");
        return 1;
    }

    // Выделяем память под путь DLL
    void* allocatedMemory = VirtualAllocEx(hProcess, nullptr, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!allocatedMemory) {
        std::cout << "[-] VirtualAllocEx failed! Error: " << GetLastError() << std::endl;
        CloseHandle(hProcess);
        system("pause");
        return 1;
    }

    // Записываем путь
    if (!WriteProcessMemory(hProcess, allocatedMemory, fullDllPath, strlen(fullDllPath) + 1, nullptr)) {
        std::cout << "[-] WriteProcessMemory failed! Error: " << GetLastError() << std::endl;
        VirtualFreeEx(hProcess, allocatedMemory, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        system("pause");
        return 1;
    }

    // Берём адрес LoadLibraryA
    LPVOID loadLibraryAddress = (LPVOID)GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA");
    if (!loadLibraryAddress) {
        std::cout << "[-] Failed to get LoadLibraryA address!" << std::endl;
        VirtualFreeEx(hProcess, allocatedMemory, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        system("pause");
        return 1;
    }

    // Запускаем поток в игре без админских привилегий
    HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0, (LPTHREAD_START_ROUTINE)loadLibraryAddress, allocatedMemory, 0, nullptr);
    if (!hThread) {
        std::cout << "[-] CreateRemoteThread blocked! Error: " << GetLastError() << std::endl;
        VirtualFreeEx(hProcess, allocatedMemory, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        system("pause");
        return 1;
    }

    std::cout << "[+] Инжект прошёл успешно, петушара закрывается!" << std::endl;
    
    // Аккуратно чистим за собой
    WaitForSingleObject(hThread, INFINITE);
    VirtualFreeEx(hProcess, allocatedMemory, 0, MEM_RELEASE);
    CloseHandle(hThread);
    CloseHandle(hProcess);

    Sleep(2000);
    return 0;
}
