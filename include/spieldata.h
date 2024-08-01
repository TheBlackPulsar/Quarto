#ifndef __quarto_spieldaten_h__
#define __quarto_spieldaten_h__

#include <sys/types.h>
#include <sys/wait.h>
#include "bibs.h"

typedef struct{
    char Spielername[20]; //*Spielername
    int Spielernummer;
    int Spieleranzahl;
    pid_t t_pid;
    pid_t c_pid;
} spielerdaten; 


typedef struct{
    char Gegnername[20]; //*Gegnername;
    int Gegnernummer;
    int Flag;
} gegnerdaten; 


typedef struct
{
    bool is_ready;
    unsigned long time_limit;
    int next;
    unsigned long width;
    unsigned long height;
    // spaces allocated in advance
    unsigned long capacity;
    /* field must be in the last member */
    int *field;
    /* data */
} spielfeld;

bool spielfeld_update();


#endif // __quarto_socket_h__



