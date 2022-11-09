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
    // On vérifie que le port du serveur est bien fourni en argument
    if (argc != 2)
    {
        printf("Utilisation : ./server <port UDP>\n");
        exit(0);
    }

    // Initialisation de variables
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

    while (1)
    {
        // Initialisation d'un buffer + lecture sur la socket UDP
        char msg_udp[9];
        recvfrom(udp_sock, (char *)msg_udp, 9, MSG_WAITALL, (struct sockaddr *)&c_addr, &c_addr_size);
        printf("Message on UDP socket : %s\n", msg_udp);

        if (strcmp(msg_udp, "SYN") == 0)
        {
            // On est ici si le message reçu est un SYN
            printf("Someone attempting to connect ...\n");

            //Initialisation de la socket de discussion
            int new_socket = socket(AF_INET, SOCK_DGRAM, 0);
            if (new_socket < 0)
            {
                printf("New udp socket failed");
                exit(-1);
            }
            int new_port = atoi(argv[1]) + 1;
            struct sockaddr_in addr_new_sock;
            memset((char *)&addr_new_sock, 0, sizeof(addr_new_sock));
            addr_new_sock.sin_family = AF_INET;
            addr_new_sock.sin_port = htons(new_port);
            addr_new_sock.sin_addr.s_addr = INADDR_ANY;
            setsockopt(new_socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

            bind(new_socket, (struct sockaddr *)&addr_new_sock, sizeof(addr_new_sock));

            //Renvoi du SYNACK+nouveau port de connexion
            char SYNACK[1024];
            snprintf(SYNACK, 1024, "SYNACK %d", new_port);
            printf("%s\n", SYNACK);
            sendto(udp_sock, SYNACK, 1024, 0, (struct sockaddr *)&c_addr, c_addr_size);

            // On envoie recoit le SYN sur l'ancienne socket
            char msg_udp[9];
            recvfrom(udp_sock, (char *)msg_udp, 9, MSG_WAITALL, (struct sockaddr *)&c_addr, &c_addr_size);

            if (strcmp(msg_udp, "ACK") == 0)
            {
                // Si on reçoit un ack, connexion établie, on peut communiquer sur la nouvelle socket
                printf("Received : %s from socket number %d, connection established !!!\n", msg_udp, udp_sock);

                FILE *fichier = NULL;
                printf("Reading and sending file ...\n");

                //Opening file + reading
                int sent_bytes = 0;
                //char file_buffer[1020];
                fichier = fopen("./text.txt", "r");
                fseek(fichier, 0, SEEK_END);
                int size = ftell(fichier);
                int iterations = size / 1024;
                size += 4 * iterations;
                printf("Taille fichier %d\n", size);

                // Sending number of iterations
                char file_size[1024];
                sprintf(file_size, "%d", size);
                sendto(new_socket, (char *)file_size, 1024, MSG_WAITALL, (struct sockaddr *)&c_addr, c_addr_size);
                printf("Sent %s on socket number %d\n", file_size, new_socket);

                int i;
                fseek(fichier, 0, SEEK_SET);
                // Envoi du fichier
                printf("Sending file on socket number %d\n", new_socket);

                for (i = 0; i < iterations; i++)
                {
                    printf("%d\n\n", i);
                    // Initializing sending buffer
                    char to_send[1024];
                    memset(to_send, 0, sizeof(to_send));
                    // Reading 1020 bytes of the file
                    fread(to_send, 1024, 1, fichier); // MIEUX DE LIRE TOUT LE FICHIER D'UN COUP
                    printf("%d\n\n", i);
                    // sending the buffer
                    sendto(new_socket, to_send, 1024, 0, (struct sockaddr *)&c_addr, c_addr_size);
                }
                //Last buffer needs to be dealt with differently
                char to_send[size - (iterations)*1024];
                memset(to_send, 0, sizeof(to_send));
                // Adding seq number
                //sprintf(to_send, "%d ", iterations);
                // Initializing last file buffer to the right size
                //char last_file_buffer[size - (iterations)*1020];
                // Reading from file into last file buffer
                fread(to_send, size - (iterations * 1024), 1, fichier);
                // Adding last file buffer content to sending buffer
                //sprintf(to_send + strlen(to_send), "%s", last_file_buffer);
                printf("%d\n\n", iterations);
                printf("%s\n\n", to_send);
                // sending it
                sendto(new_socket, to_send, 1024, 0, (struct sockaddr *)&c_addr, c_addr_size);
                printf("File sent ! \n");
            }
        }
        //exit(0);
    };
}