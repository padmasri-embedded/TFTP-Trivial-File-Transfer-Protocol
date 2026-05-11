#ifndef TFTP_CLIENT_H
#define TFTP_CLIENT_H

#include <netinet/in.h>    // For sockaddr_in
#include <sys/socket.h>    // For socklen_t
#include <arpa/inet.h>     // For INET_ADDRSTRLEN

/* ---------------- Client Structure ----------------
   Holds all information related to the client connection.
*/
typedef struct {
    int sockfd;                        // Socket file descriptor
    struct sockaddr_in srv_addr;       // Server address structure
    socklen_t srv_len;                 // Length of server address
    char srv_ip[INET_ADDRSTRLEN];      // Server IP address (as string)
} tftp_client_t;

/* ---------------- Function Prototypes ----------------
   Functions used by the TFTP client application.
*/
void connect_to_server(tftp_client_t *client, char *ip, int port);
void put_file(tftp_client_t *client, char *filename);
void get_file(tftp_client_t *client, char *filename);
void disconnect(tftp_client_t *client);
void process_command(tftp_client_t *client, char *command);
void send_request(int sockfd, struct sockaddr_in srv_addr, char *filename, int opcode);

#endif // TFTP_CLIENT_H
