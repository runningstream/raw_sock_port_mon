#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

#include <arpa/inet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <linux/if_packet.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include "output_help.h"
#include "accounting.h"
#include "frame_parsing.h"
#include "tcp_logger.h"
#include "bpf_bytecode.h"

#define NOBODY_UID 65534
#define NOBODY_GID 65534

#define STAT_PRINT_FREQ_SECS 60 * 10

#define JSON_DEFAULT_HOST "localhost"
#define JSON_DEFAULT_PORT "3334"

void usage(char *procname) {
    printf("Usage: %s [-a log_addr] [-p log_port] [-e iface] [-d dst_ip]\n",
            procname);
    printf("         [-v] [-t output_period]\n"
        "  -a log_addr    Address to connect to via TCP to output JSON logs\n"
        "                 (default localhost)\n"
        "  -p log_port    JSON logging TCP port (default 3334)\n"
        "  -e iface       The interface on which to listen for connections\n"
        "  -d dst_ip      Drop all IPV4 traffic except destined to this IP\n"
        "  -t out_period  The time, in seconds, between JSON log output\n"
        "  -v             Outputs verbose text info.  Specifying more than\n"
        "                 once increases verbosity.\n"
        );
}


int main(int argc, char * argv[]) {
    int retval = 0;
    int sock = 0;
    tcp_logger logger = { 0 };

    int opt = 0, verbosity = 0;
    unsigned int out_period = STAT_PRINT_FREQ_SECS;
    char * json_out_addr = JSON_DEFAULT_HOST;
    char * json_out_port = JSON_DEFAULT_PORT;
    char * listen_iface = 0;
    char * listen_dst_ip = 0;
    while( (opt = getopt(argc, argv, "hva:p:e:d:t:")) != -1 ) {
        switch( opt ) {
            case 'v':
                verbosity += 1;
            case 'a':
                json_out_addr = optarg;
                break;
            case 'p':
                json_out_port = optarg;
                break;
            case 'e':
                listen_iface = optarg;
                break;
            case 'd':
                listen_dst_ip = optarg;
                break;
            case 't':
                out_period = atoi(optarg);
                break;
            default:
            case 'h':
                usage(argv[0]);
                return 0;
        }
    }

    set_verbose_level(verbosity);

    if( setup_tcp_logger(json_out_addr, json_out_port, &logger) < 0 ) {
        eprintf("Error setting up JSON connection.\n");
        retval = 1;
        goto end;
    }

    sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if( sock == -1 ) {
        eprintf("Error %i building raw socket: %s\n", errno, strerror(errno));
        retval = 1;
        goto end;
    }

    // Bind socket to interface
    if( listen_iface != 0 ) {
        struct sockaddr_ll bind_addr = { 0 };
        struct ifreq ifr = { 0 };

        size_t iface_len = strnlen(listen_iface, IFNAMSIZ);
        if( iface_len == IFNAMSIZ ) {
            eprintf("Interface name too long\n");
            retval = 1;
            goto end;
        }

        // Find the interface index
        strncpy((char *) ifr.ifr_name, listen_iface, IFNAMSIZ-1);
        if( ioctl(sock, SIOCGIFINDEX, &ifr) == -1 ) {
            eprintf("Error during ioctl: %s\n", strerror(errno));
            retval = 1;
            goto end;
        }

        // Setup the bind address with interface index
        bind_addr.sll_family = AF_PACKET;
        bind_addr.sll_ifindex = ifr.ifr_ifindex;
        bind_addr.sll_protocol = htons(ETH_P_ALL);
        
        if( bind(sock, (struct sockaddr *) &bind_addr, sizeof(bind_addr)) ) {
            eprintf("Error during bind: %s\n", strerror(errno));
            retval = 1;
            goto end;
        }
    }

    // Setup a BPF filter for a dest IP, if selected

    size_t inst_cnt = sizeof(bpf_code) / sizeof(bpf_code[0]);
    size_t ind = 0;
    if( listen_dst_ip != 0 ) {
        // Set the filter up for this application
        // First, setup the ipv4 ver
        struct in_addr listen_dst_addr = { 0 };
        if( inet_aton(listen_dst_ip, &listen_dst_addr) == 0 ) {
            eprintf("Error interpreting listen IP address\n");
            retval = 1;
            goto end;
        }
        for( ind = 0; ind < inst_cnt; ind += 1 ) {
            if( bpf_code[ind].k == bpf_code_ipv4_standin ) {
                // Patch the standin with the ipv4 addr
                // It needs to be in host byte order, but
                // inet_aton puts it in network order, so convert
                // it back first...
                bpf_code[ind].k = ntohl(listen_dst_addr.s_addr);
                break;
            }
        }
        if( ind == inst_cnt ) {
            eprintf("Failed to find ipv4 standin\n");
            retval = 1;
            goto end;
        }
    } else {
        // Disable the dest-IP-based filter
        for( ind = 0; ind < inst_cnt; ind += 1 ) {
            if( bpf_code[ind].k == bpf_code_dst_ipv4_enable ) {
                // Disable the filter by changing the k value
                // to any other value.  Like 0.
                bpf_code[ind].k = 0;
                break;
            }
        }
        if( ind == inst_cnt ) {
            eprintf("Failed to find ipv4 filter disable\n");
            retval = 1;
            goto end;
        }
    }

    // Setup the ipv6 filter (currently, just accept all)
    for( ind = 0; ind < inst_cnt; ind++ ) {
        if( bpf_code[ind].k == bpf_code_ipv6_standin[0][0] ) {
            // Patch the standin with the accept code
            // TODO: actually handle ipv6 addr filter
            bpf_code[ind].code = accept_sock_filter.code;
            bpf_code[ind].jt = accept_sock_filter.jt;
            bpf_code[ind].jf = accept_sock_filter.jf;
            bpf_code[ind].k = accept_sock_filter.k;
            break;
        }
    }
    if( ind == inst_cnt ) {
        eprintf("Failed to find ipv6 standin\n");
        retval = 1;
        goto end;
    }

    // Apply the filter
    struct sock_fprog bpf = {
        .len = inst_cnt,
        .filter = bpf_code,
    };
    if( setsockopt(sock, SOL_SOCKET, SO_ATTACH_FILTER, &bpf,\
            sizeof(bpf)) < 0 ) {
        eprintf("Error applying BPF filter: %s\n", strerror(errno));
        retval = 1;
        goto end;
    }

    // TODO: filter out outgoing traffic, or account for it
    // Probably don't need to do this... anymore, with dst ip filter

    // Drop privs
    if( getgid() == 0 ) {
        if( setgid(NOBODY_GID) != 0 ) {
            eprintf("Error setting GID: %s\n", strerror(errno));
            retval = 1;
            goto end;
        }
    }
    if( getuid() == 0 ) {
        if( setuid(NOBODY_UID) != 0 ) {
            eprintf("Error setting UID: %s\n", strerror(errno));
            retval = 1;
            goto end;
        }
    }

    // Setup the alarm that processes/prints/sends data
    void log_stats() {
        int json_out_func(char *buf, size_t len) {
            return send_via_tcp_logger(&logger, buf, len);
        }
        output_json_acct_results(&json_out_func);
    }
    set_alarm(out_period, log_stats);

    //for( size_t ind = 0; ind < 1000; ind += 1 ) {
    while( 1 ) {
        char buf[4096] = { 0 };
        ssize_t recv_amt = recvfrom(sock, buf, sizeof(buf), 0, NULL, NULL);
        if( recv_amt == -1 ) {
            if( errno == EINTR ) {
                // Receive was interrupted by a signal, just continue...
            } else {
                eprintf("Error %i receiving: %s\n", errno,
                        strerror(errno));
                retval = 2;
                goto end;
            }
        } else {
            if( interpret_eth_header(buf, recv_amt) < 0 ) {
                eprintf("Error interpreting frame header!\n");
            }
        }
    }

    print_acct_results();

end:
    close(sock);
    close_tcp_logger_conn(&logger);
    return retval;
}
