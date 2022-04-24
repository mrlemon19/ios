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

#define MMAP(pointer) {(pointer) = mmap(NULL, sizeof(*(pointer)), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);}
#define UNMAP(pointer) {munmap((pointer), sizeof((pointer)));}

// prekladat takto:     cc -std=gnu99 -Wall -Wextra -Werror -pedantic proj2.c -o proj2

//FILE *pfile;
int *hy = NULL;
int *ox = NULL;
int *printnum = 1;
sem_t *qvGen = NULL;
sem_t *qvHy = NULL;
sem_t *qvOx = NULL;


// zatim hlasi error


int init()
{
    //pfile = fopen("proj2.out", "w");
    MMAP(hy);
    MMAP(ox);
    MMAP(printnum);
    if ((qvGen = sem_open("/xlukas18.ios.proj2.sem.qvgen", O_CREAT | O_EXCL, 0666, 1)) == SEM_FAILED) return -1;
    else{return 0;}
    if ((qvHy = sem_open("/xlukas18.ios.proj2.sem.qvhy", O_CREAT | O_EXCL, 0666, 0)) == SEM_FAILED) return -1;
    else{return 0;}
    if ((qvOx = sem_open("/xlukas18.ios.proj2.sem.qvox", O_CREAT | O_EXCL, 0666, 0)) == SEM_FAILED) return -1;
    else{return 0;}
}
void clean()
{
    UNMAP(hy);
    UNMAP(ox);
    UNMAP(printnum);
    sem_close(qvGen);
    sem_unlink("/xlukas18.ios.proj2.sem.qvgen");
    sem_close(qvHy);
    sem_unlink("/xlukas18.ios.proj2.sem.qvhy");
    sem_close(qvOx);
    sem_unlink("/xlukas18.ios.proj2.sem.qvox");
    //if (pfile != NULL) fclose(pfile);
}


void raiseErr()
{
    fprintf(stderr, "chyba forku \n");
}

void processOx(int id, int dell1, int dell2)
{
    id++;
    // srandom pro sleeptime1 a sleeptime2
    // int sleeptime1 = (rand() % dell1 + 1);   // FIXME dedelat random generovani zpozdeni
    // int sleeptime2 = (rand() % dell2 + 1); // same ^^
    printf("O %d: started \n", id);
    usleep(dell1); // nefunguje jeste, predelat na sleeptime2
    sem_wait(qvGen); //defaultne na 1
    ox++;
    if (*hy >= 2) {
        sem_post(qvHy);
        sem_post(qvHy);
        hy -= 2;
        sem_post(qvOx);
        ox --;
        }
    else {sem_post(qvGen);}
    printf("O %d: going to queue \n", id);
    sem_wait(qvOx); //defaultne na 0
    printf("O %d: creating molecule [cislo molekuly] \n", id); // FIXME cislo molekuly
    usleep(dell2); // nefunguje jeste, predelat na sleeptime2
    printf("O %d: molecule [cislo melekuly] created \n", id);
    // qvOx.post()
    // qvGen.post()
    exit(0);
}

void processHy(int id, int dell1, int dell2)
{
    id++;
    //int sleeptime1 = (rand() % dell1 + 1);   // FIXME dedelat random generovani zpozdeni
    //int sleeptime2 = (rand() % dell2) // same ^^
    printf("H %d: started \n", id);
    usleep(dell1); // nefunguje jeste, predelat na sleeptime1
    // sdilena pamet: hydrogens++
    // if (hydrogen >= 2) {}
    // semH.wait() - je defaultne nastaven na 0
    printf("H %d: going to queue \n", id);
    printf("H %d: creating molecule [cislo molekuly] \n", id); // FIXME cislo molekuly
    // sdilena pamet: hydrogens--
    usleep(dell2); // nefunguje jeste, predelat na sleeptime2
    printf("H %d: molecule [cislo melekuly] created \n", id);
    // semH.post(2)
    exit(0);
}

void genO(int nox, int dell1, int dell2)
{
    for (int i = 0; i < nox; i++){
        pid_t ox = fork();
        if (ox == 0){
            printf("ox num. %d generating\n", i);
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
            printf("hy num. %d generating\n", i);
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
    
    // zatim nefunguje
    /*
    if (init() == -1){
        clean();
        return -1;
    }
    */
    
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
        pid_t wpid2;    // pro cekani procesu dokud nejsou vnorene procesy ukonceny

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
        while ((wpid2 = wait(&status)) > 0);    // cekani...
        
    }

    while ((wpid = wait(&status)) > 0);         // cekani...

    exit(0);
    //clean(); // uklizeci funkce
    return 0;
}