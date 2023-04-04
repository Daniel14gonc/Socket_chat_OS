#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>

//ProtoBuff
#include "project.pb.h"

#define PORT 8080
#define CLIENT_BUFFER_SIZE 3072

using namespace std;
using namespace chat;

struct User {
    string username;
    string ip;
    int socketFD;
    int status;
};

void processAllConnectedUsers(const AllConnectedUsers& allUsers) {
    // Iterar a través de todos los usuarios conectados en el campo connectedUsers
    for (const auto& userInfo : allUsers.connectedusers()) {
        // Acceder a las propiedades de cada usuario conectado (por ejemplo, el nombre de usuario y la dirección IP)
        cout << "Usuario: " << userInfo.username() << ", IP: " << userInfo.ip() << ", Estado: " << userInfo.status() << endl;
    }
}


int main() {
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    int clientDescriptor = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serverAddress;
    int status;
    char buffer[CLIENT_BUFFER_SIZE];

    UserRequest userRequest;
    UserRegister* registerUser = userRequest.mutable_newuser();
    
    userRequest.set_option(1);
    
    registerUser->set_ip("127.0.0.1");
    registerUser->set_username("user");

    ServerResponse serverResponse;

    string request = userRequest.SerializeAsString();
    
    if (clientDescriptor < 0) {
        perror("socket creation error");
        return -1;
    }

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serverAddress.sin_addr) <= 0) 
    {
        printf("\n Invalid address/ Address not supported \n");
        return -1;
    }

    if ((status = connect(clientDescriptor, (struct sockaddr*)&serverAddress, sizeof(serverAddress))) < 0) {
        printf("\nConnection failed2\n");
        return -1;
    }
    write(clientDescriptor, request.c_str(), request.size());
    cout << "Requesting connection to server..." << endl;

    int valread = read(clientDescriptor, buffer, CLIENT_BUFFER_SIZE);
    
    buffer[valread] = '\0';

    serverResponse.ParseFromArray(buffer, CLIENT_BUFFER_SIZE);
    int option = serverResponse.option();
    int conectioncode = serverResponse.code();
    string message = serverResponse.servermessage();
    
    cout << "Requesting connection to server..." << endl;
    cout << "Server response: " << message << endl;

    // 1. Obtener listado de usuarios conectados
    // 2. Obtener inforamcion de usuario especifico
    // 3. Cambio de status de usuario
    // 4. Mandar mensaje directo
    // 5. Mandar mensaje general


    if (conectioncode == 200){
        int bandera =0;
        while (bandera ==0 ){
            cout << "1. Obtener listado de usuarios conectados" << endl;
            cout << "2. Obtener informacion de usuario especifico" << endl;
            cout << "3. Cambio de status de usuario" << endl;
            cout << "4. Mandar mensaje directo" << endl;
            cout << "5. Mandar mensaje general" << endl;
            cout << "6. Salir" << endl;
            int opcion;
            cin >> opcion;
            switch (opcion){
                case 1:{
                    UserRequest userRequest;
                    userRequest.set_option(2);

                    UserInfoRequest* listInfoRequest = userRequest.mutable_inforequest();
                                        
                    listInfoRequest->set_type_request(true);

                    string request = userRequest.SerializeAsString();

                    write(clientDescriptor, request.c_str(), request.size());
                    int valread = read(clientDescriptor, buffer, CLIENT_BUFFER_SIZE);
                    buffer[valread] = '\0';
                    serverResponse.ParseFromArray(buffer, CLIENT_BUFFER_SIZE);
                    AllConnectedUsers allConectedUsers = serverResponse.connectedusers();
                    int code = serverResponse.code();
                    string message = serverResponse.servermessage();
                    cout << "Server response: " << message << endl;
                
                    if (code == 200){
                        cout << "Connected users:" << endl;
                        processAllConnectedUsers(allConectedUsers);
                    }
                    break;
                }
                case 2:{
                    UserRequest userRequest;
                    userRequest.set_option(2);

                    UserInfoRequest* listInfoRequest = userRequest.mutable_inforequest();
                                        
                    listInfoRequest->set_type_request(false);
                    listInfoRequest->set_user("user");

                    string request = userRequest.SerializeAsString();

                    write(clientDescriptor, request.c_str(), request.size());
                    int valread = read(clientDescriptor, buffer, CLIENT_BUFFER_SIZE);
                    buffer[valread] = '\0';
                    serverResponse.ParseFromArray(buffer, CLIENT_BUFFER_SIZE);
                    AllConnectedUsers allConectedUsers = serverResponse.connectedusers();
                    int code = serverResponse.code();
                    string message = serverResponse.servermessage();
                    cout << "Server response: " << message << endl;
                    
                    if (code == 200){
                        processAllConnectedUsers(allConectedUsers);
                    }
                    break;
                }
                case 3:{
                    cout << "1. Activo" << endl;
                    cout << "2. Ocupado" << endl;
                    cout << "3. InActivo" << endl;
                    int newSttat;
                    cin >> newSttat;
                    UserRequest userRequest;
                    
                    userRequest.set_option(2);

                    ChangeStatus* userStatus = userRequest.mutable_status();

                    userStatus->set_newstatus(newSttat);
                    userStatus->set_username("user");

                    string request = userRequest.SerializeAsString();

                    write(clientDescriptor, request.c_str(), request.size());
                    int valread = read(clientDescriptor, buffer, CLIENT_BUFFER_SIZE);
                    buffer[valread] = '\0';
                    serverResponse.ParseFromArray(buffer, CLIENT_BUFFER_SIZE);
                    AllConnectedUsers allConectedUsers = serverResponse.connectedusers();
                    int code = serverResponse.code();
                    string message = serverResponse.servermessage();
                    cout << "Server response: " << message << endl;
                }
                case 6:{
                    bandera = 1;
                    break;
                }
            }

        }
    }


    close(clientDescriptor);
    cout << "Connection closed" << endl;
    return 0;
}