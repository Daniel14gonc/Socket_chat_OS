#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <pthread.h>


//ProtoBuff
#include "project.pb.h"

#define PORT 8080
#define CLIENT_BUFFER_SIZE 3072

using namespace std;
using namespace chat;


pthread_mutex_t mutexP;

struct User {
    string username;
    string ip;
    int socketFD;
    int status;
};

void processAllConnectedUsers(const AllConnectedUsers& allUsers) {
    int contador = 1;
    // Iterar a través de todos los usuarios conectados en el campo connectedUsers
    for (const auto& userInfo : allUsers.connectedusers()) {
        cout << "Usuario #" << contador << ":" << endl;
        // Acceder a las propiedades de cada usuario conectado (por ejemplo, el nombre de usuario y la dirección IP)
        cout << "\tUsuario: " << userInfo.username() << ", IP: " << userInfo.ip() << ", Estado: " << userInfo.status() << endl;
        contador++;
    }
}

void* receiveMessages(void* arg) {
    int clientDescriptor = *((int*)arg);
    char buffer[CLIENT_BUFFER_SIZE];
    ServerResponse serverResponse;
    pthread_mutex_lock(&mutexP);
    while (true) {

        int valread = read(clientDescriptor, buffer, CLIENT_BUFFER_SIZE);
        if (valread <= 0) {
            break;
        }
        buffer[valread] = '\0';
        serverResponse.ParseFromArray(buffer, CLIENT_BUFFER_SIZE);
        int option = serverResponse.option();
        string Smessage = serverResponse.servermessage();
        int code = serverResponse.code();

        if (option == 1) {
            AllConnectedUsers allConectedUsers = serverResponse.connectedusers();
            cout << "Server response: " << Smessage << endl;
        
            if (code == 200){
                cout << "Connected users:" << endl;
                processAllConnectedUsers(allConectedUsers);
            }

        } else if (option == 2) {
            AllConnectedUsers allConectedUsers = serverResponse.connectedusers();
            cout << "Server response: " << Smessage << endl;
            
            if (code == 200){
                const auto& user = allConectedUsers.connectedusers()[0];
                cout << "User info:" << endl;
                cout << "\tUsuario: " << user.username() << endl;
                cout << "\tIP: " << user.ip() << endl;
                cout << "\tEstado: " << user.status() << endl;
            }

        } else if (option == 3) {
            cout << "Server response: " << Smessage << endl;

        } else
        if (option == 4) {
            newMessage msg = serverResponse.message();
            if (msg.message_type()){
                cout << "\n-------Nuevo mensaje general recibido-------\t\n" << endl;
                cout << msg.sender() <<": " << msg.message() << endl << endl;
                break
            }
            else{

                if (msg.sender() != "") {
                    cout << "\n-------Nuevo mensaje privado recibido-------\t\n" << endl;
                    cout << msg.sender() <<": " << msg.message() << endl << endl;
                }
                else {
                    cout << "Server response: " << Smessage << endl;
                }
                break
            }
        } 

    }
    pthread_mutex_unlock(&mutexP);
    return NULL;
}


// TODO: Recibir argumentos de command line
int main(int argc, char** argv) {

    string username = argv[1];
    string ip = argv[2];

    GOOGLE_PROTOBUF_VERIFY_VERSION;

    int clientDescriptor = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serverAddress;
    int status;
    char buffer[CLIENT_BUFFER_SIZE];

    UserRequest userRequest;
    UserRegister* registerUser = userRequest.mutable_newuser();
    
    userRequest.set_option(1);
    
    registerUser->set_ip(ip);
    registerUser->set_username(username);

    ServerResponse serverResponse;

    string request = userRequest.SerializeAsString();
    
    if (clientDescriptor < 0) {
        perror("socket creation error");
        return -1;
    }

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(PORT);

    if (inet_pton(AF_INET, ip.c_str(), &serverAddress.sin_addr) <= 0) 
    {
        printf("\n Invalid address/ Address not supported \n");
        return -1;
    }
    int epochs = 0;
    while((status = connect(clientDescriptor, (struct sockaddr*)&serverAddress, sizeof(serverAddress))) < 0 and epochs < 100) {
        epochs += 1;
    }
    cout << "Connection succesful" << endl;

    // if ((status = connect(clientDescriptor, (struct sockaddr*)&serverAddress, sizeof(serverAddress))) < 0) {
    //     printf("\nConnection failed2\n");
    //     return -1;
    // }

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
        pthread_t receiveThread;
        pthread_mutex_init(&mutexP, NULL);
        int createResult = pthread_create(&receiveThread, NULL, receiveMessages, (void*)&clientDescriptor);
        if (createResult != 0) {
            perror("Error al crear el hilo");
            return -1;
        }
        pthread_detach(receiveThread); // Desvincula el thread para que no sea necesario esperar a su finalización.

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
                    break;
                }
                case 3:{
                    cout << "1. Activo" << endl;
                    cout << "2. Ocupado" << endl;
                    cout << "3. InActivo" << endl;
                    int newSttat;
                    cin >> newSttat;
                    UserRequest userRequest;
                    
                    userRequest.set_option(3);

                    ChangeStatus* userStatus = userRequest.mutable_status();

                    userStatus->set_newstatus(newSttat);
                    userStatus->set_username("user");

                    string request = userRequest.SerializeAsString();

                    write(clientDescriptor, request.c_str(), request.size());
                    break;
                }
                case 4:{
                    UserRequest userRequest;
                    userRequest.set_option(4);

                    newMessage* message = userRequest.mutable_message();

                    string input,recipient, message_user;
                    cin >> recipient;
                    getline(cin, message_user);

                    message->set_message_type(false);
                    message->set_sender(username.c_str());
                    message->set_recipient(recipient.c_str());
                    message->set_message(message_user.c_str());

                    string request = userRequest.SerializeAsString();

                    write(clientDescriptor, request.c_str(), request.size());
                    break;
                }
                case 5:{
                    UserRequest userRequest;
                    userRequest.set_option(4);

                    newMessage* message = userRequest.mutable_message();

                    message->set_message_type(true);
                    message->set_sender("user");
                    message->set_message("Hola");

                    string request = userRequest.SerializeAsString();

                    write(clientDescriptor, request.c_str(), request.size());
                    break;
                }
                case 6:{
                    bandera = 1;
                    break;
                }
                default:{
                    cout << "Opcion no valida" << endl;
                    break;
                }
            }

        }
    }


    close(clientDescriptor);
    cout << "Connection closed" << endl;
    pthread_mutex_destroy(&mutexP);
    return 0;
}