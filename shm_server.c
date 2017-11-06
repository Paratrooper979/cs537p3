#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/mman.h>
#include <memory.h>
#include <sys/time.h>
#include <time.h>
#include <signal.h>
// ADD NECESSARY HEADERS
#define SHM_NAME "zhifeng_jcheng"
#define maxClients 63
// Mutex variables
pthread_mutex_t* mutex;
pthread_mutexattr_t mutexAttribute;


typedef struct {
    int pid;
    char birth[25];
    char clientString[10];
    int elapsed_sec;
    double elapsed_msec;
} stats_t;

void exit_handler(int sig)
{
    // ADD
    if (munmap(NULL, (size_t) getpagesize()) < 0) {
        perror("munmap failed");
        exit(1);
    }
    if (shm_unlink(SHM_NAME) < 0) {
        perror("shm_unlink failed");
        exit(1);
    }
    exit(0);
}

int main(int argc, char *argv[])
{
    // ADD
    //signal handling
    struct sigaction act;
    act.sa_handler = exit_handler;
    if (sigaction(SIGINT, &act, NULL) < 0) {
        perror ("sigaction");
        return 1;
    }

    if (sigaction(SIGTERM, &act, NULL) < 0) {
        perror ("sigaction");
        return 1;
    }

    // Creating a new shared memory segment
    int fd_shm = shm_open(SHM_NAME, O_RDWR | O_CREAT, 0660);
    if (fd_shm == -1) {
        perror("shm_open failed\n");
        exit(1);
    }
    if (ftruncate(fd_shm, getpagesize()) < 0) {
        perror("ftruncate failed\n");
        exit(1);
    }


    stats_t *shm_ptr = (stats_t *) mmap(NULL, getpagesize(), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0);
    close(fd_shm);
    if (shm_ptr == MAP_FAILED) {
        perror("mmap failed\n");
        exit(1);
    }


    // Initializing mutex
    mutex = (pthread_mutex_t *) shm_ptr;
    pthread_mutexattr_init(&mutexAttribute);
    pthread_mutexattr_setpshared(&mutexAttribute, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(mutex, &mutexAttribute);
    int counter = 0;

    while (1)
    {
        // ADD

        counter ++;
        for (int i = 0; i < maxClients; i++) {
            if (shm_ptr[i].pid > 0) {
                printf("%i, pid : %i, birth : %s, elapsed : %i s %f ms, %s\n", counter, shm_ptr[i].pid,
                       shm_ptr[i].birth, shm_ptr[i].elapsed_sec, shm_ptr[i].elapsed_msec, shm_ptr[i].clientString);
            }
        }

        sleep(1);
    }

    return 0;
}
