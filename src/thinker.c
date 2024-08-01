#include <sys/types.h>
#include <signal.h>
#include <unistd.h>

#include "defs.h"
#include "bibs.h"
#include "spieldata.h"

volatile sig_atomic_t stop;
spielfeld* g_sf;

void signal_handler_thinker(int signum) {
    //printf("Received signal %d\n", signum);
    if (g_sf->is_ready)
        stop = 1;
}


/*
 * Bit counted from lsb=0 to msb
 */
int get_nth_bit(int num, int n) {
    int mask = 1 << n;
    return (num & mask) / mask;
}

int equal_among_four(int a, int b, int c, int d) {
    return (a == b) && (b == c) &&(c == d);
}

int check_line(int stein1, int stein2, int stein3, int stein4) {
    if (stein1 == -1 || stein2 == -1 || stein3 == -1 || stein4 == -1) {
        return 0;
    }
    //printf("%d\n", equal_among_four(get_nth_bit(stein1, 0), get_nth_bit(stein2, 0), get_nth_bit(stein3, 0), get_nth_bit(stein4, 0)));
    if (equal_among_four(get_nth_bit(stein1, 0), get_nth_bit(stein2, 0), get_nth_bit(stein3, 0), get_nth_bit(stein4, 0)) ||
        equal_among_four(get_nth_bit(stein1, 1), get_nth_bit(stein2, 1), get_nth_bit(stein3, 1), get_nth_bit(stein4, 1)) ||
        equal_among_four(get_nth_bit(stein1, 2), get_nth_bit(stein2, 2), get_nth_bit(stein3, 2), get_nth_bit(stein4, 2)) ||
        equal_among_four(get_nth_bit(stein1, 3), get_nth_bit(stein2, 3), get_nth_bit(stein3, 3), get_nth_bit(stein4, 3))) {

        return 1;
    } else {
        return 0;
    }
}


int if_can_win(spielfeld* sf, unsigned int* rrow, unsigned int* rcol) {
    int res = 0;
    for (unsigned int row = 0; row < sf->height; row++) {
        for (unsigned int col = 0; col < sf->width; col++) {
            unsigned int curr_row = row*sf->width;
            if (sf->field[row*sf->width + col] == -1) {
                // try to put the stein
                sf->field[row*sf->width + col] = sf->next;
                // check horizontal line
                res = res || check_line(sf->field[curr_row], sf->field[curr_row+1], sf->field[curr_row+2], sf->field[curr_row+3]);
                // check vertical line
                res = res || check_line(sf->field[col], sf->field[1*sf->width + col], sf->field[2*sf->width + col], sf->field[3*sf->width + col]);
                if (col == row) {
                    res = res || check_line(sf->field[0], sf->field[1*sf->width + 1], sf->field[2*sf->width + 2], sf->field[3*sf->width + 3]);
                } else if (col + row == 3) {
                    res = res || check_line(sf->field[3], sf->field[1*sf->width + 2], sf->field[2*sf->width + 1], sf->field[3*sf->width]);
                }
                sf->field[row*sf->width + col] = -1;
            }
            if (res) {
                *rcol = col;
                *rrow = row;
                return res;
            }
        }
    }
    return res;
}


char* nexttobin(int n){
    /*
    bin besteht aus 4 Zahlen, die jeweils anzeigen, welche vier Traits ein Spielstein mit der Nummer hat
    1.Zahl: hoch (0) oder flach (1)
    2.Zahl: breit (0) oder schmal (1)
    3.Zahl: hohl(0) oder solide (1)
    4.Zahl: rund (0) oder eckig(1)
    */
     
    char* bin;
    // falls Feld noch leer 
    if (n == -1){
        bin = "****";
    }
    else{
        switch(n){

            case 0: bin = "1110";
            break;
            case 1: bin = "1100";
            break;
            case 2: bin = "0110";
            break;
            case 3: bin = "0100";
            break;
            case 4: bin = "1010";
            break;
            case 5: bin = "1000";
            break;
            case 6: bin = "0010";
            break;
            case 7: bin = "0000";
            break;
            case 8: bin = "1111";
            break;
            case 9: bin = "1101";
            break;
            case 10: bin = "0111";
            break;
            case 11: bin = "0101";
            break;
            case 12: bin = "1011";
            break;
            case 13: bin = "1001";
            break;
            case 14: bin = "0011";
            break;
            case 15: bin = "0001";
            break;
        
        
        }
    }
    return bin;
}
char* intospielfeld(int move){

    char* rueckgabe;
    switch(move){
        case 0: rueckgabe = "A4";
        break;
        case 1: rueckgabe = "B4";
        break;
        case 2: rueckgabe = "C4";
        break;
        case 3: rueckgabe = "D4";
        break;
        case 4: rueckgabe = "A3";
        break;
        case 5: rueckgabe = "B3";
        break;
        case 6: rueckgabe = "C3";
        break;
        case 7: rueckgabe = "D3";
        break;
        case 8: rueckgabe = "A2";
        break;
        case 9: rueckgabe = "B2";
        break;
        case 10: rueckgabe = "C2";
        break;
        case 11: rueckgabe = "D2";
        break;
        case 12: rueckgabe = "A1";
        break;
        case 13: rueckgabe = "B1";
        break;
        case 14: rueckgabe = "C1";
        break;
        case 15: rueckgabe = "D1";
        break;
    }
    return rueckgabe;
}


int steineauswaehlen(int* steine,spielfeld* sf){
    
    int rueckgabe;
    steine[sf->next] = -1;
    for(int j = 0; j<16;j++){
       if(steine[j]!=-1){
            rueckgabe = steine[j];
       }
    }
    steine[rueckgabe]= -1;
    return rueckgabe;
}

int Spielfeldausgabe(spielfeld* sf, int pipe_fd,int* steine){

    char* zug;
    printf("Gegner hat gespielt: \n");
    printf("Next: %s\n",nexttobin(sf->next));
    printf("    A     B     C     D\n");
    printf("  +----------------------- +\n");
    for (unsigned int row = 0; row < sf->height; row++) {
        printf("%ld |", sf->height - row);
        for (unsigned int col = 0; col < sf->width; col++) {
            // printf("token: %d\n", sf->field[row*sf->width + col]);
            char *next_bin = nexttobin(sf->field[row*sf->width + col]);
            printf(" %s ", next_bin);
            //nächsten move auswählen
            if (sf->field[row*sf->width + col] == -1){
                zug = intospielfeld(row*sf->width + col);
            }
            if (sf->field[row*sf->width + col] != -1) {
                steine[sf->field[row*sf->width + col]] = -1;
            }
        }
        printf("| %ld\n", sf->height - row);
    }
    printf("  +----------------------- +\n");
    printf("    A     B     C     D\n");

    sf->is_ready = false;

    int spielzug = steineauswaehlen(steine,sf);
    char s_play_inst[20];
    
    unsigned row = 0, col = 0;
    if (if_can_win(sf, &row, &col)) {
        zug = intospielfeld(row*sf->width + col);
        sprintf(s_play_inst, "PLAY %s\n",zug);
    } else {
        sprintf(s_play_inst, "PLAY %s,%d\n",zug, spielzug);
    }
    //printf("can win? %d\n", if_can_win(sf, &row, &col));
    //printf("row, col: %d, %d\n", row, col);
    //printf("%s\n", s_play_inst);
    write(pipe_fd, s_play_inst, strlen(s_play_inst));

return 0;
}




int think(spielfeld* sf, int pipe_fd){
    int steine[16] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};

    g_sf = sf;
    signal(SIGUSR1, signal_handler_thinker);
    while (1) {
        while (!stop) {
            pause();
        }
        
        Spielfeldausgabe(sf, pipe_fd,steine);
        stop = 0;
    }
    return 0;

}
