#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/sem.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/stat.h>
#include <time.h>

#define MMAP(pointer) {(pointer) = mmap(NULL, sizeof(*(pointer)), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);}
#define UNMAP(pointer) {munmap((pointer), sizeof((pointer)));}

//int *sharedmem = NULL;
//FILE *pfile;
//sem_t *semafor = NULL;

// zatim hlasi error
/*
int init()
{
    pfile = fopen("proj2.out", "w");
    MMAP(sharedmem);
    if ((semafor = sem_open("/xlukas18.ios.proj2.sem", O_CREAT | O_EXCL, 0666, 1)) == SEM_FAILED) return -1;
    else{
        return 0;
    }

}

void clean()
{
    UNMAP(sharedmem);
    sem_close(semafor);
    sem_unlink("/xlukas18.ios.proj2.sem");
    if (pfile != NULL) fclose(pfile);
}
*/

int getRandom(int max)
{
    int num = (rand() % max + 1);
    return num;
}

void raiseErr()
{
    fprintf(stderr, "chyba forku \n");
}

void processOx(int id, int dell)
{
    id++;
    srand(time(0));
    int sleeptime = getRandom(dell);   // FIXME dodelat random generovani zpozdeni
    usleep(sleeptime);
    printf("process Ox no. %d with dellay: %d \n", id, sleeptime);
    exit(0);
}

void processHy(int id, int dell)
{
    id++;
    srand(time(0));
    int sleeptime = getRandom(dell);   // FIXME dedelat random generovani zpozdeni
    usleep(sleeptime);
    printf("process Hy no. %d with dellay: %d \n", id, sleeptime);
    exit(0);
}

void genO(int nox, int dell)
{
    for (int i = 0; i < nox; i++){
        pid_t ox = fork();
        if (ox == 0){
            processOx(i, dell);
        }
    }
    exit(0);
}

void genH(int nhy, int dell)
{
    for (int i = 0; i < nhy; i++){
        pid_t hy = fork();
        if (hy == 0){
            processHy(i, dell);
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
    printf("program started \n");
    // kontrola vstupu
    if (argc != 5){
        fprintf(stderr, "chyba vstupu: spatny pocet argumentu \n");
        return 1;
    }

 // checks if input data are in right format
    if (inputSort(argv)) return 1;
    
    // zatim nefunguje
    /*
    if (init() == -1){
        clean();
        return -1;
    }
    */
    
    int NH = atoi(argv[2]);
    int NO = atoi(argv[1]);
    int delay = atoi(argv[3]);

    pid_t hlavni = fork();
    if (hlavni < 0)
    {
        raiseErr();
    }
    else if (hlavni == 0)
    {
        // child process
        genO(NO, delay);
    }
    else 
    {
        // main process
        genH(NH, delay);
    }

    exit(0);
    //clean(); // uklizeci funkce
    return 0;
}