#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <strings.h>
#include <limits.h>
#include "chat.h"

#define HOSTPORT_DELIM ":"
#define BUF_SIZE 80
#define MAX_REG_ATTEMPTS 5

/**
 * Server parameters
 */
char server_iaddr[20] = {'\0'};
uint16_t server_port;

/**
 * Thread synchronization
 */
pthread_barrier_t conn_ready_barrier;

/**
 * Socket-specific connection parameters
 */
int client_socket;
struct sockaddr_in server_addr;
socklen_t server_addr_len;

/**
 * Protocol-specific parameters
 */
char registered = 0;


/**
 * Thread reading the user input and sending it to server.
 * @param arg
 * @return
 */
void * senderThread(void * arg) {
    char buf[BUF_SIZE] = {0};
    while(1) {
        fgets(buf, BUF_SIZE, stdin);
        buf[strcspn(buf, "\n")] = 0; // truncate newline
        if(sendto(client_socket, buf, strlen(buf), 0, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0){
            die("sendto server");
        }
    }
    return NULL;
}

/**
 * Thread waiting for incoming messages from the server.
 * @param arg
 * @return
 */
void * receiverThread(void * arg) {
    char buf[BUF_SIZE] = {0};
    do {
        if (recvfrom(client_socket, buf, BUF_SIZE, MSG_WAITALL, (struct sockaddr *) &server_addr, &server_addr_len) < 0) {
            die("recvfrom init_networking connection");
        }
        if(strlen(buf) > 0) printf("%s\n", buf);
        bzero(buf, BUF_SIZE);
    }while(1);
    return NULL;
}

/**
 * Creates socket using defined host address and port number.
 */
void init_networking() {
    client_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if(client_socket == -1)
        die("socket");
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    if(inet_pton(AF_INET, server_iaddr, &(server_addr.sin_addr)) <= 0) {
        perror("invalid server address");
        exit(EXIT_FAILURE);
    }
}

/**
 * Parse commandline arguments.
 * @param argc
 * @param argv
 */
void parse_args(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s <host:port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    else {
        char * hostport_str = argv[1];
        char * token = strtok(hostport_str, HOSTPORT_DELIM);
        if(token == NULL) {
            fprintf(stderr, "usage: %s <host:port>\n", argv[0]);
            exit(EXIT_FAILURE);
        }
        // save the host address
        strncpy(server_iaddr, token, strlen(token));
        token = strtok(NULL, HOSTPORT_DELIM);
        if(token == NULL) {
            printf("usage: %s <host:port>", argv[0]);
            exit(EXIT_FAILURE);
        }
        long parse_res = strtol(token, NULL, 10);
        if (parse_res != 0 && parse_res != LONG_MAX && parse_res < 65535) {  // check if the port number is valid
            server_port = (uint16_t) parse_res;
        }
        else {
            die("Invalid port number");
        }
    }
}

/**
 * Perform register procedure on a server.
 */
void register_on_server() {
    char buf[BUF_SIZE] = {0};
    for(int counter = 1; counter <= MAX_REG_ATTEMPTS && !registered; counter++) {
        printf("Trying to register on server... (attempt %d/%d)\n", counter, MAX_REG_ATTEMPTS);
        if(sendto(client_socket, magic, magic_size, 0,
                  (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0)
            die("sendto server magic connection");
        if (recvfrom(client_socket, buf, BUF_SIZE, MSG_WAITALL, (struct sockaddr *) &server_addr, &server_addr_len) < 0) {
            die("recvfrom register connection");
        }
        if(strcmp(buf, magic_ack)) {
            fprintf(stderr, "Got wrong magic from server\n");
        }
        else {
            printf("Registered on server\n");
            registered = 1;
            break;
        }
    }

    if(!registered) {
        die("Not registered and out of attempts");
    }
}

int main(int argc, char *argv[]) {
    /* init */
    parse_args(argc, argv);
    init_networking();
    register_on_server();

    /* start receiving and sending */
    pthread_t sender, receiver;
    pthread_create(&sender, NULL, senderThread, NULL);
    pthread_create(&receiver, NULL, receiverThread, NULL);
    pthread_join(receiver, NULL);
    pthread_join(sender, NULL);
    return 0;
}
