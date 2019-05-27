#ifndef TCP_LOGGER_H
#define TCP_LOGGER_H

#include <netdb.h>

typedef struct {
    int fd;
    struct addrinfo log_addr;
} tcp_logger;

int setup_tcp_logger(char *host, char *service, tcp_logger *logger);

int close_tcp_logger_conn(tcp_logger *logger);

int send_via_tcp_logger(tcp_logger *logger, char *buffer, size_t len);

#endif
