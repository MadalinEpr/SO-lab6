
#include <iostream>
#include <string>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <unistd.h>
#include <cstdlib>
#include <ctime>
#include <cstdio>

using namespace std;

const char* SHM_NAME = "/memorie_numarare";
const char* SEM_NAME = "/semafor_numarare";

const int MAX = 1000;

int main(int argc, char* argv[]) {

    // Initializarea memoriei si a semaforului
    if (argc > 1 && string(argv[1]) == "--init") {

        shm_unlink(SHM_NAME);
        sem_unlink(SEM_NAME);

        int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);

        if (fd == -1) {
            perror("Eroare shm_open");
            return 1;
        }

        if (ftruncate(fd, sizeof(int)) == -1) {
            perror("Eroare ftruncate");
            return 1;
        }

        int* memorie = (int*)mmap(
            NULL, sizeof(int),
            PROT_READ | PROT_WRITE,
            MAP_SHARED, fd, 0
        );

        if (memorie == MAP_FAILED) {
            perror("Eroare mmap");
            return 1;
        }

        *memorie = 0;

        sem_t* sem = sem_open(SEM_NAME, O_CREAT | O_EXCL, 0666, 1);

        if (sem == SEM_FAILED) {
            perror("Eroare semafor");
            return 1;
        }

        cout << "Memoria si semaforul au fost initializate."
             << endl;

        sem_close(sem);
        munmap(memorie, sizeof(int));
        close(fd);

        return 0;
    }

    if (argc < 2) {
        cout << "Utilizare: ./main 1 sau ./main 2" << endl;
        return 1;
    }

    int id = atoi(argv[1]);

    int fd = shm_open(SHM_NAME, O_RDWR, 0666);

    if (fd == -1) {
        perror("Eroare shm_open");
        return 1;
    }

    int* memorie = (int*)mmap(
        NULL, sizeof(int),
        PROT_READ | PROT_WRITE,
        MAP_SHARED, fd, 0
    );

    if (memorie == MAP_FAILED) {
        perror("Eroare mmap");
        return 1;
    }

    sem_t* sem = sem_open(SEM_NAME, 0);

    if (sem == SEM_FAILED) {
        perror("Eroare sem_open");
        return 1;
    }

    srand(time(NULL) ^ getpid());

    cout << "Procesul " << id << " a pornit." << endl;

    while (true) {

        // Rezervam semaforul
        sem_wait(sem);

        // Citim valoarea din memoria partajata
        int valoare = *memorie;

        cout << "Procesul " << id
             << " a citit: " << valoare << endl;

        // Aruncam moneda: 1 sau 2
        while (valoare < MAX && (rand() % 2 + 1) == 2) {

            valoare++;

            // Scriem urmatorul numar
            *memorie = valoare;

            cout << "Procesul " << id
                 << " a scris: " << valoare << endl;
        }

        // Eliberam semaforul
        sem_post(sem);

        if (valoare >= MAX)
            break;

        usleep(10000);
    }

    cout << "Procesul " << id
         << " s-a terminat." << endl;

    sem_close(sem);
    munmap(memorie, sizeof(int));
    close(fd);

    return 0;
}

