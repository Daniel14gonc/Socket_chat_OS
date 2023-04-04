#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <list>

#define PORT 8080
#define CLIENT_BUFFER_SIZE 3072

//ProtoBuff
#include "project.pb.h"

using namespace std;
using namespace chat;

struct User {
    string username;
    string ip;
    int socketFD;
    int status;
};

struct threadInfo {
    int socketFD;
};
 

list<User> connectedUsers;


int isUserConnected(string username, string ip) {
    for (User user : connectedUsers) {
        if (user.username == username || user.ip == ip) {
            return 1;
        }
    }

    return 0;
}

int deleteUser(string usernamef, string ipf) {

    auto it = std::find_if(connectedUsers.begin(), connectedUsers.end(), [usernamef,ipf](const User& user) { return (usernamef == user.username && ipf == user.ip); });
    if (it != connectedUsers.end()) {
        connectedUsers.erase(it);
    }
    cout << "User " << usernamef << " " << ipf << " disconnected." << endl;
    return 0;
}

void generalMessage(newMessage userMessage, ServerResponse serverResponse) {
     serverResponse.mutable_message()->CopyFrom(userMessage);
    serverResponse.set_option(4);
    serverResponse.set_servermessage("Mensaje enviado");
    string response = serverResponse.SerializeAsString();
    
    for (User user : connectedUsers) {
        send(user.socketFD , response.c_str() , response.size() , 0 );
    }
}

void directMessage(newMessage userMessage, ServerResponse serverResponse) {
    string username = userMessage.recipient();
     serverResponse.mutable_message()->CopyFrom(userMessage);
    serverResponse.set_option(4);
    serverResponse.set_code(200);
    serverResponse.set_servermessage("Mensaje enviado");
    string response = serverResponse.SerializeAsString();
    bool userFound = false;
    for (User user : connectedUsers) {
        if (user.username == username) {
            userFound = true;
            send(user.socketFD , response.c_str() , response.size() , 0 );
        }
    }

    if (!userFound) {
        serverResponse.set_code(400);
        serverResponse.set_servermessage("User no found.");
    }
}

void* connectionHandler(void* arg) {
    int valread;
    char buffer[CLIENT_BUFFER_SIZE] = {0};
    const char* hello = "Hello from server";
    UserRequest userRequest;

    // User Request
    newMessage userMessage;
    UserRegister userRegister;
    UserInfoRequest userInfoRequest;
    ChangeStatus changeStatus;

    // Server Response
    ServerResponse serverResponse;

    struct threadInfo* data = (struct threadInfo*)arg;

    User user;

    AllConnectedUsers allConnectedUsers;

    UserInfo userInfo;
    


    // Se obtiene el socket_fd de la estructura
    int new_socket = data->socketFD;

    bool notClosed = true;

    while (notClosed)
    {
        cout << "Server: Waiting for request..." << endl;

        valread = read(new_socket , buffer, CLIENT_BUFFER_SIZE - 1);
        buffer[valread] = '\0';

        string request = (string) buffer;
        userRequest.ParseFromArray(buffer, CLIENT_BUFFER_SIZE);
            

        int option = userRequest.option();
        cout << option << endl;

        string response;
        
        switch (option)
        {
        case 1:// Registro de Usuarios
            userRegister = userRequest.newuser();
            cout << "El username es: " << userRegister.username() << endl;
            cout << "El ip es: " << userRegister.ip() << endl;


            if (isUserConnected(userRegister.username(), userRegister.ip()) == 0) {
                printf("Usuario registrado\n");
                serverResponse.set_option(1);
                serverResponse.set_code(200);
                serverResponse.set_servermessage("Usuario registrado");

                user.username = userRegister.username();
                user.ip =userRegister.ip();
                user.socketFD = new_socket;
                user.status = 1;
                connectedUsers.push_back(user);

            } else {
                printf("Usuario ya registrado\n");
                serverResponse.set_option(1);
                serverResponse.set_code(400);
                serverResponse.set_servermessage("Usuario ya existente");
            }
            
            response = serverResponse.SerializeAsString();
            
            break;
        
        case 2:// Informacion de usuario
            userInfoRequest = userRequest.inforequest(); 
            cout << "El tipo de request es: " << userInfoRequest.type_request() << endl; 
            cout << "El usuario es: " << userInfoRequest.user() << endl; 

            AllConnectedUsers* allConnectedUsers;
            if (userInfoRequest.type_request()) {
                allConnectedUsers = new AllConnectedUsers;
                printf("Informacion de todos los usuarios\n");
                serverResponse.set_option(2);
                serverResponse.set_code(200);
                serverResponse.set_servermessage("Informacion de todos los usuarios");
                for (User user : connectedUsers) {
                    UserInfo userInfo;
                    userInfo.set_username(user.username);
                    userInfo.set_ip(user.ip);
                    userInfo.set_status(user.status);
                    allConnectedUsers->add_connectedusers()->CopyFrom(userInfo);
                }

                serverResponse.set_allocated_connectedusers(allConnectedUsers);


            } else {
                allConnectedUsers = new AllConnectedUsers;
                printf("Informacion de un usuario\n");
                serverResponse.set_option(2);
                serverResponse.set_code(200);
                serverResponse.set_servermessage("Informacion de un usuario");
                
                bool userFound = false;
                for (User user : connectedUsers) {
                    if (user.username == userInfoRequest.user()) {
                        userFound = true;
                        userInfo.set_username(user.username);
                        userInfo.set_ip(user.ip);
                        userInfo.set_status(user.status);
                        allConnectedUsers->add_connectedusers()->CopyFrom(userInfo);
                        serverResponse.set_allocated_connectedusers(allConnectedUsers);
                    }
                }

                if (!userFound) {
                        serverResponse.set_code(400);
                        serverResponse.set_servermessage("Usuario no encontrado");
                    }

            }
            response = serverResponse.SerializeAsString();
            // delete allConnectedUsers;
            
            break;
        case 3:// Cambio de status
            changeStatus = userRequest.status();
            cout << "El usuario es: " << changeStatus.username() << endl;
            cout << "El nuevo status es: " << changeStatus.newstatus() << endl;

            bool userFound = false;
            for (User user : connectedUsers) {
                if (user.username == changeStatus.username()) {
                    user.status = changeStatus.newstatus();
                    userFound = true;
                }
            }

            if (userFound) {
                serverResponse.set_option(3);
                serverResponse.set_code(200);
                serverResponse.set_servermessage("Status cambiado");
            } else {
                serverResponse.set_option(3);
                serverResponse.set_code(400);
                serverResponse.set_servermessage("Usuario no encontrado");
            }   

            response = serverResponse.SerializeAsString();

            break;
        case 4://Nuevo mensaje
            userMessage = userRequest.message();
            if (userMessage.message_type()) {
                generalMessage(userMessage, serverResponse);
            }
            else {
                directMessage(userMessage, serverResponse);
            }
            // cout << "El tipo de mensaje es: " << userMessage.message_type() << endl;
            // cout << "El emisor es: " << userMessage.sender() << endl;
            // cout << "El receptor es: " << userMessage.recipient() << endl;
            // cout << "El mensaje es: " << userMessage.message() << endl;
            break;
        case 5://Heartbeat
            printf("Hearbeat\n");
            break;
        default:
            notClosed = false;
            break;
        }



        send(new_socket , response.c_str() , response.size() , 0 );
        printf("Hello message sent\n");
    }
    cout << "Closing user connection gracefully..." << endl;
    deleteUser(user.username, user.ip);
    close(new_socket);
    pthread_exit(0);
}

int main(){
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

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

    while (true) {
        cout << "Waiting for new connections..." << endl;
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0){
            perror("accept");
            exit(EXIT_FAILURE);
        }
        else {
            pthread_t thread;
            struct threadInfo* data = new threadInfo;
            data->socketFD = new_socket;
            pthread_create(&thread, NULL, connectionHandler, (void *) data);
        }
    }
    shutdown(server_fd, SHUT_RDWR);
    return 0;

}