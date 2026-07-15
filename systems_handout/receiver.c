#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>

int main(void) {
    int in_fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in in_addr = {0};
    in_addr.sin_family = AF_INET;
    in_addr.sin_port = htons(47002);
    in_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    if (bind(in_fd, (struct sockaddr *)&in_addr, sizeof in_addr) < 0) {
        perror("bind 47002");
        return 1;
    }

    int out_fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in player = {0};
    player.sin_family = AF_INET;
    player.sin_port = htons(47020);
    player.sin_addr.s_addr = inet_addr("127.0.0.1");

    unsigned char buf[2048];
    uint32_t expected_seq = 0;
    int expected_initialized = 0;

    unsigned char p_history[128][160];
    int has_p[128] = {0};
    unsigned char x_history[128][160];
    int has_x[128] = {0};

    for (;;) {
        ssize_t n = recvfrom(in_fd, buf, sizeof buf, 0, NULL, NULL);
        if (n == 161) {
            uint8_t seq8 = buf[0];
            int is_xor = (seq8 & 0x80) != 0;
            uint8_t seq7 = seq8 & 0x7F;
            
            if (!expected_initialized) {
                expected_seq = seq7;
                expected_initialized = 1;
            }
            
            // 7-bit diff logic. Mask to 7 bits, sign extend 7th bit to 8th bit.
            uint8_t raw_diff = (seq7 - (expected_seq & 0x7F)) & 0x7F;
            int8_t diff = (raw_diff & 0x40) ? (int8_t)(raw_diff | 0x80) : (int8_t)raw_diff;
            uint32_t host_seq = expected_seq + diff;
            
            if (diff > 0) {
                for (uint32_t s = expected_seq + 1; s <= host_seq; s++) {
                    int clear_idx = (s + 64) % 128;
                    has_p[clear_idx] = 0;
                    has_x[clear_idx] = 0;
                }
                expected_seq = host_seq;
            } else if (diff == 0 && expected_initialized) {
                // Duplicate or same seq, just clear +64 anyway to be safe
                int clear_idx = (host_seq + 64) % 128;
                has_p[clear_idx] = 0;
                has_x[clear_idx] = 0;
            }

            int idx = host_seq % 128;

            if (!is_xor) {
                if (!has_p[idx]) {
                    memcpy(p_history[idx], buf + 1, 160);
                    has_p[idx] = 1;
                    
                    // Forward it
                    unsigned char out_buf[164];
                    uint32_t net_seq = htonl(host_seq);
                    memcpy(out_buf, &net_seq, 4);
                    memcpy(out_buf + 4, p_history[idx], 160);
                    sendto(out_fd, out_buf, 164, 0, (struct sockaddr *)&player, sizeof player);
                }
            } else {
                if (!has_x[idx]) {
                    memcpy(x_history[idx], buf + 1, 160);
                    has_x[idx] = 1;
                }
            }

            // Bidirectional sweep for robust FEC cascade recovery
            for (int step = 0; step < 10; step++) {
                int changed = 0;
                for (int offset = -20; offset <= 20; offset++) {
                    uint32_t seq = host_seq + offset;
                    int p_idx = seq % 128;
                    int n_idx = (seq + 1) % 128;
                    
                    // Recover forward: P(seq) + X(seq+1) -> P(seq+1)
                    if (has_p[p_idx] && has_x[n_idx] && !has_p[n_idx]) {
                        for (int i = 0; i < 160; i++) p_history[n_idx][i] = x_history[n_idx][i] ^ p_history[p_idx][i];
                        has_p[n_idx] = 1;
                        unsigned char out_buf[164];
                        uint32_t net_seq = htonl(seq + 1);
                        memcpy(out_buf, &net_seq, 4);
                        memcpy(out_buf + 4, p_history[n_idx], 160);
                        sendto(out_fd, out_buf, 164, 0, (struct sockaddr *)&player, sizeof player);
                        changed = 1;
                    }
                    
                    // Recover backward: P(seq+1) + X(seq+1) -> P(seq)
                    if (has_p[n_idx] && has_x[n_idx] && !has_p[p_idx]) {
                        for (int i = 0; i < 160; i++) p_history[p_idx][i] = x_history[n_idx][i] ^ p_history[n_idx][i];
                        has_p[p_idx] = 1;
                        unsigned char out_buf[164];
                        uint32_t net_seq = htonl(seq);
                        memcpy(out_buf, &net_seq, 4);
                        memcpy(out_buf + 4, p_history[p_idx], 160);
                        sendto(out_fd, out_buf, 164, 0, (struct sockaddr *)&player, sizeof player);
                        changed = 1;
                    }
                }
                if (!changed) break;
            }
        }
    }
    return 0;
}
