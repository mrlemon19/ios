#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/sem.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <limits.h>
#include <time.h>

#define MMAP(pointer) {(pointer) = mmap(NULL, sizeof(*(pointer)), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);}
#define UNMAP(pointer) {munmap((pointer), sizeof((pointer)));}

// prekladat takto:     cc -std=gnu99 -pthread -Wall -Wextra -Werror -pedantic proj2.c -o proj2

FILE *pfile;
int *hy = NULL;
int *ox = NULL;
int *printnum = NULL;
int *molnum = NULL;
int *count = NULL;
sem_t *mutex = NULL;
sem_t *qvHy = NULL;
sem_t *qvOx = NULL;
sem_t *printGrd = NULL;
sem_t *barrier1 = NULL;
sem_t *barrier2 = NULL;
sem_t *barMutex = NULL;


// zatim hlasi error


int init()
{
    pfile = fopen("proj2.out", "w");
    MMAP(hy);
    MMAP(ox);
    MMAP(printnum);
    MMAP(molnum);
    MMAP(count);
    *printnum = 1;
    *molnum = 1;
    *hy = 0;
    *ox = 0;
    *count = 0;
    if ((mutex = sem_open("/xlukas18.sem.mutex", O_CREAT, 0666, 1)) == SEM_FAILED) return -1;
    else if ((qvHy = sem_open("/xlukas18.sem.qvhy", O_CREAT, 0666, 0)) == SEM_FAILED) return -1;
    else if ((qvOx = sem_open("/xlukas18.sem.qvox", O_CREAT, 0666, 0)) == SEM_FAILED) return -1;
    else if ((printGrd = sem_open("/xlukas18.sem.printgrd", O_CREAT, 0666, 1)) == SEM_FAILED) return -1;
    else if ((barrier1 = sem_open("/xlukas18.sem.barrier1", O_CREAT, 0666, 0)) == SEM_FAILED) return -1;
    else if ((barrier2 = sem_open("/xlukas18.sem.barrier2", O_CREAT, 0666, 0)) == SEM_FAILED) return -1;
    else if ((barMutex = sem_open("/xlukas18.sem.barmutex", O_CREAT, 0666, 1)) == SEM_FAILED) return -1;
    else return 0;
}
void clean()
{
    UNMAP(hy);
    UNMAP(ox);
    UNMAP(printnum);
    UNMAP(molnum);
    UNMAP(count);
    sem_close(mutex);
    sem_unlink("/xlukas18.sem.mutex");
    sem_close(qvHy);
    sem_unlink("/xlukas18.sem.qvhy");
    sem_close(qvOx);
    sem_unlink("/xlukas18.sem.qvox");
    sem_close(printGrd);
    sem_unlink("/xlukas18.sem.printgrd");
    sem_close(barrier1);
    sem_unlink("/xlukas18.sem.barrier1");
    sem_close(barrier2);
    sem_unlink("/xlukas18.sem.barrier2");
    sem_close(barMutex);
    sem_unlink("/xlukas18.sem.barmutex");
    if (pfile != NULL) fclose(pfile);
}


void raiseErr()
{
    fprintf(stderr, "chyba forku \n");
}

void processOx(int id, int dell1, int dell2)
{
    id++;
    srand(id * time(0));
    int sleeptime1 = (rand() % dell1 + 1);
    int sleeptime2 = (rand() % dell2 + 1);
    
    sem_wait(printGrd);
        fprintf(pfile, "%d: O %d: started\n",*printnum, id);
        fflush(pfile);
        (*printnum)++;
    sem_post(printGrd);
    
    usleep(sleeptime1); 
    
    sem_wait(mutex);    // defaultne na 1
    (*ox)++;
    if (*hy >= 2) {
        sem_post(qvHy);
        sem_post(qvHy);
        (*hy) -= 2;
        sem_post(qvOx);
        (*ox)--;
        }
    else sem_post(mutex);

    sem_wait(printGrd);
        fprintf(pfile, "%d: O %d: going to queue\n",*printnum, id);
        fflush(pfile);
        (*printnum)++;
    sem_post(printGrd);

    sem_wait(qvOx); //defaultne na 0

    sem_wait(printGrd);     // vypise, ze tvori molekulu
        fprintf(pfile, "%d: O %d: creating molecule %d\n",*printnum, id, *molnum);
        fflush(pfile);
        (*printnum)++;
    sem_post(printGrd);

    sem_wait(barMutex);
    (*count)++;    // pocet procesu tvoricich molekulu
    if (*count == 3){
        sem_post(barrier1);
        sem_post(barrier1);
        sem_post(barrier1);
    }
    else sem_post(barMutex);

    sem_wait(barrier1);

    usleep(sleeptime2);

    sem_wait(printGrd);
        fprintf(pfile, "%d: O %d: molecule %d created\n",*printnum, id, *molnum);
        fflush(pfile);
        (*printnum)++;
    sem_post(printGrd);

    (*count)--;
    if (*count == 0){
        (*molnum)++;
        sem_post(barrier2);
        sem_post(barrier2);
        sem_post(barrier2);
        sem_post(barMutex);
        sem_post(mutex);    
    }
    sem_wait(barrier2);
    exit(0);
}

void processHy(int id, int dell1, int dell2)
{
    id++;
    srand(id * time(0));
    int sleeptime1 = (rand() % dell1 + 1);
    int sleeptime2 = (rand() % dell2 + 1);

    sem_wait(printGrd);
        fprintf(pfile, "%d: H %d: started\n",*printnum, id);
        fflush(pfile);
        (*printnum)++;
    sem_post(printGrd);
    usleep(sleeptime1);

    sem_wait(mutex);
    (*hy)++;      // hy++
    if (*hy >= 2 && *ox >= 1) {
        sem_post(qvHy);
        sem_post(qvHy);
        (*hy) -= 2;
        sem_post(qvOx);
        (*ox)--;
        }
    else sem_post(mutex);

    sem_wait(printGrd);
        fprintf(pfile, "%d: H %d: going to queue\n",*printnum, id);
        fflush(pfile);
        (*printnum)++;
    sem_post(printGrd);

    sem_wait(qvHy);

    sem_wait(printGrd);
        fprintf(pfile, "%d: H %d: creating molecule %d\n",*printnum, id, *molnum);
        fflush(pfile);
        (*printnum)++;
    sem_post(printGrd);

    // barrier in some sort
    sem_wait(barMutex);
    
    (*count)++;    // pocet procesu tvoricich molekulu
    if (*count == 3){
        sem_post(barrier1);
        sem_post(barrier1);
        sem_post(barrier1);
    }
    else sem_post(barMutex);

    sem_wait(barrier1);

    usleep(sleeptime2);

    sem_wait(printGrd);
        fprintf(pfile, "%d: H %d: molecule %d created\n",*printnum, id, *molnum);
        fflush(pfile);
        (*printnum)++;
    sem_post(printGrd);

    (*count)--;
    if (*count == 0){
        (*molnum)++;
        sem_post(barrier2);
        sem_post(barrier2);
        sem_post(barrier2);
        sem_post(barMutex);
        sem_post(mutex);    
    }

    sem_wait(barrier2);

    exit(0);
}

void genO(int nox, int dell1, int dell2)
{
    for (int i = 0; i < nox; i++){
        pid_t ox = fork();
        if (ox == 0){
            processOx(i, dell1, dell2);
        }
    }
    exit(0);
}

void genH(int nhy, int dell1, int dell2)
{
    for (int i = 0; i < nhy; i++){
        pid_t hy = fork();
        if (hy == 0){
            processHy(i, dell1, dell2);
        }
    }
    exit(0);
}

int inputSort(char **argv)
{
    int argv1 = atoi(argv[1]);
    int argv2 = atoi(argv[2]);
    int argv3 = atoi(argv[3]);
    int argv4 = atoi(argv[4]);

    if (argv1 < 0 || argv2 < 0){
        fprintf(stderr, "spatny format dat \n");
        return 1;
    }
    else if (argv3 <= 0 || argv3 > 1000 || argv4 <= 0 || argv4 > 1000){
        fprintf(stderr, "chyba vstupu: spatny rozsah casu \n");
        return 1;
    }
    else{
        return 0;
    }
}

int main(int argc, char **argv)
{
    // kontrola vstupu
    if (argc != 5){
        fprintf(stderr, "chyba vstupu: spatny pocet argumentu \n");
        return 1;
    }

 // checks if input data are in right format
    if (inputSort(argv)) return 1;
    
    if (init() == -1){
        clean();
        return -1;
    }
    
    int NH = atoi(argv[2]);
    int NO = atoi(argv[1]);
    int delay1 = atoi(argv[3]);
    int delay2 = atoi(argv[4]);

    pid_t hlavni = fork();
    
    pid_t wpid;         // pro cekani hlavniho procesu na ukonceni vedlejsich
    int status = 0;     // same ^^^

    if (hlavni < 0)     // kontrola erroru forku
    {
        raiseErr();
    }
    else if (hlavni == 0)
    {
        pid_t OH = fork();
        //pid_t wpid2;    // pro cekani procesu dokud nejsou vnorene procesy ukonceny

        if (OH < 0)     // kontrola chyby forku
        {
            raiseErr();
        }
        if (OH == 0)    // child proces 1 generuje kyslik
        {
            genO(NO, delay1, delay2);
        }
        else{
            genH(NH, delay1, delay2);   // child proces 2 generuje vodik
        }
        //while ((wpid2 = wait(&status)) > 0);    // cekani...
        
    }

    while ((wpid = wait(&status)) > 0);         // cekani...

    exit(0);
    clean(); // uklizeci funkce
    return 0;
}