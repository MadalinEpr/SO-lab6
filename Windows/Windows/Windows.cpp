
#include <windows.h>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <string>

using namespace std;

const int MAX = 1000;

const char* NUME_MEMORIE = "Local\\MemoriePartajataSO";
const char* NUME_SEMAFOR = "Local\\SemaforSO";

int procesCopil()
{
    HANDLE memorie = OpenFileMappingA(
        FILE_MAP_ALL_ACCESS, FALSE, NUME_MEMORIE);

    HANDLE semafor = OpenSemaphoreA(
        SEMAPHORE_ALL_ACCESS, FALSE, NUME_SEMAFOR);

    if (memorie == NULL || semafor == NULL)
    {
        cout << "Eroare la deschiderea resurselor!\n";
        return 1;
    }

    int* valoare = (int*)MapViewOfFile(
        memorie, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(int));

    if (valoare == NULL)
    {
        cout << "Eroare la maparea memoriei!\n";
        CloseHandle(memorie);
        CloseHandle(semafor);
        return 1;
    }

    srand((unsigned int)(time(NULL) + GetCurrentProcessId()));

    while (true)
    {
        WaitForSingleObject(semafor, INFINITE);

        if (*valoare >= MAX)
        {
            ReleaseSemaphore(semafor, 1, NULL);
            break;
        }

        int rezultat = rand() % 2 + 1;

        while (rezultat == 2 && *valoare < MAX)
        {
            (*valoare)++;
            cout << "Procesul " << GetCurrentProcessId()
                << " a scris: " << *valoare << endl;

            rezultat = rand() % 2 + 1;
        }

        bool terminat = (*valoare >= MAX);

        ReleaseSemaphore(semafor, 1, NULL);

        if (terminat)
            break;

        Sleep(1);
    }

    UnmapViewOfFile(valoare);
    CloseHandle(memorie);
    CloseHandle(semafor);

    cout << "Procesul " << GetCurrentProcessId()
        << " s-a terminat.\n";

    return 0;
}

int main(int argc, char* argv[])
{
    // Modul copil
    if (argc > 1 && string(argv[1]) == "child")
        return procesCopil();

    // Cream memoria partajata
    HANDLE memorie = CreateFileMappingA(
        INVALID_HANDLE_VALUE,
        NULL,
        PAGE_READWRITE,
        0,
        sizeof(int),
        NUME_MEMORIE);

    if (memorie == NULL)
    {
        cout << "Eroare la crearea memoriei!\n";
        return 1;
    }

    int* valoare = (int*)MapViewOfFile(
        memorie, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(int));

    if (valoare == NULL)
    {
        cout << "Eroare la maparea memoriei!\n";
        CloseHandle(memorie);
        return 1;
    }

    *valoare = 0;

    // Cream semaforul cu valoarea initiala 1
    HANDLE semafor = CreateSemaphoreA(
        NULL, 1, 1, NUME_SEMAFOR);

    if (semafor == NULL)
    {
        cout << "Eroare la crearea semaforului!\n";
        UnmapViewOfFile(valoare);
        CloseHandle(memorie);
        return 1;
    }

    cout << "Memoria partajata a fost initializata.\n";
    cout << "Pornim cele doua procese...\n\n";

    // Pornim primul proces copil
    STARTUPINFOA si1 = { sizeof(STARTUPINFOA) };
    PROCESS_INFORMATION pi1 = {};

    string comanda1 = "\"" + string(argv[0]) + "\" child";

    if (!CreateProcessA(
        NULL, &comanda1[0], NULL, NULL, FALSE,
        0, NULL, NULL, &si1, &pi1))
    {
        cout << "Eroare la pornirea procesului 1!\n";
        return 1;
    }

    // Pornim al doilea proces copil
    STARTUPINFOA si2 = { sizeof(STARTUPINFOA) };
    PROCESS_INFORMATION pi2 = {};

    string comanda2 = "\"" + string(argv[0]) + "\" child";

    if (!CreateProcessA(
        NULL, &comanda2[0], NULL, NULL, FALSE,
        0, NULL, NULL, &si2, &pi2))
    {
        cout << "Eroare la pornirea procesului 2!\n";
        return 1;
    }

    // Asteptam terminarea ambelor procese
    HANDLE procese[] = { pi1.hProcess, pi2.hProcess };

    WaitForMultipleObjects(2, procese, TRUE, INFINITE);

    cout << "\nAmbele procese s-au terminat.\n";
    cout << "Valoarea finala din memoria partajata: "
        << *valoare << endl;

    CloseHandle(pi1.hThread);
    CloseHandle(pi1.hProcess);
    CloseHandle(pi2.hThread);
    CloseHandle(pi2.hProcess);

    UnmapViewOfFile(valoare);
    CloseHandle(semafor);
    CloseHandle(memorie);

    return 0;
}
