#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <string.h>

#define FILENAME "shared_mem.dat"

// Структура, которая будет лежать в общей памяти
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
        printf("\n--- Меню Сервера ---\n");
        printf("1 - Выполнить проецирование\n");
        printf("2 - Записать данные и ждать клиента\n");
        printf("0 - Завершить работу\n");
        printf("Выбор: ");
        if (scanf("%d", &choice) == 1){
            getchar(); 
            if (choice == 1) {
                // Создание файла (0666 - права на чтение и запись для всех)
                fd = open(FILENAME, O_RDWR | O_CREAT | O_TRUNC, 0666);
                if (fd < 0) { 
                    perror("\nОшибка создания файла"); 
                    continue; 
                }
    
                // Установка размер файла ДО проецирования
                if (ftruncate(fd, FILESIZE) == -1) {
                    perror("\nОшибка ftruncate");
                    close(fd);
                    continue;
                }
    
                // Проецирование в память MAP_SHARED
                shared_mem = mmap(NULL, FILESIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
                if (shared_mem == MAP_FAILED) { 
                    perror("\nОшибка mmap"); 
                    continue; 
                }
    
                shared_mem->state = 0; // Инициализируем флаг состояния
                printf("\n[+] Файл успешно спроецирован в память сервера.\n");
            }
            else if (choice == 2) {
                if (shared_mem == MAP_FAILED) { 
                    printf("\n[-] Сначала выполните проецирование!\n"); 
                    continue; 
                }
                
                printf("\nВведите сообщение для клиента: ");
                fgets(shared_mem->buffer, sizeof(shared_mem->buffer), stdin);
                // Проверка: влезла ли строка целиком (есть ли в конце \n)
                if (strchr(shared_mem->buffer, '\n') == NULL) {
                    // Если \n нет, значит в потоке еще остались лишние символы
                    int c;
                    while ((c = getchar()) != '\n' && c != EOF); // очиста лишних символов до конца строки
                } else shared_mem->buffer[strcspn(shared_mem->buffer, "\n")] = 0;
    
                shared_mem->state = 1; // Сигнал клиенту о готовности данных
                printf("[i] Данные записаны. Ожидание прочтения клиентом...\n");
                while (shared_mem->state != 2) {
                    usleep(100000); // чтобы не грузить процессор
                }
                printf("[+] Клиент успешно прочитал данные!\n");
    
                // Отмена проецирования и удаление файла
                munmap(shared_mem, FILESIZE);
                shared_mem = MAP_FAILED;
                close(fd);
                unlink(FILENAME);
                printf("[i] Проецирование отменено, файл удален.\n");
            }
            else if (choice == 0) {
                if (shared_mem != MAP_FAILED) {
                    munmap(shared_mem, FILESIZE);
                    close(fd);
                    unlink(FILENAME);
                }
                exit = 0;
            }
        }

    }
    return 0;
}