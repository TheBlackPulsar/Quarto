#include "handle_rounds.h"

#include <regex.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <sys/socket.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>

#include "defs.h"
#include "performConnection.h"
#include "spieldata.h"

static int s_sock_stream;
static spielfeld *s_shm_spielfeld_ptr;

/* malloced for:
* s_time_limit (freed)
* s_next_spielstein
* s_field_dims (freed)
* s_field (freed)
*/
void handle_rounds(int sock_stream, spielfeld* shm_spielfeld_ptr, int pipe_fd) {
    s_sock_stream = sock_stream;
    s_shm_spielfeld_ptr = shm_spielfeld_ptr;
    int thinker_pid = getppid();
    regex_t regex_move;
    // regex_t regex_over;
    int recv_stat;
    // to match: + MOVE <time_limit>\n\0
    int reti;
    reti = regcomp(&regex_move, "\\+ MOVE [0-9]*", 0);
    if (reti) {
        printf("Regex regex_move compile failed\n");
        return;
    }
    char *buffer = malloc( sizeof(char) * BUF );
    if (buffer == NULL) {
        printf("Alloc for buffer failed! Please check if OOM.\n");
        regfree(&regex_move);
        return;
    }
    while (1)
    {
        recv_stat = getServerMessage(sock_stream,buffer,BUF,0);
        //if (recv_stat == 0) printf("S: %s", buffer);
        if (strcmp(buffer, "+ WAIT\n") == 0) {
            printf("Warte auf Gegner...\n");
            // instruction wait
            send(sock_stream, "OKWAIT\n", strlen("OKWAIT\n"), 0);
        } else if (strcmp(buffer, "+ GAMEOVER\n") == 0) {
            // instruction gameover
            printf("Das Spiel ist vorbei.\n");
            int winner = 0;
            while(getServerMessage(sock_stream,buffer,BUF,0) == 0) {
                //printf("%s",buffer); //Endsequenz nach dem Gameover (Field, Endfield, PLAYER0WON Yes/No, PLAYER1WON Yes/No, QUIT)
                if (strcmp(buffer, "+ PLAYER0WON Yes\n") == 0){
                    printf("Spieler 1 hat gewonnen!\n");
                    winner++;
                }
                if (strcmp(buffer, "+ PLAYER1WON Yes\n") == 0){
                    printf("Spieler 2 hat gewonnen!\n");
                    winner++;
                    if (winner == 2) printf("Das Spiel ist unentschieden ausgegangen.\n");
                }
                
                if (strcmp(buffer, "+ QUIT\n") == 0) break;
            }
            break;
        } else if (!regexec(&regex_move, buffer, 0, NULL, 0)) {
            // instruction move
            // + MOVE <maximale Zugzeit>
            if (strlen(buffer) <= 8) {
                printf("Incomplete MOVE instruction received!\n");
                //printf("%s", buffer);
                break;
            }
            char* s_time_limit = malloc(strlen(buffer) - 7);
            memcpy(s_time_limit, buffer + 7, strlen(buffer) - 8);// strlen(): returns to length of the string
            s_time_limit[strlen(buffer) - 8] = '\0';
            unsigned long time_limit = strtoul(s_time_limit,NULL, 0);// strtoul():string to unsigned long
            free(s_time_limit);
            shm_spielfeld_ptr->time_limit = time_limit;

            // + NEXT <zu setzender Spielstein>
            recv_stat = getServerMessage(sock_stream,buffer,BUF,0);
            //if (recv_stat == 0) printf("Nächster zu setzender Stein: %s", buffer);
            char *s_next_spielstein = malloc(strlen(buffer) - 7);
            memcpy(s_next_spielstein, buffer + 7, strlen(buffer) - 8);
            s_next_spielstein[strlen(buffer) - 8] = '\0';
            int next_spielstein = atoi(s_next_spielstein);
            shm_spielfeld_ptr->next = next_spielstein;
            free(s_next_spielstein);
            
            // + FIELD <Breite>, <Hoehe>
            recv_stat = getServerMessage(sock_stream,buffer,BUF,0);
            //if (recv_stat == 0) printf("S: %s", buffer);
            char *s_field_dims = malloc(strlen(buffer) - 8);
            memcpy(s_field_dims, buffer + 8, strlen(buffer) - 9);
            //printf("len: %s\n", buffer + 8);
            s_field_dims[strlen(buffer) - 9] = '\0';
            char *token = NULL;
            char *save_ptr = NULL;
            // match the width
            token = strtok_r(s_field_dims, ",", &save_ptr);
            unsigned long width = strtoul(token, NULL, 0);
            // match the height
            token = strtok_r(NULL, ",", &save_ptr);
            unsigned long height = strtoul(token, NULL, 0);
            free(s_field_dims);

            
            shm_spielfeld_ptr->height = height;
            shm_spielfeld_ptr->width = width;

            for (int i = 0; i < height; i++) {
                getServerMessage(sock_stream,buffer,BUF,0);
                token = NULL;
                save_ptr = NULL;
                token = strtok_r(buffer, " ", &save_ptr); // +
                token = strtok_r(NULL, " ", &save_ptr); // <Y>
                for (int j = 0; j < width && token; j++) {
                    token = strtok_r(NULL, " ", &save_ptr);
                    if (token[0] == '*') {
                        // -1 stands for *
                        shm_spielfeld_ptr->field[i * width + j] = -1;
                    } else {
                        // others directly make into integers
                        shm_spielfeld_ptr->field[i * width + j] = atoi(token);
                    }
                }
            }

            recv_stat = getServerMessage(sock_stream,buffer,BUF,0);
            //if (recv_stat == 0) printf("S: %s", buffer);
            if (strcmp(buffer, "+ ENDFIELD\n") != 0) {
                printf("kein ENDFIELD erhalten.\n");
                printf("%s", buffer);
                break;
            }

            shm_spielfeld_ptr->is_ready = true; 

            send(sock_stream, "THINKING\n", strlen("THINKING\n"), 0);
            recv_stat = getServerMessage(sock_stream,buffer,BUF,0);
            //if (recv_stat == 0) printf("S: %s", buffer);
            if (recv_stat == 0); // Werror unused variable is used now ;)
            if (strcmp(buffer, "+ OKTHINK\n") != 0) {
                printf("Assertion failed\n");
                //printf("%s", buffer);
                break;
            }
            
            kill(thinker_pid, SIGUSR1);
            // sending move instruction see sig_handler()

            fd_set set;
            FD_ZERO(&set);
            FD_SET(pipe_fd, &set);

            struct timeval timeout;
            timeout.tv_sec = 0;
            timeout.tv_usec = time_limit * 1e3;

            int ret = select(pipe_fd + 1, &set, NULL, NULL, &timeout);
            //printf("RET: %d\n", ret);
            if (ret == 0)
            {
                printf("No messages received at CONNECTOR side after timeout!\n");
            }
            else if (ret < 0)
            {
                printf("Pipe ERROR at CONNECTOR side!\n");
            }
            else
            {
                memset(buffer, 0, BUF);
                read(pipe_fd, buffer, BUF);
                printf("Sysprak-Client macht folgenden Zug: %s\n", buffer);
                send(s_sock_stream, buffer, strlen(buffer), 0);
            }

            recv_stat = getServerMessage(sock_stream,buffer,BUF,0);
            //if (recv_stat == 0) printf("S: %s", buffer);
            if (strcmp(buffer, "+ MOVEOK\n") != 0) {
                printf("Fehler bezügliches des Spielzugs\n");
                break;
            }
        }
    }
    regfree(&regex_move);
    free(buffer);
    kill(thinker_pid, SIGTERM);
}
