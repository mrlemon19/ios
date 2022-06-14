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

// project number 2 to operating systems seminar 2021/22
// author: Jakub Lukas, xlukas18
//
// execute program in this format: ./proj2 NO NH TI TB
// When executed, program checks input data and allocates needed resources
// Then creates NO processes of oxygen and NH processes of hydrogen
// Those processes gather in line and when two hydrogen processes and one oxygen process are waiting in the line
// they are released and form a molecule of water (H2O)
// After there are no more processes in line to create a molecule, not enough O or H message is printed
// allocated memory and all semaphores are freed then

// creating shared memory pointers and semaphores
FILE *pfile;
int *hy = NULL;
int *ox = NULL;
int *printnum = NULL;
int *molnum = NULL;
int *count = NULL;
int *allox = NULL;
int *allhy = NULL;
int *noten = NULL;
sem_t *mutex = NULL;
sem_t *qvHy = NULL;
sem_t *qvOx = NULL;
sem_t *printGrd = NULL;
sem_t *barrier1 = NULL;
sem_t *barrier2 = NULL;
sem_t *barMutex = NULL;
sem_t *semsleep = NULL;

// mapping shared memory with MMAP and openning semaphores
int init()
{
    pfile = fopen("proj2.out", "w");
    MMAP(hy);
    MMAP(ox);
    MMAP(printnum);
    MMAP(molnum);
    MMAP(count);
    MMAP(allox);
    MMAP(allhy);
    MMAP(noten);
    *printnum = 1;
    *molnum = 1;
    *hy = 0;
    *ox = 0;
    *count = 0;
    *allox = 0;
    *allhy = 0;
    *noten = 0;
    if ((mutex = sem_open("/xlukas18.sem.mutex", O_CREAT, 0666, 1)) == SEM_FAILED) return 1;
    else if ((qvHy = sem_open("/xlukas18.sem.qvhy", O_CREAT, 0666, 0)) == SEM_FAILED) return 1;
    else if ((qvOx = sem_open("/xlukas18.sem.qvox", O_CREAT, 0666, 0)) == SEM_FAILED) return 1;
    else if ((printGrd = sem_open("/xlukas18.sem.printgrd", O_CREAT, 0666, 1)) == SEM_FAILED) return 1;
    else if ((barrier1 = sem_open("/xlukas18.sem.barrier1", O_CREAT, 0666, 0)) == SEM_FAILED) return 1;
    else if ((barrier2 = sem_open("/xlukas18.sem.barrier2", O_CREAT, 0666, 0)) == SEM_FAILED) return 1;
    else if ((barMutex = sem_open("/xlukas18.sem.barmutex", O_CREAT, 0666, 1)) == SEM_FAILED) return 1;
    else if ((semsleep = sem_open("/xlukas18.sem.semsleep", O_CREAT, 0666, 0)) == SEM_FAILED) return 1;
    else return 0;
}

// cleanning function, unmaps all shared memory, closes and unlinks all semaphores
void clean()
{
    UNMAP(hy);
    UNMAP(ox);
    UNMAP(printnum);
    UNMAP(molnum);
    UNMAP(count);
    UNMAP(allox);
    UNMAP(allhy);
    UNMAP(noten);
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
    sem_close(semsleep);
    sem_unlink("/xlukas18.sem.semsleep");
    if (pfile != NULL) fclose(pfile);
}

// oxygen process
void processOx(int id, int dell1, int dell2)
{
    id++;   // unique oxygen process id
    srand(id * time(0));    // sets random seed with time
    int sleeptime1;
    int sleeptime2;
    
    // sets random sleeptime with process id
    if (dell1 != 0){
        sleeptime1 = (rand() % dell1 + 1);
    }
    else{
        sleeptime1 = 0; 
    }
    if (dell2 != 0){
        sleeptime2 = (rand() % dell2 + 1);
    }
    else{
        sleeptime2 = 0;
    }
    
    // printGrd semaphore protects output, so there is only one process printing on it
    sem_wait(printGrd);
        fprintf(pfile, "%d: O %d: started\n",*printnum, id);    // prints that process started
        fflush(pfile);
        (*printnum)++;  // number of line in printed output
    sem_post(printGrd);
    
    usleep(sleeptime1); // delay of molecule creating time
    
    sem_wait(mutex);       // mutex has value of 1 at start, so at least one process comes through
    // procrss is going to ox queue

    sem_wait(printGrd);
        (*ox)++;        // global number of ox procesess that are waiting in oxqv
        (*allox)--;     // number of ox processes created
        fprintf(pfile, "%d: O %d: going to queue\n",*printnum, id);
        fflush(pfile);
        (*printnum)++;
    sem_post(printGrd);

    // checking if there is enough processes to form a molecule
    // if there is not, prints not enough O
    if ((*allhy == 0 && *hy < 2) || (*allhy <2 && *hy == 0)){
        sem_wait(printGrd);
            fprintf(pfile, "%d: O %d: not enough H\n",*printnum, id);
            fflush(pfile);
            (*printnum)++;
        sem_post(printGrd);

        sem_post(mutex);
        exit(0);
    }

    // bond
    // if there are at least 2 hy processes and 1 ox in queues, processes are freed from queues and send to barriere
    if (*hy >= 2) {
        sem_post(qvHy);
        sem_post(qvHy);
        (*hy) -= 2;
        sem_post(qvOx);
        (*ox)--;
        }
    else sem_post(mutex);

    sem_wait(qvOx); // semaphore symulating the oxygen queue

    // checking if there is enough processes to continue
    // when signal flag noten is up (there is no way a molecule can be formed), all processes in queues are freed
    // "not enough" message is printed 
    if (*noten > 0){
        sem_wait(printGrd);
            fprintf(pfile, "%d: O %d: not enough H\n",*printnum, id);
            fflush(pfile);
            (*printnum)++;
            (*ox)--;
        sem_post(printGrd);
        if (*ox > 0){
            sem_post(qvOx);
        }
        exit(0);
    }

    // barrier
    sem_wait(barMutex);     // lets processes in barrier one by one, 1 on default

    (*count)++;    // number of processes inside barrier
    if (*count == 3){   // just 3 processes passes barrier1 so processes forming a molecule are together and synchronised
        sem_post(barrier1);
        sem_post(barrier1);
        sem_post(barrier1);
    }
    else sem_post(barMutex);

    sem_wait(barrier1);

    sem_wait(printGrd);     // creating molecule
        fprintf(pfile, "%d: O %d: creating molecule %d\n",*printnum, id, *molnum);
        fflush(pfile);
        (*printnum)++;
    sem_post(printGrd);

    usleep(sleeptime2);     // delay symulating creating of the molecule
    sem_post(semsleep);     // after dellay 2 hy processes are freed form semsleep semaphore
    sem_post(semsleep);

    sem_wait(printGrd);     // molecule is created
        fprintf(pfile, "%d: O %d: molecule %d created\n",*printnum, id, *molnum);
        fflush(pfile);
        (*printnum)++;
    sem_post(printGrd);

    (*count)--;
    if (*count == 0){   // all processes are simultaneously freed from barrier2 after finished creating molenuce
        (*molnum)++;    // number of molecule
        // checking if another molecule can be formed, if not sets noten flag to 1
        if (((*allox + *ox) < 1) || ((*allhy + *hy) < 2)){
            (*noten)++;

            if (*hy > 0){
                sem_post(qvHy);
            }
            if (*ox > 0){
                sem_post(qvOx);
            }
        }

        sem_post(barrier2);
        sem_post(barrier2);
        sem_post(barrier2);
        sem_post(barMutex);
        sem_post(mutex);    
    }
    sem_wait(barrier2);

    exit(0);
}

// hydrogen process
void processHy(int id, int dell1, int dell2)
{
    id++;   // unique process number
    int sleeptime1;
    int sleeptime2;
    srand(id * time(0));    // sets seed according to time

    // sest random delay time according to id
    if (dell1 != 0){
        sleeptime1 = (rand() % dell1 + 1);
    }
    else{
        sleeptime1 = 0; 
    }
    if (dell2 != 0){
        sleeptime2 = (rand() % dell2 + 1);
    }
    else{
        sleeptime2 = 0;
    }

    fprintf(pfile, "%d: H %d: started\n",*printnum, id); // process started
        fflush(pfile);
        (*printnum)++;
    sem_post(printGrd);
    usleep(sleeptime1);

    sem_wait(mutex);

    sem_wait(printGrd); // process is going to hydro queue
        (*hy)++;        // number of hy processes waiting in hyQv
        (*allhy)--;     // number of created hy processes
        fprintf(pfile, "%d: H %d: going to queue\n",*printnum, id);
        fflush(pfile);
        (*printnum)++;
    sem_post(printGrd);

    // checks if it's possible to create a molecule
    if ((*allox == 0 && *ox == 0) || (*hy <2 && *allhy == 0)){
        sem_wait(printGrd);
            fprintf(pfile, "%d: H %d: not enough O or H\n",*printnum, id);
            fflush(pfile);
            (*printnum)++;
        sem_post(printGrd);

        sem_post(mutex);
        exit(0);
    }

    // bond
    // if there is enough processes in queues to bond a molecule, those processes are freed and go to barrier
    if (*hy >= 2 && *ox >= 1) {
        sem_post(qvHy);
        sem_post(qvHy);
        (*hy) -= 2;
        sem_post(qvOx);
        (*ox)--;
        }
    else sem_post(mutex);

    sem_wait(qvHy); // hydro queue

    // checks if there is enough processes to form a molecule by noten flag
    // in case of shortage of needed processes all processes are freed from queues and "not enough" message is printed
    if (*noten > 0){
        sem_wait(printGrd);
            fprintf(pfile, "%d: H %d: not enough O or H\n",*printnum, id);
            fflush(pfile);
            (*printnum)++;
            (*hy)--;
        sem_post(printGrd);
        if (*hy > 0){
            sem_post(qvHy);
        }
        exit(0);
    }

    // barrier
    sem_wait(barMutex); // lets one process at time inside barrier
    
    (*count)++;
    if (*count == 3){
        sem_post(barrier1);
        sem_post(barrier1);
        sem_post(barrier1);
    }
    else sem_post(barMutex);

    sem_wait(barrier1); // all 3 processes to create a molecule are let in barrier at same time

    sem_wait(printGrd); // creating molecule
        fprintf(pfile, "%d: H %d: creating molecule %d\n",*printnum, id, *molnum);
        fflush(pfile);
        (*printnum)++;
    sem_post(printGrd);

    (void) sleeptime2;
    sem_wait(semsleep);

    sem_wait(printGrd); // moelcule created
        fprintf(pfile, "%d: H %d: molecule %d created\n",*printnum, id, *molnum);
        fflush(pfile);
        (*printnum)++;
    sem_post(printGrd);

    (*count)--;
    if (*count == 0){
        (*molnum)++;

        // checking if another molecule can be formed, if not sets noten flag to 1
        if (((*allox + *ox) < 1) || ((*allhy + *hy) < 2)){
            (*noten)++;

            if (*hy > 0){
                sem_post(qvHy);
            }
            if (*ox > 0){
                sem_post(qvOx);
            }
        }

        sem_post(barrier2);
        sem_post(barrier2);
        sem_post(barrier2);
        sem_post(barMutex);
        sem_post(mutex);    
    }

    sem_wait(barrier2); // after all 3 processes are waiting on 2. barrier, they are freed at one time

    exit(0);
}

// creates NO oxygen processes with fork
void genO(int no, int dell1, int dell2)
{
    int nox = no;       // number of all oxygen processes
    for (int i = 0; i < nox; i++){
        pid_t ox = fork();
        if (ox == 0){
            processOx(i, dell1, dell2);
        }
    }
    exit(0);
}

// creates NH hydrogen processes with fork
void genH(int nh, int dell1, int dell2)
{
    int nhy = nh;       // number of all hydrogen processes
    for (int i = 0; i < nhy; i++){
        pid_t hy = fork();
        if (hy == 0){
            processHy(i, dell1, dell2);
        }
    }
    exit(0);
}

// checks validity of input data
int inputSort(char **argv)
{
    int argv1 = atoi(argv[1]);
    int argv2 = atoi(argv[2]);
    int argv3 = atoi(argv[3]);
    int argv4 = atoi(argv[4]);

    // checks number of ox and hy on input
    if (argv1 < 0 || argv2 < 0){
        fprintf(stderr, "spatny format dat \n");
        return 1;
    }
    // checks if time is in correct range
    else if (argv3 < 0 || argv3 > 1000 || argv4 < 0 || argv4 > 1000){
        fprintf(stderr, "chyba vstupu: spatny rozsah casu \n");
        return 1;
    }
    else{
        return 0;
    }
}

int main(int argc, char **argv)
{
    // number of arguments check
    if (argc != 5){
        fprintf(stderr, "chyba vstupu: spatny pocet argumentu \n");
        exit (1);
    }

    // input check
    if (inputSort(argv)) exit (1);
    
    // shared memory and semaphore init
    if (init() == 1){
        fprintf(stderr, "error: chyba pri vytvareni semaforu\n");
        clean();
        exit (1);
    }
    
    *allox = atoi(argv[1]);
    *allhy = atoi(argv[2]);
    int delay1 = atoi(argv[3]);
    int delay2 = atoi(argv[4]);

    pid_t hlavni = fork();
    
    pid_t wpid;         // for main process to wait till child processes are dead
    int status = 0;

    if (hlavni < 0)     // fork error check
    {
        fprintf(stderr, "fork error \n");
    }
    else if (hlavni == 0)
    {
        pid_t OH = fork();

        if (OH < 0)     // fork error check
        {
            fprintf(stderr, "fork error \n");
        }
        if (OH == 0)    // child process 1 generates oxygen
        {
            genO(*allox, delay1, delay2);
        }
        else{
            genH(*allhy, delay1, delay2);   // child process 2 generates hydrogen
        }        
    }

    while ((wpid = wait(&status)) > 0);         // waiting for child processes to end

    exit(0);
    clean(); // cleaning function
    return 0;
}