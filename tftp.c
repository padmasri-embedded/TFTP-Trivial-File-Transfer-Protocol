/* TFTP Common Implementation
   - Contains functions shared by both client and server
   - Implements block-by-block file transfer using UDP */

#include "tftp.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define DATA_SIZE 512   // Maximum bytes per TFTP DATA packet

/* Function: send_file
   Purpose : Used to send a file (client → server or server → client).
   Steps   :
      1. Open the file in read mode
      2. Read data in blocks of 512 bytes
      3. Send each block in a DATA packet
      4. Wait for ACK from the receiver
      5. Stop when the last block is sent */
void send_file(int sockfd, struct sockaddr_in target_addr, socklen_t addr_len,
               char *filename, char *mode) {
    FILE *fp = fopen(filename, (strcmp(mode, "netascii") == 0) ? "r" : "rb");
    if (!fp) {
        perror("Unable to open file");
        return;
    }

    tftp_packet pkt_data, pkt_ack;
    int block = 1;
    size_t bytes_read;

    while (1) {
        // Prepare a DATA packet
        memset(&pkt_data, 0, sizeof(pkt_data));
        pkt_data.opcode = htons(DATA);
        pkt_data.body.data_packet.block_number = htons(block);

        // Read next 512 bytes from file
        bytes_read = fread(pkt_data.body.data_packet.data, 1, DATA_SIZE, fp);

        // Send the DATA packet
        sendto(sockfd, &pkt_data, 4 + bytes_read, 0,
               (struct sockaddr*)&target_addr, addr_len);

        // Wait for ACK
        recvfrom(sockfd, &pkt_ack, sizeof(pkt_ack), 0,
                 (struct sockaddr*)&target_addr, &addr_len);

        // Validate ACK
        if (ntohs(pkt_ack.opcode) != ACK ||
            ntohs(pkt_ack.body.ack_packet.block_number) != block) {
            printf("ACK mismatch or error on block %d\n", block);
            break;
        }

        // Stop if this was the last block (< 512 bytes)
        if (bytes_read < DATA_SIZE)
            break;

        block++;
    }

    fclose(fp);
    printf("[INFO] File '%s' sent successfully.\n", filename);
}

/* Function: receive_file
   Purpose : Used to receive a file (server ← client or client ← server).
   Steps   :
      1. Open/create file in write mode
      2. Receive DATA packets from sender
      3. Write each block into the file
      4. Send ACK for every received block
      5. Stop when the last block arrives (< 512 bytes) */
void receive_file(int sockfd, struct sockaddr_in source_addr, socklen_t addr_len,
                  char *filename, char *mode) {
    FILE *fp = fopen(filename, (strcmp(mode, "netascii") == 0) ? "w" : "wb");
    if (!fp) {
        perror("Unable to create file");
        return;
    }

    tftp_packet pkt_data, pkt_ack;
    int block = 1;
    ssize_t bytes_received;

    while (1) {
        // Receive a DATA packet
        memset(&pkt_data, 0, sizeof(pkt_data));
        bytes_received = recvfrom(sockfd, &pkt_data, sizeof(pkt_data), 0,
                                  (struct sockaddr*)&source_addr, &addr_len);

        // Validate packet type and block number
        if (ntohs(pkt_data.opcode) != DATA ||
            ntohs(pkt_data.body.data_packet.block_number) != block) {
            printf("Invalid packet or block number mismatch.\n");
            break;
        }

        // Write data to file (exclude 4-byte header)
        fwrite(pkt_data.body.data_packet.data, 1, bytes_received - 4, fp);

        // Prepare and send ACK
        memset(&pkt_ack, 0, sizeof(pkt_ack));
        pkt_ack.opcode = htons(ACK);
        pkt_ack.body.ack_packet.block_number = htons(block);
        sendto(sockfd, &pkt_ack, 4, 0,
               (struct sockaddr*)&source_addr, addr_len);

        // Stop if this was the last block
        if (bytes_received < (4 + DATA_SIZE))
            break;

        block++;
    }

    fclose(fp);
    printf("[INFO] File '%s' received successfully.\n", filename);
}
