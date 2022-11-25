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
    int option = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
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

    bind(sock, (struct sockaddr *)&my_addr, sizeof(my_addr));

    char msg[buff_size] = "SYN";
    char to_rcv[buff_size];
    int i;

    // Sending SYN
    sendto(sock, msg, buff_size, 0, (struct sockaddr *)&my_addr, sizeof(my_addr));
    printf("Sent SYN\n");

    // Waiting for SYNACK
    recvfrom(sock, (char *)to_rcv, buff_size, MSG_WAITALL, (struct sockaddr *)&my_addr, &taille);
    printf("Received : %s\n", to_rcv);

    // Separating new port and synack
    char *buff_to_compare = strtok(to_rcv, " ");
    char *port_to_contact = strtok(NULL, " ");
    if (strcmp(buff_to_compare, "SYNACK") == 0)
    {
        // Here if synack was received ; changing port to the port the server just sent

        //Socket should be addressed on the new port received
        int new_port = atoi(port_to_contact);
        printf("%d\n", new_port);
        char msg[buff_size] = "ACK";

        // On old socket, ACK should be sent to finish 3 way handshake
        sendto(sock, (char *)msg, buff_size, 0, (struct sockaddr *)&my_addr, taille);
        printf("Sent ACK on socket number %d\n", sock);
        printf("Connection established ! Waiting for file ... \n");

        my_addr.sin_port = htons(new_port);

        // Receiving file size
        char file_size_buff[buff_size];
        memset(file_size_buff, 0, sizeof(file_size_buff));
        recvfrom(sock, (char *)file_size_buff, buff_size, 0, (struct sockaddr *)&my_addr, &taille);
        printf("Received file size: %s\n", file_size_buff);

        // with file size, compute number of segments to be received
        int file_size = atoi(file_size_buff);
        int iterations = file_size / (buff_size - 8);
        memset(file_size_buff, 0, sizeof(file_size_buff));

        //Create new file to store result in
        FILE *fichier = NULL;
        // FILE NAME
        fichier = fopen("./result.pdf", "w");
        int n;
        char to_rcv[buff_size];
        memset(to_rcv, 0, sizeof(to_rcv));
        int i = 0;
        int last_segment_received = i;
        // Receiving and sending acks
        while (i < iterations)
        {
            // Receive segment and extracting seq number
            recvfrom(sock, (char *)to_rcv, 1024, MSG_WAITALL, (struct sockaddr *)&my_addr, &taille);
            char *seq_number = strtok(to_rcv, " ");
            last_segment_received = atoi(seq_number);
            printf("Received segment number %d\n", last_segment_received);

            // Sending ACK
            char ack[1024];
            if (i == 20)
            {
                sprintf(ack, "ACK_000019");
            }
            else
            {
                sprintf(ack, "ACK_%s", seq_number);
            }

            printf("%s\n", ack);
            sendto(sock, ack, 1024, 0, (struct sockaddr *)&my_addr, sizeof(my_addr));
            printf("ACK sent\n");

            if (last_segment_received == i)
            {
                // Writing segment in the file
                fwrite(to_rcv + 8, buff_size - 8, 1, fichier);
                memset(to_rcv, 0, sizeof(to_rcv));
                i += 1;
            }
            else
            {
                printf("Received segment %d\n", last_segment_received);
                printf("Already received it / not looking to receive it now - ignoring \n");
            }
        }
        // Last segment is dealt with differently
        char last_to_rcv[file_size - (iterations * (buff_size - 8)) + 8];
        memset(last_to_rcv, 0, sizeof(last_to_rcv));
        recvfrom(sock, (char *)last_to_rcv, file_size - (iterations * (buff_size - 8)) + 8, MSG_WAITALL, (struct sockaddr *)&my_addr, &taille);
        char *seq_number = strtok(to_rcv, " ");
        // Sending last ACK
        char ack[1024];
        sprintf(ack, "ACK_%s", seq_number);
        sendto(sock, ack, 1024, 0, (struct sockaddr *)&my_addr, sizeof(my_addr));

        //Writing last segment and closing the file
        fwrite(last_to_rcv + 8, file_size - (iterations * (buff_size - 8)), 1, fichier);
        fclose(fichier);

        printf("File successfully saved ! \n");
    }
}