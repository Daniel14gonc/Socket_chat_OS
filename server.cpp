#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#define PORT 8080
#define CLIENT_BUFFER_SIZE 3072

//ProtoBuff
#include "project.pb.h"

using namespace std;
using namespace chat;

int main(){

    GOOGLE_PROTOBUF_VERIFY_VERSION;

    
    UserRequest UserRequest;
    
    // User Request
    newMessage userMessage;
    UserRegister userRegister;
    UserInfoRequest userInfoRequest;
    ChangeStatus changeStatus;



    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[CLIENT_BUFFER_SIZE] = {0};
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

   /*ssize_t nbytes;
    string request = "";
    bool finished = false;

    while((nbytes = read(new_socket, buffer, 1)) > 0) {
        if (buffer[0] != '\0') {
            printf("Read f\n");
            request += buffer[0];
        }
    }
    */ 

    valread = read(new_socket , buffer, CLIENT_BUFFER_SIZE - 1);
    buffer[valread] = '\0';

    string request = (string) buffer;
    UserRequest.ParseFromArray(buffer, CLIENT_BUFFER_SIZE);
        

    int option = UserRequest.option();
    
    switch (option)
    {
    case 1:// Registro de Usuarios
        userRegister = UserRequest.newuser();
        cout << "El username es: " << userRegister.username() << endl;
        cout << "El ip es: " << userRegister.ip() << endl;

        break;
    
    case 2:// Informacion de usuario
        userInfoRequest = UserRequest.inforequest(); 
        cout << "El tipo de request es: " << userInfoRequest.type_request() << endl; 
        cout << "El usuario es: " << userInfoRequest.user() << endl; 

        printf("Informacion de usuario\n");
        break;
    case 3:// Cambio de status
        changeStatus = UserRequest.status();
        cout << "El usuario es: " << changeStatus.username() << endl;
        cout << "El nuevo status es: " << changeStatus.newstatus() << endl;
        break;
    case 4://Nuevo mensaje
        userMessage = UserRequest.message();
        cout << "El tipo de mensaje es: " << userMessage.message_type() << endl;
        cout << "El emisor es: " << userMessage.sender() << endl;
        cout << "El receptor es: " << userMessage.recipient() << endl;
        cout << "El mensaje es: " << userMessage.message() << endl;


        
        break;
    case 5://Heartbeat
        printf("Hearbeat\n");
        break;
    default:
        break;
    }



    send(new_socket , hello , strlen(hello) , 0 );
    printf("Hello message sent\n");

    close(new_socket);
    shutdown(server_fd, SHUT_RDWR);

    return 0;
}