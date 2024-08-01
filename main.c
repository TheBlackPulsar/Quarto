#include <signal.h>

#include "bibs.h"
#include "socket.h"
#include "performConnection.h"
#include "spieldata.h"
#include "handle_rounds.h"
#include "thinker.h"
#include "defs.h"
#include "config.h"
   
static char* HOSTNAME;
static char* GAMEKINDNAME;
static int PORTNUMMER;


int main(int argc, char *argv[]){
    
    //GetOpt kommandozeile einlesen
    char *gameID[14];
    char *config[20];
    memset(gameID, 0, sizeof(gameID));
    memset(config, 0, sizeof(config));
    int opt;
    
    while((opt = getopt(argc, argv, ":g:c:p:")) != -1){
        
        switch (opt){
            case 'g':
                *gameID = optarg;
                printf("GameID Input: %s\n", *gameID);
                break;
            case 'c':
                *config = optarg;
                printf("Konfiguration Input: %s\n", *config);
                break;
            case 'p':
                printf("Spielernummer Input: %s\n", *config);
                break;
            default: 
                //printf("default case\n");
                printf("Bitte GameID und Konfigurationsdatei mit angeben\n(./sysprak-client -g <<GAME ID>> -c <<client.conf>>)\n");
                return 1;
                break;
        }  
    }


    config_vector* cv = read_config(*config);
    HOSTNAME = config_vector_find(cv, "HOSTNAME")->value;
    GAMEKINDNAME = config_vector_find(cv, "GAMEKINDNAME")->value;
    printf("GAMEKINDNAME in main: %s\n", GAMEKINDNAME);
    PORTNUMMER = atoi(config_vector_find(cv, "PORTNUMMER")->value);
   
    
    //Filedeskritpor von Sockets 
    int sock_stream = createSocket(HOSTNAME, PORTNUMMER);
    if(sock_stream < 0){
        return 1;
    }

 
    //spielerdaten & gegnerdaten
    int shm_id_spielerdaten = -1;
    int shm_id_gegnerdaten = -1;
    spielerdaten *shm_p;
    gegnerdaten *shm_g;
    //SHM getten
    if ( (shm_id_spielerdaten = shmget(IPC_PRIVATE, sizeof(spielerdaten), FLAG|IPC_CREAT)) < 0 )
    {
        perror("spielerdaten shmget() failed:\n");
        return -1;
    }
    if ( (shm_id_gegnerdaten = shmget(IPC_PRIVATE, maxanzahl* (sizeof(gegnerdaten)) -1, FLAG|IPC_CREAT)) < 0 )
    {
        perror("gegnerdaten shmget() failed:\n");
        return -1;
    }
    //SHM attachen
    if ((shm_p = (spielerdaten *) shmat (shm_id_spielerdaten,NULL,0)) == NULL ){
        perror("shmat() failed:\n");
        return -1;
    }
    if ((shm_g = (gegnerdaten *) shmat (shm_id_gegnerdaten,NULL,0)) == NULL ){
        perror("shmat() failed:\n");
        return -1;
    } 


    
    //ShaMo fürs Spielfeld anlegen
    int shm_id_spielfeld = -1;
    spielfeld *shm_sf;
    if ( (shm_id_spielfeld = shmget(IPC_PRIVATE, sizeof(spielfeld) + SPIELFELD_CAPCITY * sizeof(int), FLAG|IPC_CREAT)) < 0 )
    {
        perror("shmget() failed:\n");
        return -1;
    }

    if ((shm_sf= (spielfeld *) shmat (shm_id_spielfeld,NULL,0)) == NULL ){
        perror("shmat() failed:\n");
        return -1;
    }
    shm_sf->capacity = SPIELFELD_CAPCITY;
    shm_sf->field = (int*) (shm_sf + sizeof(spielfeld));

    //Pipe erstellen
    int fd[2]; 
    if (pipe(fd) == -1) {
        printf("Die Pipe konnte nicht erzeugt werden.\n");
        return -1;
    }  

    //PROZESS AUFSPALTEN
    pid_t pid = fork();
    //Fehlerbehandlung
    if (pid < 0) {
        perror ("Fehler bei fork().");
        exit(EXIT_FAILURE);
    }

    //Aufgabe Connector (CHILD)
    if (pid == 0) { 
        
        shm_p->c_pid = pid;
        
        //Schreibseite der Pipe schliessen
        close(fd[1]); 
       
        if ( (performConnect(sock_stream, *gameID, shm_p, shm_g, GAMEKINDNAME)) < 0){
            printf("Programm wird vorzeitig beendet\n");
            return -1;
        }

        handle_rounds(sock_stream, shm_sf, fd[0]);
        
       
        
    }

    //Aufgabe Thinker (PARENT)
    else { 
       
        shm_p->t_pid = pid;
        //Leseseite der Pipe schliessen
        close(fd[0]); 

        think(shm_sf, fd[1]);

        wait(0);
    }   

    //Shared Memories clearen
    shmctl(shm_id_gegnerdaten,IPC_RMID,0);
    shmctl(shm_id_spielerdaten,IPC_RMID,0);
    shmctl(shm_id_spielfeld,IPC_RMID,0);
    config_vector_clear(cv);

    return 0;
}