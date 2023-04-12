/*
*   Autores: Sebastian Aristondo, Juan Angel Carrera, Daniel González
*   Descripción:
*           Programa que funge como servidor para el manejo de conexiones por medio de sockets
*           de forma que se puedan manejar mensajes entre usuarios y otras funcionalidades.
*/




#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <list>
#include <fcntl.h>
#include <chrono>
#include <arpa/inet.h>

#define PORT 8080
#define CLIENT_BUFFER_SIZE 3072

//ProtoBuff
#include "project.pb.h"

using namespace std;
using namespace chat;
using namespace std::chrono;

pthread_mutex_t mutexP;

// Estrucutra para almacenar la información de los usuarios conectados
struct User {
    string username;
    string ip;
    int socketFD;
    int status;
};

// Estrucutra para almacenar informacion del socket
struct threadInfo {
    int socketFD;
};

// Estructura para almacenar el estado del usuario
struct ActivityInfo {
    int socketFD;
    steady_clock::time_point* start_time;
    bool* isActive;
};
 
// Listado de usuarios
list<User> connectedUsers;

//Verificar si un usuario esta conectado
int isUserConnected(string username, string ip) {
    for (User user : connectedUsers) {
        if (user.username == username && user.ip == ip) {
            return 1;
        }
    }

    return 0;
}

// Elimimnar a un usuario de la lista de usuarios conectados
int deleteUser(string usernamef, string ipf) {

    auto it = std::find_if(connectedUsers.begin(), connectedUsers.end(), [usernamef,ipf](const User& user) { return (usernamef == user.username && ipf == user.ip); });
    if (it != connectedUsers.end()) {
        connectedUsers.erase(it);
    }
    cout << "User " << usernamef << " " << ipf << " disconnected." << endl;
    return 0;
}

// Función para el manejo de mensajes generales
void generalMessage(newMessage userMessage) {
    ServerResponse serverResponse;
    serverResponse.mutable_message()->CopyFrom(userMessage);
    serverResponse.set_option(4);
    serverResponse.set_code(200);
    serverResponse.set_servermessage("Mensaje enviado");
    string response = serverResponse.SerializeAsString();

    for (User user : connectedUsers) {
        if (user.status != 2)
            send(user.socketFD , response.c_str() , response.size() , 0 );
    }
}

// Funcion para el manejo de mensajes directos
bool directMessage(newMessage userMessage) {
    ServerResponse serverResponse;
    string username = userMessage.recipient();

    cout << "Mensaje directo a: " << username << endl;
    cout << "Mensaje: " << userMessage.message() << endl;
    cout << "Emisor: " << userMessage.sender() << endl;

    serverResponse.mutable_message()->CopyFrom(userMessage);
    serverResponse.set_option(4);
    serverResponse.set_code(200);
    serverResponse.set_servermessage("Mensaje enviado");
    string response = serverResponse.SerializeAsString();
    bool userFound = false;
    for (User user : connectedUsers) {
        if (user.username == username && user.status != 2) {
            cout << "mensaje enviado a " << user.username << endl;
            userFound = true;
            send(user.socketFD , response.c_str() , response.size() , 0 );
        }
    }

    return userFound;
}

// Funcion para el manejo de mensajes de estado por inactividad
void on_timeout(int fd) {
    for (auto& user : connectedUsers) {
        if (user.socketFD == fd) {
            cout << "Status change to inactive for user: " << user.username << endl;
            user.status = 3;
        }
    }
}

// Funcion para controlar el tiempo de inactividad de los usuarios
void* checkActivity (void* arg) {
    struct ActivityInfo* info = (struct ActivityInfo*)arg;
    duration<int> time_limit = seconds(30);

    while (true) {
        steady_clock::time_point current_time = steady_clock::now();
        duration<int> elapsed_time = duration_cast<seconds>(current_time - *info->start_time);

        // Comprueba si se ha alcanzado el límite de tiempo
        if (elapsed_time >= time_limit && (*info->isActive)) {
            on_timeout(info->socketFD);
            // Reinicia el temporizador estableciendo un nuevo tiempo de referencia
            *info->start_time = steady_clock::now();
            *info->isActive = false;
        }
    }
}


// Funcion para reactivar usuarios inactivos
void activeUser(int fd, bool* isActive) {
    if (!(*isActive)) {
        for (auto& user : connectedUsers) {
            if (user.socketFD == fd) {
                cout << "Status change to active for user: " << user.username << endl;
                user.status = 1;
            }
        }
        *isActive = true;
    }
}

// Funcion para el manejo de las peticiones de los clientes
void* connectionHandler(void* arg) {
    
    struct threadInfo* data = (struct threadInfo*)arg;

    // Se obtiene el socket_fd de la estructura
    int new_socket = data->socketFD;

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

    User user;

    AllConnectedUsers allConnectedUsers;

    UserInfo userInfo;

    pthread_t activityThread;

    bool* isActive = new bool;
    *isActive = true;

    // Se crea un timer para que cada 30 segundos, si el usuario no ha hecho nada, se cambie su estado a inactivo.
    steady_clock::time_point start_time = steady_clock::now();
    steady_clock::time_point* ptr_start_time = &start_time;

    struct ActivityInfo* info = new ActivityInfo;
    info->socketFD = new_socket;
    info->start_time = ptr_start_time;
    info->isActive = isActive;

    pthread_create(&activityThread, NULL, checkActivity, (void *) info);

    bool notClosed = true;

    // Mientras la conexión sea válida
    while (notClosed)
    {
        cout << "Server: Waiting for request..." << endl;

        valread = read(new_socket , buffer, CLIENT_BUFFER_SIZE - 1);
        buffer[valread] = '\0';

        if (valread <= 0) {
            break;
        }

        // Validacion de la implementacion del proto
        try {
            string request = (string) buffer;
            userRequest.ParseFromArray(buffer, CLIENT_BUFFER_SIZE);
                
            int option = userRequest.option();
            cout << option << endl;

            string response;
            bool userFound = false;

            if (option != 0 || option != 5) {
                *ptr_start_time = steady_clock::now();
                activeUser(new_socket, isActive);
            }
            
            //manejo de opciones
            switch (option)
            {
            case 1:// Registro de Usuarios
                userRegister = userRequest.newuser();
                cout << "El username es: " << userRegister.username() << endl;
                cout << "El ip es: " << userRegister.ip() << endl;


                if (isUserConnected(userRegister.username(), userRegister.ip()) == 0) {
                    printf("Nuevo usuario registrado\n");
                    serverResponse.set_option(1);
                    serverResponse.set_code(200);
                    serverResponse.set_servermessage("Usuario registrado");

                    user.username = userRegister.username();
                    user.ip =userRegister.ip();
                    user.socketFD = new_socket;
                    user.status = 1;
                    pthread_mutex_lock(&mutexP);
                    connectedUsers.push_back(user);
                    pthread_mutex_unlock(&mutexP);

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
                    printf("Informacion de un usuario\n");
                    serverResponse.set_option(2);
                    serverResponse.set_code(200);
                    serverResponse.set_servermessage("Informacion de un usuario");
                    serverResponse.clear_connectedusers();
                    userFound = false;
                    for (User user : connectedUsers) {
                        if (user.username == userInfoRequest.user()) {
                            userFound = true;
                            //userInfo = new UserInfo;
                            //UserInfo* userInfo = serverResponse.add_userinforesponse();
                            userInfo.set_username(user.username);
                            userInfo.set_ip(user.ip);
                            userInfo.set_status(user.status);
                            //allConnectedUsers->add_connectedusers()->CopyFrom(userInfo);r
                            serverResponse.mutable_userinforesponse()->CopyFrom(userInfo);
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

                userFound = false;
                for (auto& user : connectedUsers) {
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
                bool responseMessage;
                responseMessage = true;
                if (userMessage.message_type()) {
                    generalMessage(userMessage);
                }
                else {
                    responseMessage = directMessage(userMessage);
                }
                if (responseMessage) {
                    serverResponse.set_code(200);
                    serverResponse.set_option(4);
                    serverResponse.set_servermessage("Mensaje enviado correctamente.");
                }
                else {
                    serverResponse.set_code(400);
                    serverResponse.set_option(4);
                    serverResponse.set_servermessage("Error: el mensaje no se pudo enviar.");
                }

                response = serverResponse.SerializeAsString();
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
        catch (const exception& e) {
            serverResponse.set_option(1);
            serverResponse.set_code(400);
            serverResponse.set_servermessage("Error en request");
            string response = serverResponse.SerializeAsString();
            send(new_socket , response.c_str() , response.size() , 0 );
            continue;
        }

        
    }
    // Cierra el socket
    cout << "Closing user connection gracefully..." << endl;
    deleteUser(user.username, user.ip);
    close(new_socket);

    pthread_cancel(activityThread);
    pthread_join(activityThread, NULL);

    pthread_exit(0);
}

int main(int argc, char** argv){
    if (argc <= 1) {
        cout << "Error ingrese el puerto." << endl;
        return -1;
    }
    int port = stoi(argv[1]);
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    pthread_mutex_init(&mutexP, NULL);

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
    address.sin_port = htons( port );

    // Forcefully attaching socket to the port 8080
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0){
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Se indica al socket que escuche por nuevas conexiones.
    if (listen(server_fd, 3) < 0){
        perror("listen");
        exit(EXIT_FAILURE);
    }

    while (true) {
        cout << "Waiting for new connections..." << endl;
        // Se maneja una nueva conexión
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0){
            perror("accept");
            exit(EXIT_FAILURE);
        }
        else {
            // Si la conexión es aceptada, se crea un nuevo thread que maneja la sesión del usuario.
            pthread_t thread;

            // Se envía el file descriptor de la conexión para poder identificar que usuario conectado se está comunicando.
            struct threadInfo* data = new threadInfo;
            data->socketFD = new_socket;
            pthread_create(&thread, NULL, connectionHandler, (void *) data);
        }
    }
    shutdown(server_fd, SHUT_RDWR);
    pthread_mutex_destroy(&mutexP);
    return 0;

}