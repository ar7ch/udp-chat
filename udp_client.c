#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <strings.h>
#include "chat.h"

#define SERVER_PORT 8082
#define BUF_SIZE 80
#define MAX_REG_ATTEMPTS 5

char SERVER_IP[] = "192.168.160.62";
char registered = 0;
int fd_to_server;
struct sockaddr_in server_addr;
socklen_t server_addr_len;

void * senderThread(void * arg) {
    char buf[BUF_SIZE] = {0};
    for(int counter = 1; counter <= MAX_REG_ATTEMPTS && !registered; counter++) {
        printf("Trying to register on server... (attempt %d/%d)\n", counter, MAX_REG_ATTEMPTS);
        if(sendto(fd_to_server, magic, magic_size, 0, 
                (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0)
            die("sendto server magic connection");
        usleep(500000);
    }

    if(!registered) {
        die("Not registered and out of attempts");
    }

    while(1) {
        fgets(buf, BUF_SIZE, stdin);
        buf[strcspn(buf, "\n")] = 0; // truncate newline
        if(sendto(fd_to_server, buf, strlen(buf), 0, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0){
            die("sendto server");
        }
    }
    return NULL;
}

void * receiverThread(void * arg) {
    char buf[BUF_SIZE] = {0};
    do { 
        if (recvfrom(fd_to_server, buf, BUF_SIZE, MSG_WAITALL, (struct sockaddr *) &server_addr, &server_addr_len) < 0) {
            die("recvfrom init connection");
        }
        if(!registered) {
            if(strcmp(buf, magic_ack)) {
                fprintf(stderr, "Got wrong magic from server\n");
            }
            else {
                printf("Registered on server\n");
                registered = 1;
            }
        }
        else {
            if(strlen(buf) > 0) printf("%s\n", buf);
        }
        bzero(buf, BUF_SIZE);
    }while(1);
    return NULL;
}

void init() {
    fd_to_server = socket(AF_INET, SOCK_DGRAM, 0);
    if(fd_to_server == -1)
        die("socket");
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    if(inet_pton(AF_INET, SERVER_IP, &(server_addr.sin_addr)) <= 0) {
        perror("invalid server address");
        exit(EXIT_FAILURE);
    }
}

int main() {
        init();
        pthread_t sender, receiver;
        pthread_create(&sender, NULL, *senderThread, NULL);
        usleep(1000);
        pthread_create(&receiver, NULL, *receiverThread, NULL);
        pthread_join(receiver, NULL);
        pthread_join(sender, NULL);
    return 0;
}
