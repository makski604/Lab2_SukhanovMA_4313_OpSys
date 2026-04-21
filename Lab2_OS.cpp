#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <limits>
#include <iomanip>

void Error(int type);
void MenuOutput();
std::wstring GetStateString(DWORD state);
std::wstring GetProtectString(DWORD protect);
std::wstring GetTypeString(DWORD type);
bool SafeWrite(uintptr_t addr, int data);

int main()
{
    setlocale(LC_ALL, "");

    char choice;
    int subChoice;
    std::vector<void*> myAllocations;
    SIZE_T pgsize = 4096;

    do {
        MenuOutput();
        std::cin >> choice;

        switch (choice) {
        case '1': {
            // 1. Получение информации о вычислительной системе
            SYSTEM_INFO sysInfo;
            GetSystemInfo(&sysInfo);

            std::wcout << L"\n--- Информация о системе ---\n";
            std::wcout << L"Архитектура процессора: " << sysInfo.wProcessorArchitecture << L"\n";
            std::wcout << L"Размер страницы памяти: " << sysInfo.dwPageSize << L" байт\n";
            std::wcout << L"Мин. адрес приложения: 0x" << std::hex << std::uppercase << sysInfo.lpMinimumApplicationAddress << std::dec << L"\n";
            std::wcout << L"Макс. адрес приложения: 0x" << std::hex << std::uppercase << sysInfo.lpMaximumApplicationAddress << std::dec << L"\n";
            std::wcout << L"Количество процессоров: " << sysInfo.dwNumberOfProcessors << L"\n";
            std::wcout << L"Гранулярность выделения памяти: " << sysInfo.dwAllocationGranularity << L" байт\n";

            std::wcout << L"\n"; system("pause");
            break;
        }

        case '2': {
            // 2. Определение статуса виртуальной памяти
            MEMORYSTATUS memStat;
            memStat.dwLength = sizeof(MEMORYSTATUS);
            GlobalMemoryStatus(&memStat);

            const double GB = 1024.0 * 1024.0 * 1024.0;
            const double TB = GB * 1024.0;

            std::wcout << L"\n--- Статус виртуальной памяти ---\n";
            std::wcout << L"Загрузка памяти: " << memStat.dwMemoryLoad << L"%\n";
            std::wcout << std::fixed << std::setprecision(2); // Установка 2-ух знаков после запятой
            std::wcout << L"Всего физической памяти: " << memStat.dwTotalPhys / GB << L" ГБ\n";
            std::wcout << L"Свободно физической памяти: " << memStat.dwAvailPhys / GB << L" ГБ\n";
            std::wcout << L"Всего в файле подкачки: " << memStat.dwTotalPageFile / GB << L" ГБ\n";
            std::wcout << L"Свободно в файле подкачки: " << memStat.dwAvailPageFile / GB << L" ГБ\n";
            std::wcout << L"Всего виртуальной памяти: " << memStat.dwTotalVirtual / TB << L" ТБ\n";
            std::wcout << L"Свободно виртуальной памяти: " << memStat.dwAvailVirtual / TB << L" ТБ\n";

            std::wcout << L"\n"; system("pause");
            break;
        }

        case '3': {
            // 3. Определение состояния участка памяти
            uintptr_t inputAddr;
            std::wcout << L"\nВведите адрес для проверки (в шестнадцатеричном формате, например 7FF6A1): ";
            std::wcin >> std::hex >> inputAddr >> std::dec;

            if (std::wcin.fail()) {
                std::wcin.clear();
                std::wcin.ignore((std::numeric_limits<std::streamsize>::max)(), L'\n');
                std::wcout << L"\nОшибка ввода адреса!\n";
                Sleep(1000);
            }
            else {
                MEMORY_BASIC_INFORMATION mbi;
                if (VirtualQuery(reinterpret_cast<LPCVOID>(inputAddr), &mbi, sizeof(mbi))) {
                    std::wcout << L"\n--- Состояние памяти по адресу 0x" << std::hex << std::uppercase << inputAddr << std::dec << L" ---\n";
                    std::wcout << L"Базовый адрес региона: 0x" << std::hex << mbi.BaseAddress << std::dec << L"\n";
                    std::wcout << L"Базовый адрес выделения: 0x" << std::hex << mbi.AllocationBase << std::dec << L"\n";
                    std::wcout << L"Размер региона: " << mbi.RegionSize << L" байт\n";
                    std::wcout << L"Состояние (State): " << GetStateString(mbi.State) << L"\n";
                    std::wcout << L"Защита (Protect): " << GetProtectString(mbi.Protect) << L"\n";
                    std::wcout << L"Тип (Type): " << GetTypeString(mbi.Type) << L"\n";
                }
                else Error(1);
                std::wcout << L"\n"; system("pause");
            }
            break;
        }

        case '4': {
            // 4. Раздельное резервирование и передача памяти
            std::wcout << L"\n1 - Автоматический режим (адрес выберет ОС)\n2 - Ввод адреса вручную\nВаш выбор: ";
            std::wcin >> subChoice;

            LPVOID targetAddr = NULL;
            if (subChoice == 2) {
                uintptr_t manualAddr;
                std::wcout << L"Введите адрес начала региона (hex): ";
                std::wcin >> std::hex >> manualAddr >> std::dec;
                targetAddr = reinterpret_cast<LPVOID>(manualAddr);
            }

            // Резервирование
            LPVOID reservedAddr = VirtualAlloc(targetAddr, pgsize, MEM_RESERVE, PAGE_READWRITE);
            if (!reservedAddr) {
                std::wcout << L"\nОшибка при резервировании!\n";
                Error(2);
            }
            else {
                std::wcout << L"\n[+] Память успешно ЗАРЕЗЕРВИРОВАНА по адресу: 0x" << std::hex << reservedAddr << std::dec << L"\n";
                Sleep(1500);

                // Передача физической памяти
                LPVOID committedAddr = VirtualAlloc(reservedAddr, pgsize, MEM_COMMIT, PAGE_READWRITE);
                if (!committedAddr) {
                    std::wcout << L"\nОшибка при передаче физической памяти!\n";
                    Error(2);
                    VirtualFree(reservedAddr, 0, MEM_RELEASE); // Откат резервирования
                }
                else {
                    std::wcout << L"[+] Физическая память успешно ПЕРЕДАНА (Commit) по адресу: 0x" << std::hex << committedAddr << std::dec << L"\n";
                    myAllocations.push_back(committedAddr);
                }
                std::wcout << L"\n"; system("pause");
            }
            break;
        }

        case '5': {
            // 5. Одновременное резервирование и передача памяти
            std::wcout << L"\n1 - Автоматический режим\n2 - Ввод адреса вручную\nВаш выбор: ";
            std::wcin >> subChoice;

            LPVOID targetAddr = NULL;
            if (subChoice == 2) {
                uintptr_t manualAddr;
                std::wcout << L"Введите адрес начала региона (hex): ";
                std::wcin >> std::hex >> manualAddr >> std::dec;
                targetAddr = reinterpret_cast<LPVOID>(manualAddr);
            }

            // Одновременный RESERVE и COMMIT
            LPVOID allocatedAddr = VirtualAlloc(targetAddr, pgsize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            if (!allocatedAddr) {
                std::wcout << L"\nОшибка при одновременном выделении памяти!\n";
                Error(2);
            }
            else {
                std::wcout << L"\n[+] Память одновременно зарезервирована и передана по адресу: 0x" << std::hex << allocatedAddr << std::dec << L"\n";
                myAllocations.push_back(allocatedAddr);
            }
            std::wcout << L"\n"; system("pause");
            break;
        }

        case '6': {
            // 6. Запись данных по заданному адресу
            if (myAllocations.empty()) {
                std::wcout << L"\nСначала выделите память (пункты 4 или 5)!\n";
                Sleep(1500);
            }
            else {
                uintptr_t inputAddr;
                std::wcout << L"\nВведите адрес для записи (hex) из выделенных вами: ";
                std::wcin >> std::hex >> inputAddr >> std::dec;

                std::wcout << L"Введите число для записи: ";
                int data;
                std::wcin >> data;

                if (SafeWrite(inputAddr, data)) std::wcout << L"Запись успешна!\n";
                else std::wcout << L"Ошибка доступа!\n";
                std::wcout << L"\n"; system("pause");
            }
            break;
        }

        case '7': {
            // 7. Установка и проверка защиты доступа
            uintptr_t inputAddr;
            std::wcout << L"\nВведите адрес для изменения защиты (hex): ";
            std::wcin >> std::hex >> inputAddr >> std::dec;

            DWORD oldProtect;
            // Установка защиты PAGE_READONLY
            if (VirtualProtect(reinterpret_cast<LPVOID>(inputAddr), pgsize, PAGE_READONLY, &oldProtect)) {
                std::wcout << L"\n[+] Защита PAGE_READONLY успешно установлена.\n";
                std::wcout << L"Старое значение защиты: " << GetProtectString(oldProtect) << L"\n";

                std::wcout << L"\nПроверка защиты: попытка записи числа 999 по этому адресу...\n";
                if (SafeWrite(inputAddr, 999)) std::wcout << L"Запись удалась, защита не сработала...\n";
                else std::wcout << L"Исключение перехвачено! Защита памяти работает: запись запрещена.\n";
                
                DWORD temp;
                VirtualProtect(reinterpret_cast<LPVOID>(inputAddr), pgsize, oldProtect, &temp);
                std::wcout << L"\n[i] Исходные права доступа восстановлены.\n";
            }
            else Error(3);
            std::wcout << L"\n"; system("pause");
            break;
        }
        }
    } while (choice != '0');

    for (void* ptr : myAllocations) VirtualFree(ptr, 0, MEM_RELEASE);

    return 0;
}

void MenuOutput() {
    system("cls"); 
    std::wcout << L"\n--- Лабораторная работа №2. Управление памятью ---\n";
    std::wcout << L"1 - Информация о вычислительной системе\n";
    std::wcout << L"2 - Статус виртуальной памяти\n";
    std::wcout << L"3 - Состояние участка памяти (VirtualQuery)\n";
    std::wcout << L"4 - Раздельное резервирование и передача памяти\n";
    std::wcout << L"5 - Одновременное резервирование и передача памяти\n";
    std::wcout << L"6 - Запись данных в память\n";
    std::wcout << L"7 - Установка защиты доступа и ее проверка\n";
    std::wcout << L"0 - Выход\n";
    std::wcout << L"\nВыберите пункт: ";
}

std::wstring GetStateString(DWORD state) {
    if (state == MEM_COMMIT) return L"MEM_COMMIT (Физическая память выделена)";
    if (state == MEM_FREE) return L"MEM_FREE (Свободно)";
    if (state == MEM_RESERVE) return L"MEM_RESERVE (Зарезервировано)";
    return L"Неизвестно";
}

std::wstring GetProtectString(DWORD protect) {
    DWORD p = protect & 0xFF; // сбрасываю флаги-модификаторы, чтобы они не мешали сравнению
    if (p == PAGE_NOACCESS) return L"PAGE_NOACCESS";
    if (p == PAGE_READONLY) return L"PAGE_READONLY";
    if (p == PAGE_READWRITE) return L"PAGE_READWRITE";
    if (p == PAGE_WRITECOPY) return L"PAGE_WRITECOPY";
    if (p == PAGE_EXECUTE) return L"PAGE_EXECUTE";
    if (p == PAGE_EXECUTE_READ) return L"PAGE_EXECUTE_READ";
    if (p == PAGE_EXECUTE_READWRITE) return L"PAGE_EXECUTE_READWRITE";
    if (p == PAGE_EXECUTE_WRITECOPY) return L"PAGE_EXECUTE_WRITECOPY";
    return L"Неизвестно / Нет доступа";
}

std::wstring GetTypeString(DWORD type) {
    if (type == MEM_IMAGE) return L"MEM_IMAGE (Спроецированный исполняемый файл)";
    if (type == MEM_MAPPED) return L"MEM_MAPPED (Спроецированный файл)";
    if (type == MEM_PRIVATE) return L"MEM_PRIVATE (Приватная память процесса)";
    return L"Неизвестно / Свободный участок";
}

bool SafeWrite(uintptr_t addr, int data) {
    __try {
        int* ptr = reinterpret_cast<int*>(addr);
        *ptr = data;
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

void Error(int type) {
    DWORD error = GetLastError();
    switch (type) {
    case 1: std::wcout << L"\nОшибка VirtualQuery. Код: " << error << std::endl; break;
    case 2: std::wcout << L"\nОшибка VirtualAlloc. Код: " << error << std::endl; break;
    case 3: std::wcout << L"\nОшибка VirtualProtect. Код: " << error << std::endl; break;
    default: std::wcout << L"\nНеизвестная ошибка. Код: " << error << std::endl; break;
    }
}