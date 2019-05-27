#ifndef ACCOUNTING_H
#define ACCOUNTING_H

#define NUM_PORTS 0xffff + 1

// Must be sequential, starting with 0, ACCT_MAX being the largest
typedef enum {
    ACCT_TCP,
    ACCT_UDP,
    ACCT_ETH,
    ACCT_IP,

    ACCT_MAX
} account_type;

void print_acct_results();

typedef int (* json_output_func_type)(char *buffer, size_t len);
void output_json_acct_results(json_output_func_type out_func);

void account_all_connection(account_type type, uint16_t dst_port);

void account_new_connection(account_type type, uint16_t dst_port);

void set_alarm(unsigned int timeout, void (* callback)(void));

#endif
