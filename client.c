#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/types.h>
#include <time.h>

#define SHM_NAME "/shm_numbers_example"

#define MIN_VALUE 0
#define MAX_VALUE 100

struct shm_data {
    pid_t server_pid;
    pid_t client_pid;
    int  number;
    int  has_value;
    int  terminate;
};

int stop = 0;
struct shm_data *g_shm = NULL;

void sig_handler(int sig) {
    (void)sig;
    stop = 1;
}

int main(void) {
    int fd;
    struct shm_data *shm;

    signal(SIGINT,  sig_handler);
    signal(SIGTERM, sig_handler);

    fd = shm_open(SHM_NAME, O_RDWR, 0);
    if (fd == -1) {
        perror("shm_open");
        fprintf(stderr, "Client: probably server is not started.\n");
        return 1;
    }

    shm = mmap(NULL, sizeof(struct shm_data),
               PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shm == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }

    close(fd);
    g_shm = shm;

    shm->client_pid = getpid();

    printf("Client: started, PID = %d\n", (int)shm->client_pid);
    printf("Client: generating numbers in range [%d, %d]\n",
           MIN_VALUE, MAX_VALUE);

    srand((unsigned int)(time(NULL) ^ getpid()));

    while (!stop && !shm->terminate) {
        if (!shm->has_value) {
            int value = MIN_VALUE + rand() % (MAX_VALUE - MIN_VALUE + 1);
            shm->number = value;
            shm->has_value = 1;

            sleep(1);
        } else {
            sleep(1);
        }
    }

    if (!shm->terminate) {
        shm->terminate = 1;
        if (shm->server_pid > 0) {
            printf("Client: sending SIGINT to server (PID = %d)\n",
                   (int)shm->server_pid);
            kill(shm->server_pid, SIGINT);
        }
    }

    sleep(1);

    if (munmap(shm, sizeof(struct shm_data)) == -1) {
        perror("munmap");
    }

    printf("Client: exiting\n");
    return 0;
}
