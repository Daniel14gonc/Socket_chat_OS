#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#define PORT 8080

//ProtoBuff
#include "project.pb.h"

using namespace std;

int main(){

    GOOGLE_PROTOBUF_VERIFY_VERSION;

    chat::newMessage newMessage;
    chat::UserRequest UserRequest;


    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};
    const char* hello = "Hello from server";

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0){
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))){
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( PORT );

    // Forcefully attaching socket to the port 8080
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0){
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0){
        perror("listen");
        exit(EXIT_FAILURE);
    }

    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0){
        perror("accept");
        exit(EXIT_FAILURE);
    }

    valread = read(new_socket , buffer, 1024);

    string request = (string) buffer;
    UserRequest.ParseFromString(request);

    printf("La opcion es %d \n",UserRequest.option());

    // int option = 1;

    // switch (option)
    // {
    // case 1:
    //     printf("Registro de Usuario\n");
    //     break;
    
    // case 2:
    //     printf("Informacion de usuario\n");
    //     break;
    // case 3:
    //     printf("Cambio de status\n");
    //     break;
    // case 4:
    //     printf("Nuevo mensaje\n");
    //     break;
    // case 5:
    //     printf("Hearbeat\n");
    //     break;
    // default:
    //     break;
    // }

    send(new_socket , hello , strlen(hello) , 0 );
    printf("Hello message sent\n");

    close(new_socket);
    shutdown(server_fd, SHUT_RDWR);

    return 0;
}