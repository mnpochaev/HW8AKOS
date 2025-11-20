#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/types.h>

#define SHM_NAME "/shm_numbers_example"

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

    fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        perror("shm_open");
        return 1;
    }

    if (ftruncate(fd, sizeof(struct shm_data)) == -1) {
        perror("ftruncate");
        close(fd);
        shm_unlink(SHM_NAME);
        return 1;
    }

    shm = mmap(NULL, sizeof(struct shm_data),
               PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shm == MAP_FAILED) {
        perror("mmap");
        close(fd);
        shm_unlink(SHM_NAME);
        return 1;
    }

    close(fd);
    g_shm = shm;

    shm->server_pid = getpid();
    shm->client_pid = 0;
    shm->number     = 0;
    shm->has_value  = 0;
    shm->terminate  = 0;

    printf("Server: started, PID = %d\n", (int)shm->server_pid);
    printf("Server: waiting for numbers...\n");

    while (!stop && !shm->terminate) {
        if (shm->has_value) {
            printf("Server: got number = %d\n", shm->number);
            fflush(stdout);
            shm->has_value = 0;
        }
        sleep(1);
    }

    if (!shm->terminate) {
        shm->terminate = 1;
        if (shm->client_pid > 0) {
            printf("Server: sending SIGINT to client (PID = %d)\n",
                   (int)shm->client_pid);
            kill(shm->client_pid, SIGINT);
        }
    }

    sleep(1);

    if (munmap(shm, sizeof(struct shm_data)) == -1) {
        perror("munmap");
    }

    if (shm_unlink(SHM_NAME) == -1) {
        perror("shm_unlink");
    } else {
        printf("Server: shared memory removed\n");
    }

    printf("Server: exiting\n");
    return 0;
}
