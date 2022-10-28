#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define buff_size 1024
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

    // Separating new port and synack
    char *buff_to_compare = strtok(to_rcv, " ");
    char *port_to_contact = strtok(NULL, " ");
    if (strcmp(buff_to_compare, "SYNACK") == 0)
    {
        int data_sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (data_sock < 0)
        {
            exit(1);
        }
        int new_port = atoi(port_to_contact);
        printf("%d\n", new_port);
        char msg[buff_size] = "ACK";

        struct sockaddr_in new_sock;
        memset((char *)&new_sock, 0, sizeof(new_sock));
        new_sock.sin_family = AF_INET;

        new_sock.sin_port = htons(new_port);
        inet_aton(argv[1], &new_sock.sin_addr);
        new_sock.sin_addr.s_addr = htonl(new_sock.sin_addr.s_addr);
        uint taille_new_sock = sizeof(new_sock);

        bind(data_sock, (struct sockaddr *)&new_sock, sizeof(new_sock));

        sendto(sock, (char *)msg, buff_size, 0, (struct sockaddr *)&my_addr, taille);
        printf("Sent ACK on socket number %d\n", sock);
        printf("Connection established ! Waiting for file ... \n");

        char file[buff_size];

        recvfrom(data_sock, (char *)file, buff_size, 0, (struct sockaddr *)&new_sock, taille_new_sock);

        printf("%s\n", file);
    }
}