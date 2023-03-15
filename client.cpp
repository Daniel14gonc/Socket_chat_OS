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

int main() {
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    
    const char* message = "Sebas es gay";
    int clientDescriptor = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serverAddress;
    int status;
    char buffer[CLIENT_BUFFER_SIZE];

    UserRequest userRequest;
    newMessage* userMessage = userRequest.mutable_message();
    userRequest.set_option(4);
    userMessage->set_sender("Sebas");
    userMessage->set_message("Sebas es una muy buena persona. Le gusta rezar.");
    userMessage->set_message_type(false);

    if (userRequest.has_message()) {
        const newMessage& message = userRequest.message();
        if (message.has_message())
            cout << "Si tiene mensaje " << message.message() << endl;
    }

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
    printf("\nMessage sent\n");
    int valread = read(clientDescriptor, buffer, CLIENT_BUFFER_SIZE);
    printf("%s\n", buffer);

    close(clientDescriptor);
    printf("pase\n");
    return 0;
}