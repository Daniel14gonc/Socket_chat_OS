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


void* sendHeartbeat(void* arg){
    int ClientDescriptor = *((int*)arg);
    while (true){
        UserRequest userRequest;
        userRequest.set_option(5);
        pthread_mutex_lock(&mutexP);
        string request = userRequest.SerializeAsString();
        write(ClientDescriptor, request.c_str(), request.size());
        pthread_mutex_unlock(&mutexP);

        sleep(3000000000);
    }
    return NULL;
}


void* receiveMessages(void* arg) {
    int clientDescriptor = *((int*)arg);
    char buffer[CLIENT_BUFFER_SIZE];
    ServerResponse serverResponse;
    while (true) {
        int valread = read(clientDescriptor, buffer, CLIENT_BUFFER_SIZE);
        if (valread <= 0) {
            break;
        }
        buffer[valread] = '\0';
        serverResponse.ParseFromArray(buffer, CLIENT_BUFFER_SIZE);
        // pthread_mutex_lock(&mutexP);
        int option = serverResponse.option();
        string Smessage = serverResponse.servermessage();
        int code = serverResponse.code();

        if (option == 2) {
            AllConnectedUsers allConectedUsers = serverResponse.connectedusers();
            UserInfo userInfo = serverResponse.userinforesponse();

            cout << "Server response: " << Smessage << endl;

            if (code == 200){
                if (allConectedUsers.connectedusers().empty()) {
                    cout << "User info:" << endl;
                    cout << "\tUsername: " << userInfo.username() << endl;
                    cout << "\tIP: " << userInfo.ip() << endl;
                    cout << "\tStatus: " << userInfo.status() << endl;
                } else {
                    cout << "Connected users:" << endl;
                    processAllConnectedUsers(allConectedUsers);
                }
            }

        }else if (option == 3) {
            cout << "Server response: " << Smessage << endl;

        } else
        if (option == 4) {
            newMessage msg = serverResponse.message();
            if (msg.message_type()) {
                cout << "\n-------Nuevo mensaje general recibido-------\t\n" << endl;
                cout << msg.sender() <<": " << msg.message() << endl << endl;

            }
            else{

                if (msg.sender() != "") {
                    cout << "\n-------Nuevo mensaje privado recibido-------\t\n" << endl;
                    cout << msg.sender() <<": " << msg.message() << endl << endl;
                }
                else {
                    cout << "Server response: " << Smessage << endl;
                }

            }
        } 
        // pthread_mutex_unlock(&mutexP);

    }

    pthread_exit(0);
}

string get_ip() {
    char hostname[128];
    string ip;
    ip = "";
    if (gethostname(hostname, sizeof(hostname)) == -1) {
        std::cerr << "Error getting hostname." << std::endl;
        return ip;
    }

    struct hostent *he;
    if ((he = gethostbyname(hostname)) == nullptr) {
        std::cerr << "Error getting host information." << std::endl;
        return ip;
    }

    struct in_addr **addr_list;
    addr_list = (struct in_addr **)he->h_addr_list;
    for (int i = 0; addr_list[i] != nullptr; ++i) {
        ip = inet_ntoa(*addr_list[i]);
    }

    return ip;
}


// TODO: Recibir argumentos de command line
int main(int argc, char** argv) {
    // string own_ip = get_ip();

    string username = argv[1];
    string ip = argv[2];
    ip = "127.0.0.1";
    int port = stoi(argv[3]);

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
    serverAddress.sin_port = htons(port);

    if (inet_pton(AF_INET, ip.c_str(), &serverAddress.sin_addr) <= 0) 
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
        pthread_t receiveThread;
        pthread_mutex_init(&mutexP, NULL);
        int createResult = pthread_create(&receiveThread, NULL, receiveMessages, (void*)&clientDescriptor);
        if (createResult != 0) {
            perror("Error al crear el hilo");
            return -1;
        }
        pthread_detach(receiveThread); // Desvincula el thread para que no sea necesario esperar a su finalización.

        pthread_t sendThread;
       // int createResult2 = pthread_create(&sendThread, NULL, sendHeartbeat, (void*)&clientDescriptor);
        // if (createResult2 != 0) {
        //     perror("Error al crear el hilo");
        //     return -1;
        // }
      //  pthread_detach(sendThread); // Desvincula el thread para que no sea necesario esperar a su finalización.


        int bandera =0;
        while (bandera ==0 ){
            cout << "1. Obtener listado de usuarios conectados" << endl;
            cout << "2. Obtener informacion de usuario especifico" << endl;
            cout << "3. Cambio de status de usuario" << endl;
            cout << "4. Mandar mensaje directo" << endl;
            cout << "5. Mandar mensaje general" << endl;
            cout << "6. Ayuda" << endl;
            cout << "7. Salir" << endl;

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

                    string username_search;
                    cout << "Ingrese el nombre del usuario:" << endl;
                    cin >> username_search;

                    UserInfoRequest* listInfoRequest = userRequest.mutable_inforequest();
                                        
                    listInfoRequest->set_type_request(false);
                    listInfoRequest->set_user(username_search.c_str());

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
                    cout << "Ingrese el nombre del usuario y el mensaje" << endl;
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
                    cout << "Ingrese el mensaje:" << endl;
                    string input;
                    string s;
                    cin >> s;
                    getline(cin, input);
                    input = s + input;
                    message->set_message_type(true);
                    message->set_sender(username.c_str());
                    message->set_message(input.c_str());

                    string request = userRequest.SerializeAsString();

                    write(clientDescriptor, request.c_str(), request.size());
                    break;
                }
                case 6:{
                    cout << "Bienvenido a ayuda" << endl;
                    cout << "Presione 1 si desea obtener el listado de usuarios conectados" << endl;
                    cout << "Presione 2 si desea obtener informacion de un usuario especifico" << endl;
                    cout << "Presione 3 si desea cambiar de status de su usuario" << endl;
                    cout << "Presione 4 si desea mandar mensaje directo recuerde que el formato es:" << endl;
                    cout << "usuario mensaje mensaje mensaje" << endl;
                    cout << "Presione 5 si desea mandar un mensaje general recuerde que el formato es:" << endl;
                    cout << "mensaje mensaje mensaje" << endl;
                    cout << "Presionar 6 fue la opción que te trao aqui" << endl;
                    cout << "Presione 7 si desea salir" << endl;
                }

                case 7:{
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