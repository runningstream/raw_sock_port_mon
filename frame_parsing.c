#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

#include <arpa/inet.h>
#include <net/ethernet.h>
#include <sys/socket.h>

#include <assert.h>
// Set NDEBUG to disable debugging
// Also disables asserts
#ifndef NDEBUG
#define DEBUG
#endif

#include "output_help.h"
#include "accounting.h"
#include "frame_parsing.h"

#define ETH_IPV4_HDR 0x0008


int interpret_udp_header(char *buf, ssize_t amt) {
    if( amt < sizeof(tcp_header) ) {
        verbose_printf_1("TCP header too small\n");
        return -1;
    }

    udp_header *udp_hdr = (udp_header *) buf;

    uint16_t dst_port = ntohs(udp_hdr->dst_port);

    if( amt < udp_hdr->length ) {
        verbose_printf_1("UDP header too small for data length\n");
        return -1;
    }

    account_all_connection(ACCT_UDP, dst_port);
    return 0;
}

int interpret_tcp_header(char *buf, ssize_t amt) {
    if( amt < sizeof(tcp_header) ) {
        verbose_printf_1("TCP header too small\n");
        return -1;
    }

    tcp_header *tcp_hdr = (tcp_header *) buf;

    uint8_t data_off = (tcp_hdr->data_off_res_ns & 0xf0) >> 4;
    size_t hdr_len = data_off * 4;
    //uint16_t src_port = ntohs(tcp_hdr->src_port);
    uint16_t dst_port = ntohs(tcp_hdr->dst_port);

    if( amt < hdr_len ) {
        verbose_printf_1("TCP header too small for data offset\n");
        return -1;
    }

    if( tcp_hdr->flags == 0x02 ) {
        verbose_printf_2("TCP SYN only set\n");
        account_new_connection(ACCT_TCP, dst_port);
    } else {
        account_all_connection(ACCT_TCP, dst_port);
    }

    return 0;
}

int interpret_ipv4_header(char *buf, ssize_t amt) {
    if( amt < sizeof(ipv4_header) ) {
        verbose_printf_1("IPv4 header too small\n");
        return -1;
    }

    ipv4_header *ip_hdr = (ipv4_header *) buf;

    // More data breakouts...
    uint8_t ver = (ip_hdr->ver_ihl & 0xf0) >> 4;
    uint8_t ihl = (ip_hdr->ver_ihl & 0x0f);
    size_t hdr_len = ihl * 4;
    uint16_t total_len_h_end = ntohs(ip_hdr->total_len);
    /*
    uint8_t dscp = (ip_hdr->dscp_ecn & 0xfc) >> 2;
    uint8_t ecn = (ip_hdr->dscp_ecn & 0x03);
    uint8_t flags = (ip_hdr->flags_frag_offset & 0xe000) >> 13;
    uint16_t frag_offset = (ip_hdr->flags_frag_offset & 0x1fff);
    */

    if( ver != 4 ) {
        verbose_printf_1("IPv4 header version wasn't 4\n");
        return -1;
    }
    if( amt < hdr_len ) {
        verbose_printf_1("IPv4 header too small for IHL\n");
        return -1;
    }

    if( amt < total_len_h_end ) {
        verbose_printf_1("IPv4 packet smaller than reported size\n");
        return -1;
    }
    
    #ifndef NDEBUG
    struct in_addr addr = { 0 };
    addr.s_addr = ip_hdr->src_addr;
    verbose_printf_2("Source address: %s\n", inet_ntoa(addr));
    addr.s_addr = ip_hdr->dst_addr;
    verbose_printf_2("Dest address: %s\n", inet_ntoa(addr));
    #endif

    switch(ip_hdr->proto) {
        case IPPROTO_TCP:
            verbose_printf_2("TCP Packet\n");
            interpret_tcp_header(&buf[hdr_len], amt - (hdr_len));
            break;
        case IPPROTO_UDP:
            verbose_printf_2("UDP Packet\n");
            interpret_udp_header(&buf[hdr_len], amt - (hdr_len));
            break;
        default:
            verbose_printf_1("Unknown IPv4 protocol: %hx\n", ip_hdr->proto);
            account_all_connection(ACCT_IP, ip_hdr->proto);
            break;
    }

    return 0;
}

int interpret_eth_header(char *buf, ssize_t amt) {
    assert(sizeof(eth_header) == 14);

    if( amt < sizeof(eth_header) ) {
        verbose_printf_1("Frame header too small for Ethernet\n");
        return -1;
    }

    eth_header *eth_hdr = (eth_header *) buf;

    int layer_2_retval = 0;

    assert( htons(ETH_P_IP) == ETH_IPV4_HDR );
    switch(eth_hdr->eth_proto) {
        case ETH_IPV4_HDR:
            verbose_printf_2("IPv4 Packet\n");
            layer_2_retval = interpret_ipv4_header(
                    &buf[sizeof(eth_header)],
                    amt - sizeof(eth_header));
            break;
        default:
            verbose_printf_1("Unknown Ethernet protocol: 0x%hx\n",
                    eth_hdr->eth_proto);
            account_all_connection(ACCT_ETH, eth_hdr->eth_proto);
            break;
    }

    if( layer_2_retval < 0 ) {
        verbose_printf_1("Error interpreting layer 2 header!\n");
    }

    return 0;
}
