#pragma once
#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <mmsystem.h> // Библиотека для высокоточного таймера timeGetTime()

#pragma comment(lib, "winmm.lib")

// Константы проекта
const int PAGE_COUNT = 16; 
const int MIN_DELAY = 500;
const int MAX_DELAY = 1500;
const int ITERATIONS = 15;

// Объекты для межпроцессного взаимодействия
const char* SHM_NAME = "Local\\SharedBuffer_Lab4"; // Объект проекции файла
const char* BACKING_FILE = "buffer_storage.bin";   // Проецируемй файл
const char* MUTEX_PREFIX = "Local\\PageMutex_";     // Префикс для мьютексов
const char* SEM_PREFIX = "Local\\PageWriteSem_";   // Префикс для семафоров

// Структура заголовка в начале разделяемой памяти
struct ControlHeader {
    int readers_on_page[PAGE_COUNT];
};

// Функция инициализации разделяемой памяти, проецируемой на файл
void* MapSharedFile(HANDLE& hFile, HANDLE& hMap, DWORD& size) {
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    
    // Вычисляем размер
    size = sizeof(ControlHeader) + (PAGE_COUNT * si.dwPageSize);

    // Создание/открытие файла на диске
    hFile = CreateFileA(BACKING_FILE, GENERIC_READ | GENERIC_WRITE, 
                        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, 
                        OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == INVALID_HANDLE_VALUE) return nullptr;

    // Проецирование файла файла
    hMap = CreateFileMappingA(hFile, NULL, PAGE_READWRITE, 0, size, SHM_NAME);
    if (!hMap) return nullptr;

    // Отображение проекции в адресное пространство текущего процесса
    void* pBuf = MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, size);
    
    // Фиксация страниц в физической памяти
    if (pBuf) {
        if (!VirtualLock(pBuf, size)) {
            std::cerr << "VirtualLock failed. Error: " << GetLastError() << "\n";
        }
    }

    return pBuf;
}

// Запись события в CSV
void LogCsv(std::ofstream& log, int state, int page) {
    log << timeGetTime() << "," << state << "," << page << "\n";
}

// Задержка
void RandomSleep() {
    Sleep(MIN_DELAY + rand() % (MAX_DELAY - MIN_DELAY + 1));
}