#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Utilisation : ./server <port UDP>\n");
        exit(0);
    }

    int udp_sock;
    int reuse = 1;
    struct sockaddr_in my_addr_udp;
    fd_set f_des;

    // initialisation socket UDP

    udp_sock = socket(AF_INET, SOCK_DGRAM, 0);

    if (udp_sock < 0)
    {
        printf("Erreur création socket UDP\n");
        exit(0);
    }

    printf("Socket UDP : %d\n", udp_sock);
    memset((char *)&my_addr_udp, 0, sizeof(my_addr_udp));
    my_addr_udp.sin_family = AF_INET;
    my_addr_udp.sin_port = htons(atoi(argv[1]));
    my_addr_udp.sin_addr.s_addr = INADDR_ANY;
    setsockopt(udp_sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    bind(udp_sock, (struct sockaddr *)&my_addr_udp, sizeof(my_addr_udp));

    // structure pour récupérer infos clients
    struct sockaddr_in c_addr;
    int c_addr_size = sizeof(c_addr);

    FD_ZERO(&f_des);
    //printf("Zeroed\n");
    FD_SET(udp_sock, &f_des);
    //printf("Set socket\n");

    while (1)
    {
        // On réinitialise les bits correspondants aux socket dans le file descriptor
        FD_ZERO(&f_des);
        //printf("Zeroed\n");
        FD_SET(udp_sock, &f_des);
        //printf("Set socket\n");

        // On surveille les deux sockets maintenant
        //printf("Waiting for messages or connections\n");

        select(udp_sock + 1, &f_des, NULL, NULL, NULL);

        //rintf("Connection/message detected \n");

        if (FD_ISSET(udp_sock, &f_des))
        {
            //printf("UDP message detected\n");
            if (fork() == 0)
            {
                char msg_udp[8];
                recvfrom(udp_sock, (char *)msg_udp, 8, MSG_WAITALL, (struct sockaddr *)&c_addr, &c_addr_size);
                printf("Message on UDP socket : %s\n", msg_udp);
                exit(0);
            }
        }
    };
}