#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "tftp.h"

/* Function: handle_rrq
   - Handle Read Request (RRQ) from client.
             Calls send_file() to transfer the requested file. */
void handle_rrq(int sockfd, struct sockaddr_in cli_addr, socklen_t cli_len,
                char *filename, char *mode) {
    send_file(sockfd, cli_addr, cli_len, filename, mode);
}

/* Function: handle_wrq
   - Handle Write Request (WRQ) from client.
             Calls receive_file() to accept and store the file.*/
void handle_wrq(int sockfd, struct sockaddr_in cli_addr, socklen_t cli_len,
                char *filename, char *mode) {
    receive_file(sockfd, cli_addr, cli_len, filename, mode);
}

/* Function: main
   Steps   :
      1. Create a UDP socket
      2. Bind socket to PORT
      3. Wait for incoming client requests
      4. Handle RRQ/WRQ based on received opcode*/
int main() {
    int sockfd;
    struct sockaddr_in srv_addr, cli_addr;
    socklen_t cli_len = sizeof(cli_addr);
    tftp_packet pkt;

    //Create UDP Socket 
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    //Configure Server Address
    memset(&srv_addr, 0, sizeof(srv_addr));
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_port = htons(PORT);
    srv_addr.sin_addr.s_addr = INADDR_ANY;

    //Bind Socket to Port
    if (bind(sockfd, (struct sockaddr*)&srv_addr, sizeof(srv_addr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("[INFO] TFTP Server running on port %d...\n", PORT);

    // Main Server Loop 
    while (1) {
        ssize_t n = recvfrom(sockfd, &pkt, sizeof(pkt), 0,
                             (struct sockaddr*)&cli_addr, &cli_len);
        if (n < 0) {
            perror("recvfrom failed");
            continue;
        }

        uint16_t opcode = ntohs(pkt.opcode);
        printf("[INFO] Received opcode: %d\n", opcode);

        switch (opcode) {
            case RRQ:
                printf("[INFO] RRQ for file: %s (mode: %s)\n",
                       pkt.body.request.filename,
                       pkt.body.request.mode);
                handle_rrq(sockfd, cli_addr, cli_len,
                           pkt.body.request.filename,
                           pkt.body.request.mode);
                break;

            case WRQ:
                printf("[INFO] WRQ for file: %s (mode: %s)\n",
                       pkt.body.request.filename,
                       pkt.body.request.mode);
                handle_wrq(sockfd, cli_addr, cli_len,
                           pkt.body.request.filename,
                           pkt.body.request.mode);
                break;

            default:
                printf("[ERROR] Unsupported opcode: %d\n", opcode);
        }
    }

    close(sockfd);
    return 0;
}
