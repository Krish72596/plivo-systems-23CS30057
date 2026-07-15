#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>

int main(void) {
    int in_fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in in_addr = {0};
    in_addr.sin_family = AF_INET;
    in_addr.sin_port = htons(47010);
    in_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    if (bind(in_fd, (struct sockaddr *)&in_addr, sizeof in_addr) < 0) {
        perror("bind 47010");
        return 1;
    }

    int out_fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in relay = {0};
    relay.sin_family = AF_INET;
    relay.sin_port = htons(47001);
    relay.sin_addr.s_addr = inet_addr("127.0.0.1");

    unsigned char buf[2048];
    unsigned char prev_payload[160];
    int has_prev = 0;

    for (;;) {
        ssize_t n = recvfrom(in_fd, buf, sizeof buf, 0, NULL, NULL);
        if (n == 164) {
            uint32_t seq;
            memcpy(&seq, buf, 4);
            uint32_t host_seq = ntohl(seq);
            
            unsigned char out_buf[161];
            out_buf[0] = (uint8_t)(host_seq & 0x7F);
            memcpy(out_buf + 1, buf + 4, 160);

            // Send first copy: P(N)
            sendto(out_fd, out_buf, 161, 0, (struct sockaddr *)&relay, sizeof relay);

            // Send second copy: XOR(N, N-1) 98% of the time to stay under 2.0x
            if (has_prev && (host_seq % 100 < 98)) {
                unsigned char xor_buf[161];
                xor_buf[0] = (uint8_t)((host_seq & 0x7F) | 0x80);
                for (int i = 0; i < 160; i++) {
                    xor_buf[1 + i] = buf[4 + i] ^ prev_payload[i];
                }
                sendto(out_fd, xor_buf, 161, 0, (struct sockaddr *)&relay, sizeof relay);
            }

            memcpy(prev_payload, buf + 4, 160);
            has_prev = 1;
        }
    }
    return 0;
}
