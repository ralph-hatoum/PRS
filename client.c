#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define buff_size 9
int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Utilisation : ./client <ip_serveur> <port_serveur>\n");
        return 0;
    }
    int sock;
    struct sockaddr_in my_addr;
    fd_set f_des;

    // Initializing socket
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    // Error handling
    if (sock < 0)
    {
        exit(1);
    }
    memset((char *)&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(atoi(argv[2]));
    inet_aton(argv[1], &my_addr.sin_addr);
    my_addr.sin_addr.s_addr = htonl(my_addr.sin_addr.s_addr);
    uint taille = sizeof(my_addr);

    char msg[buff_size] = "SYN";
    char to_rcv[buff_size];
    int i;

    FD_ZERO(&f_des);
    FD_SET(sock, &f_des);
    sendto(sock, msg, buff_size, 0, (struct sockaddr *)&my_addr, taille);
    printf("Sent ACK\n");
    //select(sock + 1, &f_des, NULL, NULL, NULL);
    recvfrom(sock, (char *)to_rcv, buff_size, MSG_WAITALL, (struct sockaddr *)&my_addr, &taille);
    printf("Received : %s\n", to_rcv);
    if (strcmp(to_rcv, "SYNACK") == 0)
    {
        char msg[buff_size] = "ACK";
        sendto(sock, msg, buff_size, 0, (struct sockaddr *)&my_addr, taille);
        printf("Sent ACK\n");
    }
}