#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

void PublicMessage()
{
    printf("\nPublicMessage:\n");
}

void PrivateMessage()
{
    printf("\nPrivateMessage:\n");
}

void ReceiveMessage()
{
    printf("\nReceiveMessage:\n");
}

void ChangeStatus()
{
    printf("\nChangeStatus:\n");
}

void ListUsers()
{
    printf("\nListUsers:\n");
}

void UserInfo()
{
    printf("\nUserInfo:\n");
}

void Help()
{
    printf("\nHelp:\n");
}

void MenuHandler()
{

    while (1)
    {
        int choice;
        // Print Menu
        printf("\nMenu:\n");
        printf("1. Chatear con todos los usuarios (broadcasting).\n");
        printf("2. Enviar mensajes directos, privados, aparte del chat general.\n");
        printf("3. Cambiar de status.\n");
        printf("4. Listar los usuarios conectados al sistema de chat.\n");
        printf("5. Desplegar información de un usuario en particular.\n");
        printf("6. Ayuda.\n");
        printf("7. Salir.\n");
        scanf("%d", &choice);

        switch (choice)
        {
        case 1:
            // Send Broadcast Message
            PublicMessage();
            break;
        case 2:
            // Send Private Message
            PrivateMessage();
            break;
        case 3:
            // Change user status in the server
            ChangeStatus();
            break;
        case 4:
            // Get connected user lis from server
            ListUsers();
            break;
        case 5:
            // Get user information from server
            UserInfo();
            break;
        case 6:
            // Help Message
            printf("Este es un chat generado con C y Protobuf para comunicarte por medio de\nun servidor con tus amigos. A continuación se te mostrará las funciones \nque puedes realizar en el chat. Para salir del chat ingresa la opción 7.\n");
            break;
        case 7:
            // Exit form menu
            printf("¡Gracias por usar el chat!\n");
            break;
        default:
            // Error message
            printf("Wrong Choice. Enter again\n");
            break;
        }
    }
}

int main(int argc, char const *argv[])
{
    int sock = 0;
    struct sockaddr_in serv_addr;
    char name[20]; // declarar una variable para almacenar el nombre del cliente

    if (argc < 2)
    {
        fprintf(stderr, "Error: Debe proporcionar la dirección IP del servidor como argumento.\n");
        exit(1);
    }

    // Crear un socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\nError al crear el socket\n");
        return -1;
    }

    // Especificar la dirección y el puerto del servidor al que se conectará el cliente
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8080);

    // Convertir la dirección IP de texto a binario
    if (inet_pton(AF_INET, argv[1], &serv_addr.sin_addr) <= 0)
    {
        printf("\nDirección inválida/ no soportada\n");
        return -1;
    }

    // Conectar el socket del cliente con el servidor
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\nError al conectar con el servidor\n");
        return -1;
    }

    printf("Conexión establecida con el servidor\n");

    // Pedir al usuario que ingrese su nombre
    printf("Ingrese su nombre: ");
    fgets(name, sizeof(name), stdin);
    name[strcspn(name, "\n")] = 0; // eliminar el carácter de nueva línea agregado por fgets

    // Enviar el nombre al servidor
    if (send(sock, name, strlen(name), 0) < 0)
    {
        printf("\nError al enviar el nombre al servidor\n");
        return -1;
    }

    // Esperar una respuesta del servidor
    char buffer[1024] = {0};
    if (recv(sock, buffer, 1024, 0) < 0)
    {
        printf("\nError al recibir la respuesta del servidor\n");
        return -1;
    }

    printf("%s\n", buffer);

    // Cerrar el socket del cliente
    close(sock);
    return 0;
}