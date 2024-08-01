#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <regex.h>
#include <assert.h>

#include "defs.h"
#include "bibs.h"
#include "performConnection.h"
#include "spieldata.h"
#include "handle_rounds.h"


int getServerMessage(int socket_stream, char *buffer, int charanzahl, ssize_t size){

    int i= 0;
    char *var = malloc(sizeof(char));
    
    
    memset(buffer, 0, sizeof(*buffer));

    if ( (size = recv(socket_stream, var, 1, 0)) == -1 ) return -1;

    buffer[i] = *var;

    for(i= 1; i < charanzahl; i++){
        size = recv(socket_stream, var, 1, 0);
        buffer[i] = *var;
        if (buffer[i] == '\n') break;
    }

    i++;
    for (int j = i; j<BUF; j++){
        buffer[j] = '\0';
    }

    
    free(var);
    return 0;
}


int formatierteAusgabeTai(char *buffer, int messageNumber, spielerdaten *shm_p, gegnerdaten *shm_g, char* GAMEKINDNAME){
    //messageNumber fuer jede moegliche Server antwort

    //Server Nachricht in einzelne Worte trennen, auf die wir zugreifen koennen:
    char words[20][20]; 
  
    //strtok extrahiert strings bis zu einem bestimmten char in einem token
    char * token = strtok(buffer, " "); //ACHTUNG: AM MANCHE WORTE HABEN \n AM ENDE
    int token_anzahl = 0;
    while( token != NULL ) {
        //printf( " %s\n", token ); //print alle token
        strcpy(words[token_anzahl], token);
        token_anzahl++;
        token = strtok(NULL, " ");
    }

    //Server: - TIMEOUT Be faster next time
    switch (messageNumber){//verschiedene messageNumbers für jeden Fall
    case 1: 
        //S: + MNM Gameserver <<Gameserver Version>> accepting connections
        printf("Gameserver Version %s\n", words[3]);
        break;

    case 2: 
        //S: + Client version accepted - please send Game-ID to join
        //S: - Client Version does not match server Version
        if(buffer[0] == '-') {
            if (strcmp(words[1],"Client") == 0){
                perror("Client Version nicht akzeptiert\n");
            } else if (strcmp(words[1],"TIMEOUT") == 0){
                perror("Client Version Time Out\n");
            } else {
                perror("Case 2 unknown Server Message\n");
            }
            return -1;
        }
        if(buffer[0] == '+') {
            if (strcmp(words[1],"Client") == 0){
                printf("Client Version akzeptiert\n");
            } else perror("Case 2 unknown Server Message\n");
        }
        break; 

    case 3: 
        //S: + PLAYING  Gamekind-Name 
        //S: - Did not get the expected ID command
        //S: - Not a valid game ID
        if(buffer[0] == '-') {
            if (strcmp(words[5],"expected") == 0){
                perror("Game ID Command falsch\n");
            } else if (strcmp(words[1],"TIMEOUT") == 0){
                perror("Game-ID Time Out\n");
            } else if(strcmp(words[1],"Not") == 0) {
                perror("Game-ID not valid\n");
            } else {
                perror("Case 3 unknown Server Message\n");
            }
            return -1;
        }
        
        if(buffer[0] == '+') {
            char *name = malloc(strlen(GAMEKINDNAME) + 2);
            strcpy(name, GAMEKINDNAME);
            strcat(name, "\n");
            if (strcmp(words[2], name) != 0){
                perror("Falsches Game Kind!\n");
                free(name);
                return -1;
            } else if (strcmp(words[2], name) == 0){
                printf("Game Kind: %s", words[2]);
            } else {
                perror("Case 3 unknown Server Message\n");
            }
            free(name);
        }
        break;

    case 4: //S: + << Game-Name >> // + Spielname
        if(buffer[0] == '-') {
            if (strcmp(words[1],"TIMEOUT") == 0){
                perror("Case 4 Time Out\n");
            } else {
                perror("Case 4 unknown Server Message\n");
            }
            return -1;
        }
    
        if(buffer[0] == '+') {
            char tmp[100];
            memset(tmp, 0, sizeof(tmp));
            for (int j = 1; j < token_anzahl ; j++ ){
                strcat(tmp, words[j]);
                if(j != token_anzahl - 1) strcat(tmp, " ");
            }
            printf("Das Spiel heißt: %s", tmp);
        }




        break;

    case 5: 
        //S: + YOU << Spielernummer >> << Spielername >> // + YOU (0 oder 1) Player1
        //S: - No free player
        if(buffer[0] == '-') {
            if (strcmp(words[1],"TIMEOUT") == 0){
                perror("Case 5 Time Out\n");
            } else if(strcmp(words[2],"free") == 0){
                perror("Kein freier Spieler\n");
            }
            else {
                perror("Case 5 unknown Server Message\n");
            }
            return -1;
        }
       
        if(buffer[0] == '+') {
            if (strcmp(words[1],"YOU") == 0){
                char tmp[30];
                memset(tmp, 0, sizeof(tmp));
                for (int j = 3; j < token_anzahl; j++ ){
                    strcat(tmp, words[j]);
                    if(j != token_anzahl - 1) strcat(tmp, " ");
                }
                printf("Deine Spielernummer ist %i und dein Spielername lautet %s",atoi(words[2])+1, tmp);

                //SHM befüllen
                memcpy(shm_p->Spielername, tmp, strlen(tmp)-1);
                shm_p->Spielernummer = atoi(words[2])+1;
                
            } else perror("Case 5 unknown Server Message\n");
        }
        break;

    case 6: //S: + TOTAL << Spieleranzahl >>
        if(buffer[0] == '-') {
            if (strcmp(words[1],"TIMEOUT") == 0){
                perror("Case 6 Time Out\n");
            } else {
                perror("Case 6 unknown Server Message\n");
            }
            return -1;
        }
      
        if(buffer[0] == '+') {
            if (strcmp(words[1],"TOTAL") == 0){
                printf("Spieleranzahl: %i\n", atoi(words[2]));

                //SHM Player Belegen
                shm_p->Spieleranzahl = atoi(words[2]);

            } else perror("Case 6 unknown Server Message\n");
        }
        break;

    case 7: //S: + << Spielernummer >> << Spielername >> << Bereit >>
        if(buffer[0] == '-') {
            if (strcmp(words[1],"TIMEOUT") == 0){
                perror("Case 7 Time Out\n");
            } else {
                perror("Case 7 unknown Server Message\n");
            }
            return -1;
        }


        char tmp[30];
        memset(tmp, 0, sizeof(tmp));
        for (int j = 2; j < token_anzahl - 1; j++ ){
            strcat(tmp, words[j]);
            if(j != token_anzahl - 2) strcat(tmp, " ");
        }
    
        if (atoi(words[token_anzahl-1]) == 0){ // words == 0\n oder auch 1\n maybe
            printf("Spieler %i (%s) ist noch nicht bereit\n",atoi(words[1])+1,tmp);
            
            shm_g->Flag = 0;

        } else {
            printf("Spieler %i (%s) ist bereit\n",atoi(words[1])+1,tmp);
            
            shm_g->Flag = 1;

        }

        //SHM Gegner Belegen
        memcpy(shm_g->Gegnername, tmp, strlen(tmp));
        shm_g->Gegnernummer = atoi(words[1])+1;

        break;

    case 8: //S: + ENDPLAYERS
        if(buffer[0] == '-') {
            if (strcmp(words[1],"TIMEOUT") == 0){
                perror("Case 8 Time Out\n");
            } else {
                perror("Case 8- unknown Server Message\n");
            }
            return -1;
        }

        if(buffer[0] == '+') {
            if (strcmp(words[1],"ENDPLAYERS\n") == 0){
                printf("Lasset das Spiel beginnen!\n\n\n");
            } else perror("Case 8+ unknown Server Message\n");
        }
        break;

    default:
        break;
    }

    return 0;
}


int init_performConnect(int sock_stream, char *gameID, spielerdaten *shm_p, gegnerdaten *shm_g, char* GAMEKINDNAME){

    char *buffer = malloc( sizeof(char) * BUF );
    ssize_t size = 0;
    
    //Kommunikation:
    //S: + MNM Gameserver <<Gameserver Version>> accepting connections
    getServerMessage(sock_stream,buffer,BUF,size);
    formatierteAusgabeTai(buffer, 1, shm_p, shm_g, GAMEKINDNAME);
    
    
    //C: VERSION <<Client Version>>
    send(sock_stream,"VERSION 2.3\n",strlen("VERSION 2.3\n"),0); //passt 


    //S: + Client version accepted - please send Game-ID to join
    //S: - Client Version does not match server Version
    getServerMessage(sock_stream,buffer,BUF,size);
    if( (formatierteAusgabeTai(buffer, 2, shm_p, shm_g, GAMEKINDNAME)) < 0){
        printf("case 2 Fehler\n");
        return -1;
    }


    //C: ID <<Game-ID>>  
    char destination[17] = "ID ";
    strcat(destination,gameID);
    strcat(destination,"\n");    
    send(sock_stream,destination,strlen(destination),0); 


    //S: + PLAYING  Gamekind-Name 
    //S: - Did not get the expected ID command
    //S: - Not a valid game ID
    getServerMessage(sock_stream,buffer,BUF,size);
    if(formatierteAusgabeTai(buffer, 3, shm_p, shm_g, GAMEKINDNAME) == -1) {
        printf("case 3 Fehler\n");
        return -1;
    }


    //S: + << Game-Name >> // + Spielname
    getServerMessage(sock_stream,buffer,BUF,size);
    if(formatierteAusgabeTai(buffer, 4, shm_p, shm_g, GAMEKINDNAME) == -1) {
        printf("case 4 Fehler\n");
        return -1;
    }


    //C: PLAYER [[ Gewünschte Spielernummer ]]
    send(sock_stream,"PLAYER \n",strlen("PLAYER \n"),0);


    //S: + YOU << Spielernummer >> << Spielername >> // + YOU (0 oder 1) 
    getServerMessage(sock_stream,buffer,BUF,size);
    if(formatierteAusgabeTai(buffer, 5, shm_p, shm_g, GAMEKINDNAME) == -1) {
        printf("case 5 Fehler\n");
        return -1;
    }


    //S: + TOTAL << Spieleranzahl >>
    getServerMessage(sock_stream,buffer,BUF,size);
    if(formatierteAusgabeTai(buffer, 6, shm_p, shm_g, GAMEKINDNAME) == -1) {
        printf("case 6 Fehler\n");
        return -1;
    }
    
    
    //Nun kommt fur jeden der anderen Spieler die Zeile: ¨(bzw nur einen lol)
    //S: + << Spielernummer >> << Spielername >> << Bereit >>
    getServerMessage(sock_stream,buffer,BUF,size);
    if(formatierteAusgabeTai(buffer, 7, shm_p, shm_g, GAMEKINDNAME) == -1) {
        printf("case 7 Fehler\n");
        return -1;
    }


    //S: + ENDPLAYER
    getServerMessage(sock_stream,buffer,BUF,size);
    if(formatierteAusgabeTai(buffer, 8, shm_p, shm_g, GAMEKINDNAME) == -1) {
        printf("case 8 Fehler\n");
        return -1;
    }

    free(buffer);

    return 0;
}

int performConnect(int sock_stream, char* gameID, spielerdaten *shm_p,gegnerdaten *shm_g, char* GAMEKINDNAME){

    int init = init_performConnect(sock_stream,gameID, shm_p, shm_g, GAMEKINDNAME);
    if (init < 0 ) {
        perror("init_performConnect failed!");
        return -1;
    }

    return 0;
}
