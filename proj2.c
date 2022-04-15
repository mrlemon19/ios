#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int inputCheck(int argc, char **argv)
{
    if (argc != 5){
        fprintf(stderr, "chyba vstupu: spatny pocet argumentu \n");
        return 1;
    }
    int argv3 = atoi(argv[3]);
    int argv4 = atoi(argv[4]);
    if (argv3 <= 0 || argv3 > 1000 || argv4 <= 0 || argv4 > 1000){
        fprintf(stderr, "chyba vstupu: spatny rozsah casu ... FIXME \n");
        return 1;
    }
    return 0;
}

int main(int argc, char **argv)
{
    inputCheck(argc, argv);

    

    return 0;
}
