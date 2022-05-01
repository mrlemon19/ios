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
// projekt cislo 2 do predmetu IOS
// vypracoval: Jakub Lukas, xlukas18
//
// Program se spousti ve formatu: ./proj2 NO NH TI TB
// Program hned po spusteni zkontroluje vstupni data a alokuje potrebne zdroje.
// Nasledne vytvori NH procesu vodiku a NO procesu kysliku.
// Tyto procesy se postupne radi do fronty, ze ktere jsou nasledne uvolnovany vzdy dva vodiky a jeden kyslik a vytvari molekuly vody (H2O).
// Jakmile proces nema dostatek dalsich ingredienci vypise not enough O or H.
// Program pote, uvolni alokovane zdrojea a konci s kodem exit 0.

// inicializace spolecnych promenych a semaforu
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

// mapovani spolecne pameti a semaforu
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

// ukizeci funkce, odmapovava spolecnou pamet a odstranuje semafory
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

// vypise error
void raiseErr()
{
    fprintf(stderr, "chyba forku \n");
}

// proces kysliku
void processOx(int id, int dell1, int dell2)
{
    id++;   // unikatni cislo procesu
    srand(id * time(0));    // nastavy seed pro random podle casu a id procesu
    int sleeptime1;
    int sleeptime2;
    
    // nastavy nahodny cas zpozdeni
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
    
    // printGrd je semafor nastaveny na 0 zajistuje, ze na vystup v jednu chvili zapisoval jen jeden proces 
    sem_wait(printGrd);
        fprintf(pfile, "%d: O %d: started\n",*printnum, id);    // zapise ze proces zacal
        fflush(pfile);
        (*printnum)++;  // printnum je cislo radku na ktery porces zapisuje
    sem_post(printGrd);
    
    usleep(sleeptime1); // zpozdeni 1
    
    sem_wait(mutex);       // ze zacatku na 1 aby pustil alespon jeden proces

    sem_wait(printGrd);
        (*ox)++;        // ox je cislo nactenych procesu kyslku cekajicich ve fronte qvOx
        (*allox)--;     // allox je cislo nenactenych procesu kysliku
        fprintf(pfile, "%d: O %d: going to queue\n",*printnum, id); // vypise ze proces jde do rady
        fflush(pfile);
        (*printnum)++;
    sem_post(printGrd);

    // kontrola nedostatku komponent pro vytvoreni molekuly
    // kdyz proces nema dost vodiku na vytvoreni molekuly vypise: not enough H
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
    // kdyz je ve frontach dostatek procesu na vytvoreni molekyly, propusti tyto procesy do tvorby molekuly
    if (*hy >= 2) {
        sem_post(qvHy);
        sem_post(qvHy);
        (*hy) -= 2;
        sem_post(qvOx);
        (*ox)--;
        }
    else sem_post(mutex);

    sem_wait(qvOx); // defaultne na 0, fronta na kysliky

    // sekundarni kontrola nedostatku zdroju na vytvoreni molekuly
    // kdyz je nedostatek nalezen (to urcuje prmena noten), propusti se cekajici procesy skrz frontu a ukonci se
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
    sem_wait(barMutex);     // defaultne na 1 aby pustil alespon jeden proces, potom postupne po jednom pousti procesy na tvorbu molekuly

    (*count)++;    // pocet procesu tvoricich molekulu
    if (*count == 3){   // propusti procesy, jakmile cekaji vsechny na bariere 1
        sem_post(barrier1);
        sem_post(barrier1);
        sem_post(barrier1);
    }
    else sem_post(barMutex);

    sem_wait(barrier1);

    sem_wait(printGrd);     // vypise, ze tvori molekulu
        fprintf(pfile, "%d: O %d: creating molecule %d\n",*printnum, id, *molnum);
        fflush(pfile);
        (*printnum)++;
    sem_post(printGrd);

    usleep(sleeptime2);     // zpozdeni vytvareni molekuly
    sem_post(semsleep);
    sem_post(semsleep);

    sem_wait(printGrd);     // vypise molekula vytvorena
        fprintf(pfile, "%d: O %d: molecule %d created\n",*printnum, id, *molnum);
        fflush(pfile);
        (*printnum)++;
    sem_post(printGrd);

    (*count)--;
    if (*count == 0){   // vsechny 3 procesy dorazi na barieru 2 a jsou zaroven propusteny 
        (*molnum)++;    // pro vypis, cislo molekuly
        // kontrola zda je dost ingradienci pro vytvoreni dalsi molekuly, kdyz ne uvolni prebytecne procesy fronty a nastavy parametr noten na >0
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

// proces vodiku
void processHy(int id, int dell1, int dell2)
{
    id++;   // unikatni cislo procesu
    int sleeptime1;
    int sleeptime2;
    srand(id * time(0));    // nastavy seed pro random podle casu a id procesu
    // nastavy nahodny cas zpozdeni
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

    fprintf(pfile, "%d: H %d: started\n",*printnum, id); // vypise ze proces zacal
        fflush(pfile);
        (*printnum)++;
    sem_post(printGrd);
    usleep(sleeptime1);

    sem_wait(mutex);

    sem_wait(printGrd); // vypise ze jde do fronty na vodiky
        (*hy)++;
        (*allhy)--;
        fprintf(pfile, "%d: H %d: going to queue\n",*printnum, id);
        fflush(pfile);
        (*printnum)++;
    sem_post(printGrd);

    // kontrola dostatku dalsich procesu pro tvorbu molekuly
    if ((*allox == 0 && *ox == 0) || (*hy <2 && *allhy == 0)){
        sem_wait(printGrd);
            fprintf(pfile, "%d: H %d: not enough O or H\n",*printnum, id);
            fflush(pfile);
            (*printnum)++;
        sem_post(printGrd);

        sem_post(mutex);
        exit(0);
    }

    // samotna tvorba molekuly
    // ksyz je na frontach dost procesy vosiku a kysliku propusti je pres fronty qvOx a qvHy
    if (*hy >= 2 && *ox >= 1) {
        sem_post(qvHy);
        sem_post(qvHy);
        (*hy) -= 2;
        sem_post(qvOx);
        (*ox)--;
        }
    else sem_post(mutex);

    sem_wait(qvHy); // fornta na procesy vodiku

    // kontrola nedostatku dalsich procesu pro tvorbu molekuly
    // kdyz je nedostatek, proces vypise not enough hlasku a ukonci se
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
    sem_wait(barMutex); // vstup do bariery propousti procesy po jednom na tvorbu molekuly
    
    (*count)++;
    if (*count == 3){
        sem_post(barrier1);
        sem_post(barrier1);
        sem_post(barrier1);
    }
    else sem_post(barMutex);

    sem_wait(barrier1); // kdyz jsou vsechny 3 procesy na bariere 1, jsou pusteny

    sem_wait(printGrd); // vypise se vytvareni molekuly
        fprintf(pfile, "%d: H %d: creating molecule %d\n",*printnum, id, *molnum);
        fflush(pfile);
        (*printnum)++;
    sem_post(printGrd);

    //usleep(sleeptime2);
    (void) sleeptime2;
    sem_wait(semsleep);

    sem_wait(printGrd); // vypisuje ze molekula byla vytvorena
        fprintf(pfile, "%d: H %d: molecule %d created\n",*printnum, id, *molnum);
        fflush(pfile);
        (*printnum)++;
    sem_post(printGrd);

    (*count)--;
    if (*count == 0){
        (*molnum)++;

        // kontrola dostatku ingredienci pro tvorbu dalsi molekuly
        // pri nedostatku je parametr noten nastaven na >0 a procesy navic jsou propusteny pres fronty
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

    sem_wait(barrier2); // pote co vsechny procesy vypisou molecule created jsou propusteny pres druhou barieru

    exit(0);
}

// vytvori zadany pocet procesu kysliku pomoci volani fork
void genO(int no, int dell1, int dell2)
{
    int nox = no;       // celkovy pocet procesu kysliku
    for (int i = 0; i < nox; i++){
        pid_t ox = fork();
        if (ox == 0){
            processOx(i, dell1, dell2);
        }
    }
    exit(0);
}

// vytvori zadany pocet procesu vodiku pomoci volani fork
void genH(int nh, int dell1, int dell2)
{
    int nhy = nh;       // celkovy pocet procesu vodiku
    for (int i = 0; i < nhy; i++){
        pid_t hy = fork();
        if (hy == 0){
            processHy(i, dell1, dell2);
        }
    }
    exit(0);
}

// vytridi a zkontroluje zadana cisla 
int inputSort(char **argv)
{
    int argv1 = atoi(argv[1]);
    int argv2 = atoi(argv[2]);
    int argv3 = atoi(argv[3]);
    int argv4 = atoi(argv[4]);

    // kontrola poctu vodiku a kysliku
    if (argv1 < 0 || argv2 < 0){
        fprintf(stderr, "spatny format dat \n");
        return 1;
    }
    // kontrola zda je zpozdeni zadano ve spravnem intervalu
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
    // kontrola vstupu
    if (argc != 5){
        fprintf(stderr, "chyba vstupu: spatny pocet argumentu \n");
        exit (1);
    }

    // kontroka vstupu
    if (inputSort(argv)) exit (1);
    
    // vytvareni semaforu a mapovani spolecnych promenych
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
    
    pid_t wpid;         // pro cekani hlavniho procesu na ukonceni vedlejsich
    int status = 0;

    if (hlavni < 0)     // kontrola selhani forku
    {
        raiseErr();
    }
    else if (hlavni == 0)
    {
        pid_t OH = fork();

        if (OH < 0)     // kontrola chyby forku
        {
            raiseErr();
        }
        if (OH == 0)    // child proces 1 generuje kyslik
        {
            genO(*allox, delay1, delay2);
        }
        else{
            genH(*allhy, delay1, delay2);   // child proces 2 generuje vodik
        }        
    }

    while ((wpid = wait(&status)) > 0);         // cekani nez se ukonci vedlejsi procesy

    exit(0);
    clean(); // odmapuje spolecne promene a odstrani semafory
    return 0;
}