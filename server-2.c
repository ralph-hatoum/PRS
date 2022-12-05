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
#include <pthread.h>

pthread_mutex_t mutex;
int last_ack = 0;

struct arg_struct{
	int *skt;
	struct sockaddr_in *addr_sck_i;
};


void *RcvThread (void *arg){
	char ack_buf[100];
	struct arg_struct *args = (struct arg_struct *)arg;
	int s = sizeof *(args->addr_sck_i);
	printf("size %d\n",s);
	while(1){
		recvfrom(*(args->skt), (char *)ack_buf, 1024, 0, (struct sockaddr *)(args->addr_sck_i), &s);
		int a = atoi(&ack_buf[3]);
		//printf("ack received: %d\n", a);
		pthread_mutex_lock(&mutex);
		if (a>last_ack){
			last_ack = a;
		}
		pthread_mutex_unlock(&mutex);
		memset(ack_buf, 0, sizeof(ack_buf));
	};
	printf("ack : %s\n", ack_buf);
	return NULL;

};

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
                recvfrom(new_socket, (char *)Buffilename, 1024, MSG_WAITALL, (struct sockaddr *)&c_addr, &c_addr_size);
				char *filename = strtok(Buffilename, " ");

                printf("Received file name : %s\n", filename);

    			FILE* fichier = fopen(filename, "r");

				fseek(fichier, 0, SEEK_END); // seek to end of file
				int size = ftell(fichier)+1; // get current file pointer
				fseek(fichier, 0, SEEK_SET);
				printf("taille %d\n", size);
				int cpt = size/(1024-6);//6 car 6 digits de num plus espace
				printf("taille %d\n", cpt);
				char *BuFichier = (char*) malloc(sizeof(char)*size);

				fread(BuFichier,size-1,1,fichier);
				
				
				//thread ecoute
				struct arg_struct arguments;
				arguments.skt = &new_socket;
				arguments.addr_sck_i = &c_addr;
				printf("Before Thread\n");
				pthread_t thread_id;
				pthread_create(&thread_id, NULL, RcvThread, (void *)&arguments);


              int i = 1;
                fseek(fichier, 0, SEEK_SET);
                // Envoi du fichier
                printf("Sending file on socket number %d\n", new_socket);
                char ack_buff[1024];
                char expected_ack[12];
                
                int wnd_size = 10;
                while (i <= cpt+1)
                {
                	//sleep(0.300);
                	pthread_mutex_lock(&mutex);
                	i = last_ack+1;
					pthread_mutex_unlock(&mutex);
                	
                	if(cpt+1-i<wnd_size){
                		wnd_size = cpt+1-i;
                	}
                	
                	for (int h=0; h<wnd_size; h++){
		                int no_ack_flag = 0;
		                // Initializing sending buffer
		                char to_send[1024];
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
		                memset(to_send, 0, sizeof(to_send));
		                sprintf(to_send, "%s", seq_number);
		                // Reading bytes of the file
		                if (i == cpt+1 && (size-cpt*(1024-6))>0){
		                	memcpy(&to_send[6], &BuFichier[(i-1)*(1024-6)], size-cpt*(1024-6));
		                	sendto(new_socket, to_send, size-cpt*(1024-6)+5, 0, (struct sockaddr *)&c_addr, c_addr_size);//size-cpt*(1024-6)+6-1
		                	i++;
		                	//printf("message %d envoyé\n", i-1);
		                }else if ((size-cpt*(1024-6))==0){
		                	i++;
		                }else{
		                	memcpy(&to_send[6], &BuFichier[(i-1)*(1024-6)],(1024-6));
		                	sendto(new_socket, to_send, 1024, 0, (struct sockaddr *)&c_addr, c_addr_size);
		                	i++;
		                	//printf("message %d envoyé\n", i-1);
		                }
		            }
		            

                    // sending the buffer

                    //printf("Sent segment %s ; awaiting %s\n", seq_number, expected_ack);
					
					/*
                    // Waiting for ACK
                    memset(ack_buff, 0, sizeof(ack_buff));
                    fd_set set;
                    FD_SET(new_socket,&set);
                    struct timeval timeout;
                    timeout.tv_sec = 0;
                    timeout.tv_usec = 300;
                    int success;
                    success = select(new_socket+1, &set,NULL,NULL,&timeout);
                    if (success>0){

                        recvfrom(new_socket, (char *)ack_buff, 1024, 0, (struct sockaddr *)&c_addr, &c_addr_size);
                        if (strncmp(expected_ack, ack_buff, 9) == 0)
                        {
                            i += 1;//envoi seg suivant
                        }
                        else
                        {
                            //printf("Segment lost - retransmission needed\n");
                            char *to_throw = strtok(ack_buff, "_");
                            char *current_ack = strtok(NULL, "_");
                            //printf("Everything received until segment number %s\n", current_ack);
                            //printf("Preparing to re-send segment number %d\n", i);
                        }
                    } else {
                        //printf("Timed out ; preparing to send previous segment\n");
                    }*/
                }
				char FIN[3]="FIN";
				for (int v = 0; v<1000; v++){
					sendto(new_socket, FIN,3, 0, (struct sockaddr *)&c_addr, c_addr_size);
				}
				//libere memoire et arrete thread
                free(BuFichier);
                pthread_join(thread_id, NULL);

                printf("File sent ! \n");
            }
        }
        //exit(0);
    };
}
