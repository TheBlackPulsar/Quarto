#include <sys/types.h>
#include <sys/socket.h>

#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "defs.h"
#include "bibs.h"
#include "socket.h"


int createSocket(char* HOSTNAME, int PORTNUMMER) {

    //Erzeugen des Socket - Verbindung über TCP
    int network_socket;
    network_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (network_socket < 0) {
            //Fehler beim Erzeugen des Sockets
            printf("Es ist ein Fehler beim Erzeugen des Sockets aufgetreten\n");
            return -1;
        } else {
            printf("Der Socket zum Verbindungsaufbau wurde erfolgreich erzeugt.\n");
        } 
 
    //Auflösen des Hostname in eine IPv4 Addresse
    struct addrinfo hints;
    struct addrinfo *res, *tmp;
    char host[256];

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;

    int ret = getaddrinfo(HOSTNAME, NULL, &hints, &res);
    if (ret != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
        printf("Beim Auflösen der Addresse ist ein Fehler aufgetreten.");
        return -1;
    }
    
    printf("Bitte sicherstellen, dass Sie im Uni-Netzwerk sind. (VPN)\n");

        for (tmp = res; tmp != NULL; tmp = tmp->ai_next) {
            getnameinfo(tmp->ai_addr, tmp->ai_addrlen, host, sizeof(host), NULL, 0, NI_NUMERICHOST);
        }  

    freeaddrinfo(res);

    //Spezifikation der Adresse für den Socket
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORTNUMMER);
    //if (inet_addr(host) ==  inet_addr("10.155.92.198")) {
        server_address.sin_addr.s_addr = inet_addr(host); //Vorsicht inet_addr(host) und inet_addr(10.155.92.198)
    /*} else {
    //    return -1;
    }*/

    //Herstellen einer Verbindung mit dem Server
    int connection_status = connect(network_socket, (struct sockaddr *) &server_address, sizeof(server_address));
       
        if (connection_status < 0){
            //Fehler beim herstellen einer Verbindung
            printf("Ein Fehler bei der Verbindung mit dem Server ist aufgetreten\n");
            return -1;
        } else {
            printf("Die Verbindung zum Server wurde erfolgreich hergestellt.\n");
        }
    
    return network_socket;
    
}