#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <error.h>
#include <errno.h>
#include <stdio.h>
#include "chat.h"

#define server_port 8082
#define BUF_SIZE 250
#define MAX_HOSTS 10



struct sockaddr_in known_hosts[MAX_HOSTS];
int last_ind = 0;

void add_host(struct sockaddr_in host) {
    for(int i = 0; i < last_ind; i++) {
        if((host.sin_addr.s_addr == known_hosts[i].sin_addr.s_addr) && 
            (host.sin_port == known_hosts[i].sin_port)) return;
    }
    known_hosts[last_ind++] = host;
    if(last_ind >= MAX_HOSTS) {
        last_ind = 0;
    }
}

int server_fd;
struct sockaddr_in server_addr;

void server_init() {
    server_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(server_fd == -1) {
        die("socket");
    }
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(server_port);
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        die("bind");
    }
}

void server_loop() {

    //
    printf("Server is running on port %d\n", server_port);
    char recv_buf[BUF_SIZE] = {0};
    char output_buf[BUF_SIZE] = {0};
    char temp_buf[BUF_SIZE] = {0};

    while(1) {
        struct sockaddr_in peer_addr;
        bzero(&peer_addr, sizeof(peer_addr));
        socklen_t peer_size = sizeof(peer_addr);
        int bytes = recvfrom(server_fd, recv_buf, BUF_SIZE, 0, (struct sockaddr *)&peer_addr, &peer_size);
        if (bytes < 0) {
            die("recv");
        }
        inet_ntop(AF_INET, &peer_addr.sin_addr, temp_buf, BUF_SIZE);
        if(!strcmp(recv_buf, magic)) {
            if(sendto(server_fd, magic_ack, magic_size, 
                0, (struct sockaddr *)&peer_addr, sizeof(peer_addr)) == -1) {
                    die("sendto magic ack");
            }
            add_host(peer_addr);
            printf("registered client %s\n", temp_buf);
        } 
        else {
            // retransmit the message
            snprintf(output_buf, BUF_SIZE, "<%s>: %s", temp_buf, recv_buf);
            for(int i = 0; i < last_ind; i++) {
                if((known_hosts[i].sin_addr.s_addr != peer_addr.sin_addr.s_addr) || (known_hosts[i].sin_port != peer_addr.sin_port)) {
                    if(sendto(server_fd, output_buf, strlen(output_buf), 0, 
                        (struct sockaddr *)&known_hosts[i], sizeof(known_hosts[i])) == -1) {
                            die("sendto");
                    }
                    printf("got message from %s: %s, retransmit to registered clients\n", temp_buf, output_buf);
                }
            }
        }
        //flush buffers
        bzero(&peer_addr, sizeof(peer_addr));
        bzero(recv_buf, BUF_SIZE);
        bzero(output_buf, BUF_SIZE);
        bzero(temp_buf, BUF_SIZE);
    }
    close(server_fd);
}

int main() {
    server_init();
    server_loop();
    return 0;
}