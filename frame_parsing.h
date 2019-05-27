#ifndef FRAME_PARSING_H
#define FRAME_PARSING_H

#include <stdint.h>

typedef struct {
    uint8_t dst_mac[6];
    uint8_t src_mac[6];
    uint16_t eth_proto;
} eth_header;

typedef struct {
    uint8_t ver_ihl;
    uint8_t dscp_ecn;
    uint16_t total_len;
    uint16_t id;
    uint16_t flags_frag_offset;
    uint8_t ttl;
    uint8_t proto;
    uint16_t hdr_cksum;
    uint32_t src_addr;
    uint32_t dst_addr;
} ipv4_header;

typedef struct {
    uint16_t src_port;
    uint16_t dst_port;
    uint32_t seq_num;
    uint32_t acq_num;
    uint8_t data_off_res_ns;
    uint8_t flags;
    uint16_t window_size;
    uint16_t checksum;
    uint16_t urg_ptr;
} tcp_header;

typedef struct {
    uint16_t src_port;
    uint16_t dst_port;
    uint16_t length;
    uint16_t cksum;
} udp_header;


int interpret_udp_header(char *buf, ssize_t amt);

int interpret_tcp_header(char *buf, ssize_t amt);

int interpret_ipv4_header(char *buf, ssize_t amt);

int interpret_eth_header(char *buf, ssize_t amt);

#endif
