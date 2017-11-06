#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/mman.h>
#include <memory.h>
#include <sys/time.h>
#include <signal.h>

// ADD NECESSARY HEADERS
#define SHM_NAME "zhifeng_jcheng"
#define maxClients 63
// Mutex variables
pthread_mutex_t *mutex;

typedef struct {
    int pid;
    char birth[25];
    char clientString[10];
    int elapsed_sec;
    double elapsed_msec;
} stats_t;

int i;
stats_t *shm_ptr;

void exit_handler(int sig) {
    // ADD
    // critical section begins
    pthread_mutex_lock(mutex);

    // Client leaving; needs to reset its segment

    shm_ptr[i].pid = -1;

    pthread_mutex_unlock(mutex);

    exit(0);
}

int main(int argc, char *argv[]) {
    // ADD
    if (argc != 2) {
        printf("invalid command");
        exit(1);
    }
    if (strlen(argv[1]) > 10) {
        printf("ClientString length too long");
        exit(1);
    }

    //signal handling
    struct sigaction act;
    act.sa_handler = exit_handler;
    if (sigaction(SIGINT, &act, NULL) < 0) {
        perror("sigaction");
        return 1;
    }
    if (sigaction(SIGTERM, &act, NULL) < 0) {
        perror("sigaction");
        return 1;
    }


    int fd_shm = shm_open(SHM_NAME, O_RDWR, 0660);
    if (fd_shm == -1) {
        printf("shm_open failed\n");
        exit(1);
    }

    shm_ptr = (stats_t *) mmap(NULL, getpagesize(), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0);
    if (shm_ptr == MAP_FAILED) {
        printf("mmap failed\n");
        exit(1);
    }
    mutex = (pthread_mutex_t *) shm_ptr;

    // critical section begins
    pthread_mutex_lock(mutex);
    struct timeval timeBefore, timeAfter;
    int isSet = 0;

    // client updates available segment
    for (i = 1; i < maxClients + 1; i++) {
        if (shm_ptr[i].pid <= 0) {
            shm_ptr[i].pid = getpid();
            time_t rawtime;
            struct tm *timeinfo;
            time(&rawtime);
            timeinfo = localtime(&rawtime);
            if (timeinfo == NULL) {
                perror("localtime failed");
                pthread_mutex_unlock(mutex);
                exit(1);
            }
            strcpy(shm_ptr[i].birth, asctime(timeinfo));
            shm_ptr[i].birth[24] = '\0';
            strcpy(shm_ptr[i].clientString, argv[1]);
            shm_ptr[i].clientString[9] = '\0';
            gettimeofday(&timeBefore, NULL);
            isSet = 1;
            break;
        }
    }

    //if this client exceeds the range the server can handler, should exit.
    if (isSet == 0) {
        perror("Too many clients");
        pthread_mutex_unlock(mutex);
        exit(1);
    }

    pthread_mutex_unlock(mutex);
    // critical section ends
    while (1) {
        // ADD

        gettimeofday(&timeAfter, NULL);
        shm_ptr[i].elapsed_sec = (int) (timeAfter.tv_sec - timeBefore.tv_sec);
        shm_ptr[i].elapsed_msec = (timeAfter.tv_usec - timeBefore.tv_usec)/1000.0;

        sleep(1);

        // Print active clients
        printf("Active clients :");
        for (int j = 1; j < maxClients + 1; j++) {
            if (shm_ptr[j].pid > 0) {
                printf(" %i", shm_ptr[j].pid);
            }
        }
        printf("\n");


    }

    return 0;
}


