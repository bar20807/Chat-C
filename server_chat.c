/*

    Proyecto de Sistemas Operativos

*/

#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "chat.pb-c.h"

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 100
#define CHAT__USER_OPTION__OP_TYPE__SEND_MESSAGE 5
#define CLIENT_DISCONNECTED 3

typedef struct {
    int sockfd;        // socket file descriptor
    int status;        // 1: connected, 2: waiting, 3: disconnected
    char username[64]; // username of the client
} Client;

Client clients[MAX_CLIENTS];
int client_count = 0;

/**
 * Esta función se encarga de enviar un mensaje a todos los clientes conectados, excepto al remitente del mensaje.
 *
 * @param message un puntero al mensaje que se quiere enviar
 */
void broadcast_message(Chat__Message *message) {
    Chat__Answer answer = CHAT__ANSWER__INIT;  // Inicializa respuesta
    answer.op = 5;  // Operación: mensaje enviado
    answer.response_status_code = 200;  // Código de respuesta OK
    answer.message = message;  // Mensaje a enviar

    uint8_t buffer[BUFFER_SIZE];
    size_t message_len = chat__answer__pack(&answer, buffer);  // Empaqueta la respuesta en un buffer

    for (int i = 0; i < client_count; i++) {
        if (clients[i].status != 3 && strcmp(clients[i].username, message->message_sender) != 0) {
            send(clients[i].sockfd, buffer, message_len, 0);  // Envía el mensaje empaquetado al cliente
        }
    }
}

// Function to handle messages from a client
void *handle_client(void *arg) {
    // Extract client information from argument
    Client *client = (Client *)arg;

    // Set client socket file descriptor
    int sockfd = client->sockfd;

    // Allocate memory for incoming message
    uint8_t *buffer = (uint8_t *)malloc(BUFFER_SIZE);

    // Loop to receive messages from client
    while (1) {
        // Receive message
        ssize_t len = recv(sockfd, buffer, BUFFER_SIZE, 0);

        // If message length is negative, client has disconnected
        if (len < 0) {
            // Set client status to disconnected
            client->status = CLIENT_DISCONNECTED;
            break;
        }

        // Unpack the received message
        Chat__UserOption *user_option = chat__user_option__unpack(NULL, len, buffer);

        // If the message could not be unpacked, skip to next iteration
        if (user_option == NULL) {
            continue;
        }

        // If the user option is to send a message to other clients
        if (user_option->op == CHAT__USER_OPTION__OP_TYPE__SEND_MESSAGE && user_option->message != NULL) {
            // Forward the message to other clients
            broadcast_message(user_option->message);
        }

        // Free the unpacked message
        chat__user_option__free_unpacked(user_option, NULL);
    }

    // Free buffer memory
    free(buffer);

    // Close client socket file descriptor
    close(sockfd);

    // Return null pointer
    return NULL;
}




int main() {
    // Creamos un socket TCP/IP
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Configuramos el socket para reutilizar la dirección
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // Configuramos la dirección y el puerto del servidor
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Asignamos la dirección y el puerto al socket
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    // Escuchamos por nuevas conexiones
    if (listen(server_fd, 10) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);

    // Ciclo principal para aceptar nuevas conexiones
    while (1) {
        // Esperamos por una nueva conexión
        int addrlen = sizeof(address);
        int new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        if (new_socket < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        // Si aún no se alcanzó el límite de clientes, agregamos al nuevo cliente a la lista
        if (client_count < MAX_CLIENTS) {
            Client *client = &clients[client_count++];
            client->sockfd = new_socket;
            client->status = 1;
            // Creamos un hilo para manejar la comunicación con el nuevo cliente
            pthread_t client_thread;
            pthread_create(&client_thread, NULL, handle_client, (void *)client);
            pthread_detach(client_thread);
            printf("New client connected (fd: %d)\n", new_socket);
              } else {
                    close(new_socket);
                    printf("Client limit reached. Connection refused.\n");
                }
            }
// Close all client sockets before exiting
for (int i = 0; i < client_count; i++) {
    close(clients[i].sockfd);
}

// Close server socket
close(server_fd);

return 0;
}


