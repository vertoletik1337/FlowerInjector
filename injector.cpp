#include <windows.h>
#include <iostream>
#include <string>

int main() {
    // Настраиваем консоль
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);
    SetConsoleTitleA("kotofey.win t.me/pikmizs dsc.gg/kotofey t.me/whitelotosteams t.me/kotofeymods ❘ я ебал маму veIvet_pulls");

    std::string dllName = "kotofeywin.dll";
    char fullDllPath[MAX_PATH];

    std::cout << "[~] Ожидание запуска StandChillow..." << std::endl;

    // Проверяем путь к нашей DLL
    if (!GetFullPathNameA(dllName.c_str(), MAX_PATH, fullDllPath, nullptr)) {
        std::cout << "[-] Не удалось получить путь к DLL!" << std::endl;
        Sleep(3000);
        return 1;
    }

    HWND hWindow = nullptr;
    DWORD pId = 0;

    // Цикл крутится, пока игра реально не откроется
    while (!hWindow) {
        // Ищем окно игры. Если у приватки вверху написано "StandChillow" — найдет мгновенно.
        // Если имя окна другое (например, "Standoff 2"), заменим первый параметр на нужный.
        hWindow = FindWindowA(nullptr, "StandChillow");
        
        if (hWindow) {
            // Забираем реальный ID процесса прямо из этого окна
            GetWindowThreadProcessId(hWindow, &pId);
            if (pId == 0) {
                hWindow = nullptr; // Если ID кривой, ищем заново
            }
        }
        Sleep(500);
    }

    std::cout << "[+] Игра успешно обнаружена! Железный ID: " << pId << std::endl;
    std::cout << "[~] Подключение к памяти процесса..." << std::endl;

    // Открываем процесс с точечными правами
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

    // Выделяем память под путь к DLL
    void* allocatedMemory = VirtualAllocEx(hProcess, nullptr, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!allocatedMemory) {
        std::cout << "[-] Ошибка VirtualAllocEx: " << GetLastError() << std::endl;
        CloseHandle(hProcess);
        system("pause");
        return 1;
    }

    // Записываем путь
    if (!WriteProcessMemory(hProcess, allocatedMemory, fullDllPath, strlen(fullDllPath) + 1, nullptr)) {
        std::cout << "[-] Ошибка WriteProcessMemory: " << GetLastError() << std::endl;
        VirtualFreeEx(hProcess, allocatedMemory, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        system("pause");
        return 1;
    }

    // Вызываем загрузку DLL в игре
    LPVOID loadLibraryAddress = (LPVOID)GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA");
    HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0, (LPTHREAD_START_ROUTINE)loadLibraryAddress, allocatedMemory, 0, nullptr);
    
    if (!hThread) {
        std::cout << "[-] Ошибка CreateRemoteThread: " << GetLastError() << std::endl;
        VirtualFreeEx(hProcess, allocatedMemory, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        system("pause");
        return 1;
    }

    std::cout << "[+] Инжект пробит! Можешь разносить." << std::endl;
    
    WaitForSingleObject(hThread, INFINITE);
    VirtualFreeEx(hProcess, allocatedMemory, 0, MEM_RELEASE);
    CloseHandle(hThread);
    CloseHandle(hProcess);

    Sleep(2000);
    return 0;
}
