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
#include <stdbool.h>

struct client_queue
{
    struct sockaddr_in *client_next_up;
    struct client_queue *next_clients;
};

typedef struct client_queue client_queue_t;

client_queue_t *new_queue()
{
    client_queue_t *queue;
    queue = malloc(sizeof(*queue));
    queue->client_next_up = NULL;
    queue->next_clients = NULL;
    return queue;
};

struct sockaddr_in grab_client_from_queue(client_queue_t *queue)
{
    struct sockaddr_in client_to_return = *(queue->client_next_up);
    struct sockaddr_in client_to_append = *(queue->next_clients->client_next_up);
    client_queue_t new_queue = *(queue->next_clients->next_clients);
}

void add_client(client_queue_t *queue) {
    
}

int main()
{
}