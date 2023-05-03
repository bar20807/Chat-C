#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <curl/curl.h>
#include <ctype.h>
#include <pthread.h>
#include "chat.pb-c.h"
#include <stdio.h>
#include <stdlib.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <net/if.h>
#include <signal.h>

/* -------------------------------------------------------------------------- */
/*                                 CHAT SERVER                                */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* Sistemas Operativos 				                                          */
/* Universidad del Valle de Guatemala                                         */
/* Cristian Aguirre 20231				                                          */
/* Rodrigo BArrera 20807                                                       */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* Chat realizado en C con multithreading y uso de protobuf                   */

#define BUFFER_SIZE 1024

char *estado(int valor)
{
    char *cadena = malloc(sizeof(char) * 40);
    switch (valor)
    {
    case 1:
        strcpy(cadena, "ACTIVO");
        break;
    case 2:
        strcpy(cadena, "OCUPADO");
        break;
    case 3:
        strcpy(cadena, "INACTIVO");
        break;
    default:
        strcpy(cadena, "FUERA DEL SERVER");
        break;
    }
    return cadena;
}

void abortAction(int sig)
{
    printf("El programa ha sido interrumpido inesperadamente\n");
    exit(1);
}

void *client_listening(void *arg)
{
    int client_socket = *(int *)arg;

    while (1)
    {
        // Recibir un buffer del socket
        uint8_t recv_buffer[BUFFER_SIZE];
        ssize_t recv_size = recv(client_socket, recv_buffer, sizeof(recv_buffer), 0);
        if (recv_size < 0)
        {
            perror("ERROR > La respuesta no ha sido recibida");
            exit(1);
        }

        if (recv_size == 0)
        {
            perror("SERVIDOR APAGADO");
            exit(1);
        }

        // Deserializar el buffer en un mensaje Message
        Chat__Answer *response_servidor = chat__answer__unpack(NULL, recv_size, recv_buffer);

        int act = response_servidor->op;

        switch (act)
        {
        case 1:

            printf("<ENVIAR MENSAJE PUBLICO>\n");
            if (response_servidor->response_status_code == 200)
            {
                Chat__Message *mensaje_recibido = response_servidor->message;
                printf("\n\nDE [%s] PARA [%s]: %s\n", mensaje_recibido->message_sender, "todos", mensaje_recibido->message_content);
            }
            break;
        case 2:
            printf("<ENVIAR MENSAJE PRIVADO>\n");
            if (response_servidor->response_status_code == 200)
            {
                Chat__Message *mensaje_recibido = response_servidor->message;
                printf("\n\nDE [%s] PARA [%s]: %s\n", mensaje_recibido->message_sender, mensaje_recibido->message_destination, mensaje_recibido->message_content);
            }
            else
            {
                Chat__Message *mensaje_recibido = response_servidor->message;
                printf("\n\nDE [%s] PARA [%s]: %s\n", mensaje_recibido->message_sender, mensaje_recibido->message_destination, response_servidor->response_message);
            }
            break;
        case 3:
            printf("<CAMBIAR ESTADO DE SESION>\n");
            break;
        case 4:
            printf("\n\n\t [ Usuarios Conectados ]\n");
            printf("--------------------------------------------\n");
            Chat__UsersOnline *usuarios_conected = response_servidor->users_online;
            for (int i = 0; i < usuarios_conected->n_users; i++)
            {
                Chat__User *usuario_leido = usuarios_conected->users[i];
                char nombre_estado[40];
                strcpy(nombre_estado, estado(usuario_leido->user_state));
                printf("Usuario -> [%s] | IP -> [%s] | Estado -> [%s]\n\n", usuario_leido->user_name, usuario_leido->user_ip, nombre_estado);
            }

            break;
        case 5:
        {

            if (response_servidor->response_status_code == 200)
            {
                Chat__UsersOnline *usuarios_conected = response_servidor->users_online;
                for (int i = 0; i < usuarios_conected->n_users; i++)
                {
                    Chat__User *usuario_leido = usuarios_conected->users[i];
                    if (strcmp("Vacio", usuario_leido->user_name) != 0)
                    {
                        printf("\n\n\t [ Informacion de usuario: %s ]\n", usuario_leido->user_name);
                        printf("--------------------------------------------");
                        char nombre_estado[40];
                        strcpy(nombre_estado, estado(usuario_leido->user_state));
                        printf("Usuario -> [%s] | IP -> [%s] | Estado -> [%s]\n\n", usuario_leido->user_name, usuario_leido->user_ip, nombre_estado);
                    }
                }
            }
            else
            {
                printf("\n\n ERROR > Usuario no encontrado\n\n");
            }
            break;
        }
        default:
            break;
        }

        printf("\nQue deseas hacer?: \n");
        chat__answer__free_unpacked(response_servidor, NULL);
    }

    close(client_socket);
}

char *GetIP()
{
    struct ifaddrs *ifaddr, *ifa;
    char *local_ip = NULL;

    // obtener lista de interfaces
    if (getifaddrs(&ifaddr) == -1)
    {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == NULL)
            continue;

        int family = ifa->ifa_addr->sa_family;

        if (family == AF_INET)
        {
            local_ip = malloc(INET_ADDRSTRLEN);
            inet_ntop(AF_INET, &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr, local_ip, INET_ADDRSTRLEN);
            break;
        }
    }

    freeifaddrs(ifaddr);
    return local_ip;
}

void PublicMessage(char *username, int option, int client_socket)
{
    printf("\n------------------------\nPublicMessage\n------------------------\n");
    // Chat with others
    char private[] = "0"; // Public message indicator
    char destination[] = "";
    char message_content[BUFFER_SIZE];

    // ingresar los datos
    printf("Esribe el mensaje: ");
    scanf(" %[^\n]", message_content); // Leer hasta que se le agreguen datos

    printf("\n\nDE [%s] PARA [%s]: %s\n", username, "grupo", message_content);
    // Enviar al servidor UserOption

    printf("\n\n");

    // Mensaje General

    Chat__Message mensaje_directo = CHAT__MESSAGE__INIT;
    mensaje_directo.message_private = '0';
    mensaje_directo.message_destination = destination;
    mensaje_directo.message_content = message_content;
    mensaje_directo.message_sender = username;

    Chat__UserOption opcion_escogida = CHAT__USER_OPTION__INIT;
    opcion_escogida.op = option;
    opcion_escogida.message = &mensaje_directo;

    // Serializando registro
    size_t serialized_size_opc = chat__user_option__get_packed_size(&opcion_escogida);
    uint8_t *buffer_opc = malloc(serialized_size_opc);
    chat__user_option__pack(&opcion_escogida, buffer_opc);

    // Enviar registro
    if (send(client_socket, buffer_opc, serialized_size_opc, 0) < 0)
    {
        perror("ERROR - No ha sido posible enviar el mensaje");
        exit(1);
    }
    free(buffer_opc);
    printf("\n\n");
}

void PrivateMessage(char *username, int action, int client_socket)
{
    printf("\n------------------------\nPrivateMessage\n------------------------\n");
    // Chat with others
    char private[] = "1"; // Public message indicator
    char destination[BUFFER_SIZE];
    char content[BUFFER_SIZE];

    // ingresar los datos
    printf("A quien desea enviar el mensaje?: ");
    scanf(" %[^\n]", destination); // Leer hasta que se le agreguen datos
    printf("Esribe el mensaje: ");
    scanf(" %[^\n]", content); // Leer hasta que se le agreguen datos

    printf("\n\nDE [%s] PARA [%s]: %s\n", username, destination, content);

    // Enviar al servidor UserOption

    printf("\n\n");

    // Mensaje Privado

    Chat__Message mensaje_directo = CHAT__MESSAGE__INIT;
    mensaje_directo.message_private = '1';
    mensaje_directo.message_destination = destination;
    mensaje_directo.message_content = content;
    mensaje_directo.message_sender = username;

    Chat__UserOption opcion_escogida = CHAT__USER_OPTION__INIT;
    opcion_escogida.op = action;
    opcion_escogida.message = &mensaje_directo;

    // Serializando registro
    size_t serialized_size_opc = chat__user_option__get_packed_size(&opcion_escogida);
    uint8_t *buffer_opc = malloc(serialized_size_opc);
    chat__user_option__pack(&opcion_escogida, buffer_opc);

    // Enviar registro
    if (send(client_socket, buffer_opc, serialized_size_opc, 0) < 0)
    {
        perror("Error al enviar el mensaje");
        exit(1);
    }
    free(buffer_opc);
    printf("\n\n");
}

void ReceiveMessage(char *username, int option, int client_socket)
{
    printf("\nReceiveMessage:\n");
}

void ChangeStatus(char *username, int action, int client_socket)
{
    printf("\n------------------------\nCambiar estado\n------------------------\n");
    // Change status
    char input_str;
    int state;
    int valid_input = 0;
    while (!valid_input)
    {
        valid_input = 1;

        printf("Escoge un estado: \n");
        printf("1. En linea\n");
        printf("2. Ocuapdo\n");
        printf("3. Desconectado\n");
        scanf(" %c", &input_str); // Read a char from input, skipping whitespace

        // Check if the input is a valid digit (1 to 3)
        if (isdigit(input_str) && input_str >= '1' && input_str <= '3')
        {
            state = input_str - '0'; // Convert the character to its corresponding integer value
        }
        else
        {
            printf("ERROR. Escoge una opcion valida.\n");
            valid_input = 0; // Set valid_input to false
        }
    }

    Chat__Status estatus = CHAT__STATUS__INIT;
    estatus.user_name = username;
    estatus.user_state = state;

    Chat__UserOption opcion_escogida = CHAT__USER_OPTION__INIT;
    opcion_escogida.op = action;
    opcion_escogida.status = &estatus;

    // Serializando registro
    size_t serialized_size_opc = chat__user_option__get_packed_size(&opcion_escogida);
    uint8_t *buffer_opc = malloc(serialized_size_opc);
    chat__user_option__pack(&opcion_escogida, buffer_opc);

    // Enviar registro
    if (send(client_socket, buffer_opc, serialized_size_opc, 0) < 0)
    {
        perror("ERROR: No se pudo enviar el mensaje");
        exit(1);
    }
    free(buffer_opc);
    printf("\n\n");

    // printf("\nstate: %i", state);
    // printf("\nsender: %s", username);
}

void UserList(char *username, int action, int client_socket)
{
    {
        char connectedUsers = 0;
    }

    Chat__UserList lista_usuarios = CHAT__USER_LIST__INIT;
    lista_usuarios.list = '1';

    Chat__UserOption opcion_escogida = CHAT__USER_OPTION__INIT;
    opcion_escogida.op = action;
    opcion_escogida.userlist = &lista_usuarios;

    // Serializando registro
    size_t serialized_size_opc = chat__user_option__get_packed_size(&opcion_escogida);
    uint8_t *buffer_opc = malloc(serialized_size_opc);
    chat__user_option__pack(&opcion_escogida, buffer_opc);

    // Enviar registro
    if (send(client_socket, buffer_opc, serialized_size_opc, 0) < 0)
    {
        perror("ERROR > La comunicacion con el socket ha fallado");
        exit(1);
    }

    free(buffer_opc);
    printf("\n\n");
}

void UserInfo(char *userInput, int action, int client_socket)
{

    printf("UserInformation: %s", userInput);

    Chat__UserList lista_usuarios = CHAT__USER_LIST__INIT;
    lista_usuarios.list = '0';
    lista_usuarios.user_name = userInput;

    Chat__UserOption opcion_escogida = CHAT__USER_OPTION__INIT;
    opcion_escogida.op = action;
    opcion_escogida.userlist = &lista_usuarios;

    // Serializando registro
    size_t serialized_size_opc = chat__user_option__get_packed_size(&opcion_escogida);
    uint8_t *buffer_opc = malloc(serialized_size_opc);
    chat__user_option__pack(&opcion_escogida, buffer_opc);

    // Enviar registro
    if (send(client_socket, buffer_opc, serialized_size_opc, 0) < 0)
    {
        perror("ERROR -> La comunicacion con el socket ha fallado");
        exit(1);
    }

    free(buffer_opc);
    printf("\n\n");
}

void RegistrationHandler()
{
}

int main(int argc, char **argv)
{

    if (argc != 4)
    {
        printf("Ingrese correctamente los parametros: %s <usuario> <ip_servidor> <puerto_servidor> \n", argv[0]);
        exit(1);
    }

    // EXit on ctrl+c
    signal(SIGINT, abortAction);

    char *username = argv[1];
    char *server_ip = argv[2];
    int server_port = atoi(argv[3]);

    int action = 0;
    // Creamos un socket para el cliente
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0)
    {
        perror("ERROR > No se ha podido crear el socket");
        exit(1);
    }

    // Conectamos el socket al servidor
    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(server_port);
    inet_pton(AF_INET, server_ip, &server_address.sin_addr);

    if (connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        perror("ERROR > No se ha podido conectar con el servidor");
        exit(1);
    }

    printf("\nCONEXION EXITOSA CON EL SERVIDOR\n");
    printf(">> IP del servidor: %s\n>> Puerto del servidor: %d\n", server_ip, server_port);

    // Registramos
    Chat__NewUser registration = CHAT__NEW_USER__INIT;
    registration.username = username;
    char *ip = GetIP();
    registration.ip = ip;

    Chat__UserOption user_option_registration = CHAT__USER_OPTION__INIT;
    user_option_registration.op = action;
    user_option_registration.createuser = &registration;

    // Serializamos el registro
    size_t serialized_size_registration = chat__user_option__get_packed_size(&user_option_registration);
    uint8_t *buffer_registration = malloc(serialized_size_registration);
    chat__user_option__pack(&user_option_registration, buffer_registration);

    // Enviar registro
    if (send(client_socket, buffer_registration, serialized_size_registration, 0) < 0)
    {
        perror("ERROR > La comunicacion con el socket ha fallado");
        exit(1);
    }

    free(buffer_registration);
    free(ip);

    // Recibir un buffer del socket
    uint8_t recv_buffer[BUFFER_SIZE];
    ssize_t recv_size = recv(client_socket, recv_buffer, sizeof(recv_buffer), 0);
    if (recv_size < 0)
    {
        perror("ERROR > No se pudo obtener una respuesta");
        exit(1);
    }

    // Deserializar el buffer en un mensaje Message
    Chat__Answer *response = chat__answer__unpack(NULL, recv_size, recv_buffer);

    if (response->response_status_code == 400)
    {
        printf(">> USUARIO [%s] %s\n\n", username, response->response_message);
    }
    else
    {
        // cliente_interrumpido:
        printf(">> USUARIO [%s] %s\n\n", username, response->response_message);
        action = 7;

        Chat__UserOption finalizar = CHAT__USER_OPTION__INIT;
        finalizar.op = action;

        // Serializando registro
        size_t serialized_size_finalizar = chat__user_option__get_packed_size(&finalizar);
        uint8_t *buffer_finalizar = malloc(serialized_size_finalizar);
        chat__user_option__pack(&finalizar, buffer_finalizar);

        // Enviar registro
        if (send(client_socket, buffer_finalizar, serialized_size_finalizar, 0) < 0)
        {
            perror("ERROR > La comunicacion con el socket ha fallado");
            exit(1);
        }
        free(buffer_finalizar);
        printf("\n\n");
    }

    // Liberar los buffers y deserializamos el mensaje
    chat__answer__free_unpacked(response, NULL);

    pthread_t thread;
    if (pthread_create(&thread, NULL, client_listening, (void *)&client_socket) < 0)
    {
        perror("ERROR > No se pudo crear el thread");
        exit(1);
    }

    printf("----------------------------------------------------\n");
    printf("\t\tHola '%s'\n", username);
    printf("\tBienvenido a nuestro increible CHAT\n");

    while (action != 7)
    {
        printf("----------------------------------------------------\n");
        printf("\t\tMENU\n");
        printf("----------------------------------------------------\n");
        printf("1. Enviar mensaje publico\n");
        printf("2. Enviar mensaje privado\n");
        printf("3. Cambiar estado de sesion\n");
        printf("4. Mostrar usuarios conectados\n");
        printf("5. Buscar informacion de usuario\n");
        printf("6. Ayuda\n");
        printf("7. Salir\n");
        printf("Que deseas hacer?: ");
        scanf("%d", &action);

        switch (action)
        {
        // Enviar mensaje publico
        case 1:
        {
            PublicMessage(username, action, client_socket);
        }
        break;
        // ENviar mensaje privado
        case 2:
        {
            PrivateMessage(username, action, client_socket);
        }
        break;
        // CAmbiar estado
        case 3:
        {
            ChangeStatus(username, action, client_socket);
        }
        break;
        // Mostrar usuarios conectados
        case 4:
        {
            UserList(username, action, client_socket);
        }
        break;
        // Mostrar informacion de usuario
        case 5:
        {
            char username[BUFFER_SIZE];

            // Solicitar username a buscar
            printf("A que usuario deseas stalekar? ");
            scanf(" %[^\n]", username);

            UserInfo(username, action, client_socket);
        }

        break;
        case 6:
        {
            Chat__UserOption ayuda = CHAT__USER_OPTION__INIT;
            ayuda.op = action;

            // Serializando registro
            size_t serialized_size_ayuda = chat__user_option__get_packed_size(&ayuda);
            uint8_t *buffer_ayuda = malloc(serialized_size_ayuda);
            chat__user_option__pack(&ayuda, buffer_ayuda);

            // Enviar registro
            if (send(client_socket, buffer_ayuda, serialized_size_ayuda, 0) < 0)
            {
                perror("ERROR > La comunicacion con el socket ha fallado");
                exit(1);
            }
            free(buffer_ayuda);

            printf("\n\tAYUDA\n\n");
            printf("<<< Aca tienes una guia simple de como utilizar nuestro chat >>>\n");
            printf("Opción 1: Enviar mensaje a todos los usuarios\n");
            printf("Opción 2: Chatear en privado con un usuario\n");
            printf("\t- Primero escribe el nombre del usuario\n");
            printf("\t- Luego escribe el mensaje\n");
            printf("Opción 3: Cambiar estado de conexión\n");
            printf("Opción 4: Mostrar usuarios conectados\n");
            printf("Opción 5: Mostrar información de un usuario\n");
            printf("Opción 7: Salir del chat\n\n");
        }
        break;
        case 7:
            printf("\n\n");
            Chat__UserOption finalizar = CHAT__USER_OPTION__INIT;
            finalizar.op = action;

            size_t serialized_size_finalizar = chat__user_option__get_packed_size(&finalizar);
            uint8_t *buffer_finalizar = malloc(serialized_size_finalizar);
            chat__user_option__pack(&finalizar, buffer_finalizar);

            if (send(client_socket, buffer_finalizar, serialized_size_finalizar, 0) < 0)
            {
                perror("ERROR > La comunicacion con el socket ha fallado");
                exit(1);
            }
            free(buffer_finalizar);
            printf("--------------------------------------------\n");
            printf("  Fue un gusto %s! Vuelve pronto...\n", username);
            printf("--------------------------------------------\n");
            break;
        default:
            printf("ERROR > Ingresa una opcion valida.\n");
            break;
        }
    }

    close(client_socket);

    return 0;
}