#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <string.h>

#define FILENAME "shared_mem.dat"

typedef struct {
    int state; // 0 - пусто, 1 - сервер записал, 2 - клиент прочитал
    char buffer[256];
} SharedMemory;

#define FILESIZE sizeof(SharedMemory)

int main() {
    int fd = -1;
    SharedMemory *shared_mem = MAP_FAILED;
    int choice;

    int exit = 1;
    while(exit) {
        printf("\n--- Меню Клиента ---\n");
        printf("1 - Выполнить проецирование\n");
        printf("2 - Прочитать данные\n");
        printf("0 - Завершить работу\n");
        printf("Выбор: ");
        if (scanf("%d", &choice) == 1){
            getchar(); 
            if (choice == 1) {
                fd = open(FILENAME, O_RDWR);
                if (fd < 0) { 
                    perror("\nОшибка открытия файла (возможно, сервер его еще не создал)"); 
                    continue; 
                }
    
                shared_mem = mmap(NULL, FILESIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
                if (shared_mem == MAP_FAILED) { 
                    perror("\nОшибка mmap"); 
                    continue; 
                }
                printf("\n[+] Файл успешно спроецирован в память клиента.\n");
            }
            else if (choice == 2) {
                if (shared_mem == MAP_FAILED) { 
                    printf("\n[-] Сначала выполните проецирование!\n"); 
                    continue; 
                }
                printf("\n[i] Ожидание данных от сервера...\n");
    
                // Ожидание записи данных сервером
                while (shared_mem->state != 1) {
                    usleep(100000);
                }
                printf("\n==================================\n");
                printf("Получены данные: %s\n", shared_mem->buffer);
                printf("==================================\n");
                shared_mem->state = 2; // Сигнал серверу о прочтении 
            }
            else if (choice == 0) {
                if (shared_mem != MAP_FAILED) {
                    munmap(shared_mem, FILESIZE);
                    close(fd);
                }
                exit = 0;
            }
        }
    }
    return 0;
}