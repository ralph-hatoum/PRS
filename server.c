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

    // Initialisation variables pour la socket udp
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
        // Three way handshake ici
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
            // Nouveau port pour la nouvelle socket : ancien port + 1
            int new_port = atoi(argv[1]) + 1;
            struct sockaddr_in addr_new_sock;
            memset((char *)&addr_new_sock, 0, sizeof(addr_new_sock));
            addr_new_sock.sin_family = AF_INET;
            addr_new_sock.sin_port = htons(new_port);
            addr_new_sock.sin_addr.s_addr = INADDR_ANY;
            int addr_new_sock_len = sizeof(addr_new_sock);
            setsockopt(new_socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

            bind(new_socket, (struct sockaddr *)&addr_new_sock, sizeof(addr_new_sock));

            //Renvoi du SYNACK+nouveau port de connexion
            char SYNACK[1024];
            snprintf(SYNACK, 1024, "SYN-ACK%d", new_port);
            printf("%s\n", SYNACK);
            sendto(udp_sock, SYNACK, 1024, 0, (struct sockaddr *)&c_addr, c_addr_size);

            // On recoit le SYN sur l'ancienne socket
            char msg_udp[9];
            recvfrom(udp_sock, (char *)msg_udp, 9, MSG_WAITALL, (struct sockaddr *)&c_addr, &c_addr_size);

            if (strcmp(msg_udp, "ACK") == 0)
            {
                // Si on reçoit un ack, connexion établie, on peut communiquer sur la nouvelle socket
                printf("Received : %s from socket number %d, connection established !!!\n", msg_udp, udp_sock);

                char Buffilename[100];
                recvfrom(new_socket, (char *)Buffilename, 1024, MSG_WAITALL, (struct sockaddr *)&addr_new_sock, &addr_new_sock_len);
                char *filename = strtok(Buffilename, " ");

                printf("Received file name : %s\n", filename);

                FILE *fichier = fopen(filename, "r");

                fseek(fichier, 0, SEEK_END);   // seek to end of file
                int size = ftell(fichier) + 1; // get current file pointer
                fseek(fichier, 0, SEEK_SET);
                printf("taille %d\n", size);
                int cpt = size / (1024 - 6); //6 car 6 digits de num plus espace
                printf("taille %d\n", cpt);
                char BuFichier[size];

                int *ack_tab = NULL;
                ack_tab = malloc(sizeof(cpt));

                fread(BuFichier, size - 1, 1, fichier);

                int i = 1;
                fseek(fichier, 0, SEEK_SET);
                // Envoi du fichier
                printf("Sending file on socket number %d\n", new_socket);
                char ack_buff[1024];
                char expected_ack[12];
                int expected_ack_number = 0;
                char to_send[1024];
                int max_ack = 0;
                fd_set set;
                FD_SET(new_socket, &set);
                struct timeval timeout;
                timeout.tv_sec = 2;
                timeout.tv_usec = 300;
                while (i <= cpt + 1 && max_ack != i)
                {
                    if (i <= cpt + 1)
                    {
                        for (int k = 0; k < 3; k++)
                        {

                            // Initializing sending buffer
                            char seq_number[8];
                            memset(seq_number, 0, sizeof(seq_number));
                            // ADDING SEQ NUMBER
                            if (i < 10)
                            {
                                sprintf(seq_number, "00000%d ", i);
                            }
                            else if (i < 100)
                            {
                                sprintf(seq_number, "0000%d ", i);
                            }
                            else if (i < 1000)
                            {
                                sprintf(seq_number, "000%d ", i);
                            }
                            else if (i < 10000)
                            {
                                sprintf(seq_number, "00%d ", i);
                            }
                            else if (i < 100000)
                            {
                                sprintf(seq_number, "0%d ", i);
                            }
                            else
                            {
                                sprintf(seq_number, "%d ", i);
                            }

                            sprintf(expected_ack, "ACK%s", seq_number);
                            expected_ack_number = i;
                            memset(to_send, 0, sizeof(to_send));
                            sprintf(to_send, "%s", seq_number);
                            // Reading bytes of the file

                            if (i == cpt + 1 && (size - cpt * (1024 - 6)) > 0)
                            {
                                memcpy(&to_send[6], &BuFichier[(i - 1) * (1024 - 6)], size - cpt * (1024 - 6));
                                sendto(new_socket, to_send, size - cpt * (1024 - 6) + 5, 0, (struct sockaddr *)&c_addr, c_addr_size); //size-cpt*(1024-6)+6-1
                            }
                            else
                            {
                                memcpy(&to_send[6], &BuFichier[(i - 1) * (1024 - 6)], (1024 - 6));
                                sendto(new_socket, to_send, 1024, 0, (struct sockaddr *)&c_addr, c_addr_size);
                            }
                            printf("Sent packet : %d\n", i);
                            i += 1;
                        }
                    }

                    // Waiting for ACK
                    memset(ack_buff, 0, sizeof(ack_buff));
                    int success;
                    success = select(new_socket + 1, &set, NULL, NULL, &timeout);
                    if (success > 0)
                    {

                        recvfrom(new_socket, (char *)ack_buff, 1024, 0, (struct sockaddr *)&c_addr, &c_addr_size);
                        char ack_number[6];
                        memcpy(&ack_number, &ack_buff[6], 6);
                        int ack_num_int;
                        ack_num_int = atoi(ack_number);
                        printf("Received ack number : %d \n", ack_num_int);

                        if (ack_num_int > max_ack)
                        {
                            max_ack = ack_num_int;
                        }
                        printf("Current max ack : %d, confirming all segments until segment %d were received\n", max_ack, max_ack);
                        ack_tab[ack_num_int] += 1;

                        if (ack_num_int < max_ack)
                        {
                            printf("Packet %d already received, late ACK to be ignored\n", ack_num_int);
                        }
                        else
                        {
                            ack_tab[ack_num_int] += 1;
                            if (ack_tab[ack_num_int] > 1)
                            {
                                printf("Duplicate ACK %d; most likely packet loss - need to retransmit packet %d\n", ack_num_int, ack_num_int + 1);
                                char to_send[1024];
                                if (ack_num_int + 1 < 10)
                                {
                                    sprintf(to_send, "00000%d ", ack_num_int + 1);
                                }
                                else if (ack_num_int + 1 < 100)
                                {
                                    sprintf(to_send, "0000%d ", ack_num_int + 1);
                                }
                                else if (ack_num_int + 1 < 1000)
                                {
                                    sprintf(to_send, "000%d ", ack_num_int + 1);
                                }
                                else if (ack_num_int + 1 < 10000)
                                {
                                    sprintf(to_send, "00%d ", ack_num_int + 1);
                                }
                                else if (ack_num_int + 1 < 100000)
                                {
                                    sprintf(to_send, "0%d ", ack_num_int + 1);
                                }
                                else
                                {
                                    sprintf(to_send, "%d ", ack_num_int + 1);
                                }

                                if (ack_num_int + 1 == cpt + 1 && (size - cpt * (1024 - 6)) > 0)
                                {
                                    memcpy(&to_send[6], &BuFichier[(ack_num_int + 1 - 1) * (1024 - 6)], size - cpt * (1024 - 6));
                                    sendto(new_socket, to_send, size - cpt * (1024 - 6) + 5, 0, (struct sockaddr *)&c_addr, c_addr_size); //size-cpt*(1024-6)+6-1
                                }
                                else
                                {
                                    memcpy(&to_send[6], &BuFichier[(ack_num_int + 1 - 1) * (1024 - 6)], (1024 - 6));
                                    sendto(new_socket, to_send, 1024, 0, (struct sockaddr *)&c_addr, c_addr_size);
                                }
                                printf("Resent seg number %d\n", ack_num_int + 1);
                            }
                        }
                        i += 1;
                    }
                    else
                    {

                        if (i + 1 == cpt + 1 && (size - cpt * (1024 - 6)) > 0)
                        {
                            sendto(new_socket, to_send, size - cpt * (1024 - 6) + 5, 0, (struct sockaddr *)&c_addr, c_addr_size); //size-cpt*(1024-6)+6-1
                        }
                        else
                        {

                            sendto(new_socket, to_send, 1024, 0, (struct sockaddr *)&c_addr, c_addr_size);
                        }
                        printf("Resent packet %d\n", i);
                    }
                }

                char FIN[3] = "FIN";
                for (int h = 0; h < 1; h++)
                {
                    sendto(new_socket, FIN, 3, 0, (struct sockaddr *)&c_addr, c_addr_size);
                }
                printf("max ack : %d, last packet sent : %i, justifying transmission is indeed done\n", max_ack, i);
                printf("File sent ! \n");
                free(ack_tab);
            }
        }
        //exit(0);
    };
}
