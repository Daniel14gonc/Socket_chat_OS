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

int main() {
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    
    const char* message = "Sebas es gay";
    int clientDescriptor = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serverAddress;
    int status;
    char buffer[CLIENT_BUFFER_SIZE] = {0};

    chat::UserRequest userRequest;
    chat::newMessage newMessage = new chat::newMessage();
    userRequest.set_option(2);
    newMessage->set_sender("Sebas");
    newMessage->set_message("Gay");
    newMessage->set_message_type(false);
    userRequest.set_allocated_message(&newMessage);

    string request;
    userRequest.SerializeToString(&request); 
    strcpy(buffer, request.c_str());

    printf("String: %s\n", buffer);

    
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

    write(clientDescriptor, buffer, CLIENT_BUFFER_SIZE - 1);
    printf("\nMessage sent\n");
    int valread = read(clientDescriptor, buffer, CLIENT_BUFFER_SIZE);
    printf("%s\n", buffer);

    close(clientDescriptor);
    printf("pase\n");
    return 0;
}