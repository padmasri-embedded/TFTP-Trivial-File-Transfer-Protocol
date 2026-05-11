/* TFTP Header File
   - Common header for both client and server
   - Contains protocol definitions, packet structures, and function prototypes*/

#ifndef TFTP_H
#define TFTP_H

#include <stdint.h>        // For fixed-width integer types
#include <arpa/inet.h>     // For network functions (htons, ntohs, inet_pton)

/* ---------------- Configuration Section ---------------- */
#define PORT 7070          // Default TFTP port (non-privileged, safe to use)
#define BUFFER_SIZE 516    // Maximum packet size = 2 (opcode) + 2 (block) + 512 (data)

/* ---------------- Operation Codes ----------------
   Each TFTP packet starts with a 2-byte opcode.
   This indicates the type of the packet.
*/
typedef enum {
    RRQ = 1,   // Read Request (client requests file from server)
    WRQ = 2,   // Write Request (client uploads file to server)
    DATA = 3,  // Data Packet (contains file content)
    ACK = 4,   // Acknowledgment (confirms receipt of a block)
    ERROR = 5  // Error Packet (indicates a transmission error)
} tftp_opcode;

/* ---------------- TFTP Packet Structure ----------------
   All packets begin with a 2-byte opcode.
   Depending on the opcode, a specific format is used.
*/
typedef struct {
    uint16_t opcode;   // Packet type (network byte order)

    union {
        // Request packet (RRQ / WRQ)
        struct {
            char filename[256];  // File name
            char mode[8];        // Transfer mode ("octet" or "netascii")
        } request;

        // Data packet
        struct {
            uint16_t block_number;  // Block number (starts from 1)
            char data[512];         // File data (max 512 bytes)
        } data_packet;

        // ACK packet
        struct {
            uint16_t block_number;  // Acknowledged block number
        } ack_packet;

        // Error packet
        struct {
            uint16_t error_code;     // Numeric error code
            char error_msg[512];     // Human-readable error message
        } error_packet;
    } body;

} tftp_packet;

/* ---------------- Function Prototypes ----------------
   Common functions used by both client and server
   - send_file(): Send a file block by block
   - receive_file(): Receive a file block by block
*/
void send_file(int sockfd, struct sockaddr_in target_addr, socklen_t addr_len,
               char *filename, char *mode);

void receive_file(int sockfd, struct sockaddr_in target_addr, socklen_t addr_len,
                  char *filename, char *mode);

#endif // TFTP_H
