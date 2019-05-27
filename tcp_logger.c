#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "output_help.h"
#include "tcp_logger.h"

#define FAIL_RETRY 10

int open_tcp_logger_conn(tcp_logger *logger) {
    // See if the logger is already open...
    if( logger->fd > 0 ) {
        return 0;
    }

    int fd = socket(logger->log_addr.ai_family,
            logger->log_addr.ai_socktype,
            logger->log_addr.ai_protocol);
    if( fd < 0 ) {
        return -1;
    }

    if( connect(fd, logger->log_addr.ai_addr,
            logger->log_addr.ai_addrlen) < 0 ) {
        close(fd);
        return -2;
    }

    logger->fd = fd;
    return 0;
}

int setup_tcp_logger(char *host, char *service, tcp_logger *logger) {
    if( logger == 0 ) {
        eprintf("No logger object provided\n");
        return -1;
    }
    struct addrinfo hints = { 0 };
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    
    struct addrinfo *result = 0;
    int retval = getaddrinfo(host, service, &hints, &result);
    if( retval != 0 ) {
        eprintf("Failed to get tcp logger addrinfo: %s\n",
                gai_strerror(retval));
        return -1;
    }

    struct addrinfo *tmp_r = 0;
    for( tmp_r = result; tmp_r != NULL;
            tmp_r = tmp_r->ai_next ) {
        memcpy(&logger->log_addr, tmp_r, sizeof(logger->log_addr));
        logger->fd = 0;
        if( open_tcp_logger_conn(logger) == 0 ) {
            break;
        }
    }

    if( tmp_r == NULL ) {
        memset(logger, 0, sizeof(*logger));
        eprintf("Failed to connect to tcp logger\n");
        return -1;
    }

    freeaddrinfo(result);

    return 0;
}

int close_tcp_logger_conn(tcp_logger *logger) {
    close(logger->fd);
    logger->fd = 0;

    return 0;
}

int send_via_tcp_logger(tcp_logger *logger, char *buffer, size_t len) {
    size_t fail_count = 0;
    for( fail_count = 0; fail_count < FAIL_RETRY; fail_count += 1 ) {
        // Make sure the socket is open for sending...
        if( open_tcp_logger_conn(logger) == 0 ) {
            size_t tot_sent = 0;
            while( tot_sent < len ) {
                ssize_t sent_now = send(logger->fd, &buffer[tot_sent],
                        len-tot_sent, 0);
                if( sent_now < 0 ) {
                    // On send failure, close the socket and try again
                    close_tcp_logger_conn(logger);
                    break;
                } else {
                    tot_sent += sent_now;
                }
            }
            if( tot_sent >= len ) {
                return 0;
            }
        }
    }

    if( fail_count == FAIL_RETRY ) {
        eprintf("Failed to log msg in %zi attempts.\n", fail_count);
        return -1;
    }

    return 0;
}
