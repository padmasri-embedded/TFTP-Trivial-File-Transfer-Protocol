#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "tftp_client.h"
#include "tftp.h"

/* ---------------- Global Mode ----------------
   Default transfer mode: "octet" (binary).
   Other option: "netascii" (text mode).
*/
char global_mode[8] = "octet";

/* Function: connect_to_server
   - Create a UDP socket and set up server connection details. */
void connect_to_server(tftp_client_t *client, char *ip, int port) {
    client->sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (client->sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&client->srv_addr, 0, sizeof(client->srv_addr));
    client->srv_addr.sin_family = AF_INET;
    client->srv_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &client->srv_addr.sin_addr) <= 0) {
        perror("Invalid server IP address");
        close(client->sockfd);
        exit(EXIT_FAILURE);
    }

    strcpy(client->srv_ip, ip);
    client->srv_len = sizeof(client->srv_addr);

    printf("[INFO] Connected to TFTP server at %s:%d\n", ip, port);
}

/* Function: put_file
   - Upload a file from client to server.
   Steps   :
      1. Check if file exists locally
      2. Send WRQ packet to server
      3. Call send_file() to transfer file */
void put_file(tftp_client_t *client, char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        perror("File not found on client");
        return;
    }
    fclose(fp);

    send_request(client->sockfd, client->srv_addr, filename, WRQ);
    send_file(client->sockfd, client->srv_addr, client->srv_len, filename, global_mode);
}

/* Function: get_file
   - Download a file from server to client.
   Steps   :
      1. Send RRQ packet to server
      2. Call receive_file() to save file locally */
void get_file(tftp_client_t *client, char *filename) {
    send_request(client->sockfd, client->srv_addr, filename, RRQ);
    receive_file(client->sockfd, client->srv_addr, client->srv_len, filename, global_mode);
}

/* Function: send_request
   - Send an RRQ (read) or WRQ (write) request packet. */
void send_request(int sockfd, struct sockaddr_in srv_addr, char *filename, int opcode) {
    tftp_packet pkt;
    memset(&pkt, 0, sizeof(pkt));

    pkt.opcode = htons(opcode);
    strncpy(pkt.body.request.filename, filename, sizeof(pkt.body.request.filename) - 1);
    strncpy(pkt.body.request.mode, global_mode, sizeof(pkt.body.request.mode) - 1);

    sendto(sockfd, &pkt, sizeof(pkt), 0,
           (struct sockaddr*)&srv_addr, sizeof(srv_addr));
}

/* Function: disconnect
   - Close the client socket and end connection.*/
void disconnect(tftp_client_t *client) {
    close(client->sockfd);
    printf("[INFO] Disconnected from server.\n");
}

/* Function: process_command
   - Parse and execute commands entered by the user.
   Commands:
      - put <file>   → upload file
      - get <file>   → download file
      - mode <type>  → set transfer mode ("octet" or "netascii")
      - exit         → disconnect and quit */
void process_command(tftp_client_t *client, char *command) {
    char cmd[16], arg[256];
    sscanf(command, "%15s %255s", cmd, arg);

    if (strcmp(cmd, "put") == 0) {
        put_file(client, arg);
    } else if (strcmp(cmd, "get") == 0) {
        get_file(client, arg);
    } else if (strcmp(cmd, "mode") == 0) {
        if (strcmp(arg, "octet") == 0 || strcmp(arg, "netascii") == 0) {
            strncpy(global_mode, arg, sizeof(global_mode) - 1);
            printf("[INFO] Transfer mode set to: %s\n", global_mode);
        } else {
            printf("[ERROR] Invalid mode. Use 'octet' or 'netascii'.\n");
        }
    } else if (strcmp(cmd, "exit") == 0) {
        disconnect(client);
        exit(0);
    } else {
        printf("[ERROR] Unknown command: %s\n", cmd);
    }
}

/* Function: main
   - Entry point of the TFTP client.
   Steps   :
      1. Ask user for server IP
      2. Connect to the server
      3. Show available commands
      4. Enter command loop until user exits */
int main() {
    tftp_client_t client;
    char command[300];
    char ip[INET_ADDRSTRLEN];
    int port = PORT;

    printf("Enter server IP: ");
    scanf("%s", ip);
    getchar();  // Clear newline

    connect_to_server(&client, ip, port);

    printf("\nTFTP Client Ready. Available commands:\n");
    printf(" put <file>   -> Upload file to server\n");
    printf(" get <file>   -> Download file from server\n");
    printf(" mode <type>  -> Set transfer mode (octet/netascii)\n");
    printf(" exit         -> Quit client\n\n");

    while (1) {
        printf("tftp> ");
        if (!fgets(command, sizeof(command), stdin))
            break;

        command[strcspn(command, "\n")] = 0; // Remove newline
        process_command(&client, command);
    }

    disconnect(&client);
    return 0;
}
