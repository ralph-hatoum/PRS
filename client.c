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

    //FD_ZERO(&f_des);
    //FD_SET(sock, &f_des);
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
        //int data_sock;
        //data_sock = socket(AF_INET, SOCK_DGRAM, 0);
        /*if (data_sock < 0)
        {
            exit(1);
        }*/
        int new_port = atoi(port_to_contact);
        printf("%d\n", new_port);
        char msg[buff_size] = "ACK";

        struct sockaddr_in new_sock;
        memset((char *)&new_sock, 0, sizeof(new_sock));
        new_sock.sin_family = AF_INET;
        new_sock.sin_port = htons(new_port);
        inet_aton(argv[1], &new_sock.sin_addr);
        new_sock.sin_addr.s_addr = htonl(new_sock.sin_addr.s_addr);
        uint size_new_sock = sizeof(new_sock);

        //bind(data_sock, (struct sockaddr *)&new_sock, sizeof(new_sock));

        sendto(sock, (char *)msg, buff_size, 0, (struct sockaddr *)&my_addr, taille);
        printf("Sent ACK on socket number %d\n", sock);

        printf("Connection established ! Waiting for file ... \n");
        char file_size_buff[buff_size];
        memset(file_size_buff, 0, sizeof(file_size_buff));
        recvfrom(sock, (char *)file_size_buff, buff_size, MSG_WAITALL, (struct sockaddr *)&new_sock, &size_new_sock);

        printf("Received file size: %s\n", file_size_buff);
        int file_size = atoi(file_size_buff);
        int iterations = file_size / (buff_size - 8);
        memset(file_size_buff, 0, sizeof(file_size_buff));
        int i;
        FILE *fichier = NULL;
        // FILE NAME
        fichier = fopen("./result.pdf", "w");
        int n;
        char to_rcv[buff_size];
        memset(to_rcv, 0, sizeof(to_rcv));
        for (i = 0; i < iterations; i++)
        {
            recvfrom(sock, (char *)to_rcv, 1024, MSG_WAITALL, (struct sockaddr *)&new_sock, &size_new_sock);
            char *seq_number = strtok(to_rcv, " ");
            // ACK SENDING
            char ack[10];
            //sprintf(ack, "ACK_%s", seq_number);
            //sendto(sock, (char *)ack, 3, 0, (struct sockaddr *)&new_sock, &size_new_sock);
            printf("ACK sent\n");
            fwrite(to_rcv + 8, buff_size - 8, 1, fichier);
            printf("Received segment %s\n", seq_number);
            memset(to_rcv, 0, sizeof(to_rcv));
        }
        char last_to_rcv[file_size - (iterations * (buff_size - 8)) + 8];
        memset(last_to_rcv, 0, sizeof(last_to_rcv));
        recvfrom(sock, (char *)last_to_rcv, file_size - (iterations * (buff_size - 8)) + 8, MSG_WAITALL, (struct sockaddr *)&new_sock, &size_new_sock);
        printf("Received segment %s\n\n", last_to_rcv);
        fwrite(last_to_rcv + 8, file_size - (iterations * (buff_size - 8)), 1, fichier);
        fclose(fichier);
        }
}