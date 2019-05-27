#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>

#include "output_help.h"
#include "accounting.h"
#include "json_serialize.h"

#define BUF_LEN 1024

#define MAX_TYPE_NAME_LEN 4
char * type_names[] = {"TCP", "UDP", "ETH", "IP"};
#define MAX_TYPE_IND_NAME_LEN 6
char * type_ind_names[] = {"Port", "Port", "DIX", "Proto"};

typedef uint32_t accounting_arr_typ[ACCT_MAX][NUM_PORTS];
accounting_arr_typ new_connection_acct = { 0 };
accounting_arr_typ all_connection_acct = { 0 };

void print_array_acct_results(uint32_t arr_ptr[][NUM_PORTS]) {
    for( size_t acct_type = 0; acct_type < ACCT_MAX; acct_type += 1 ) {
        printf("Connection Type: %s\n", type_names[acct_type]);
        for( size_t port = 0; port < NUM_PORTS; port += 1 ) {
            if( arr_ptr[acct_type][port] != 0 ) {
                printf("%s %zi count %i\n", type_ind_names[acct_type], port,
                        arr_ptr[acct_type][port]);
            }
        }
    }
}
void print_acct_results() {
    printf("\n");
    printf("New connection stats:\n");
    print_array_acct_results(new_connection_acct);
    printf("All connection stats:\n");
    print_array_acct_results(all_connection_acct);
}

accounting_arr_typ * resolve_conn_acct_arr(size_t conn_type) {
    switch(conn_type) {
        case 0:
            return &new_connection_acct;
        case 1:
            return &all_connection_acct;
        default:
            eprintf("Invalid conn type specified\n");
            return 0;
    }
}

// Return the value corresponding to the parameters
// Cannot provide an error if there is an invalid setting...
// Start with has_acct_value to see if this can actually provide
// you an error-free value
uint32_t get_acct_value(size_t conn_type, size_t acct_type, size_t port) {
    accounting_arr_typ * arr_ptr = resolve_conn_acct_arr(conn_type);

    if( arr_ptr == 0 || acct_type >= ACCT_MAX || port >= NUM_PORTS) {
        // Error condition
        return 0;
    }

    return (*arr_ptr)[acct_type][port];
}

// Return true if the spot corresponding to the parameters is valid and
// has a value
int has_acct_value(size_t conn_type, size_t acct_type, size_t port) {
    accounting_arr_typ * arr_ptr = resolve_conn_acct_arr(conn_type);

    if( arr_ptr == 0 || acct_type >= ACCT_MAX || port >= NUM_PORTS) {
        // Return false for invalid values
        return 0;
    }

    return (* arr_ptr)[acct_type][port] == 0 ? 0 : 1;
}

// Send one result message
size_t json_acct_results(char *buf, size_t buf_len, size_t conn_type,
        size_t acct_type, size_t port) {
    if( ! has_acct_value(conn_type, acct_type, port) ) {
        return 0;
    }

    json_element *root = json_root(JSON_OBJECT);

    char * new_all_type = 0;
    char new_type[] = "New", all_type[] = "All";
    size_t new_all_type_len = 0;

    switch(conn_type) {
        case 0:
            new_all_type = new_type;
            new_all_type_len = sizeof(new_type);
            break;
        default:
            eprintf("Invalid conn type specified, defaulting to all\n");
        case 1:
            new_all_type = all_type;
            new_all_type_len = sizeof(all_type);
    }

    // Is this reporting for a new connection, or all connections?
    char new_all[] = "new_all";
    append_json_elem_str(root,
            new_all, sizeof(new_all),
            new_all_type, new_all_type_len);

    // What's the source - ETH, IP, TCP, UDP?
    char acct_nm_key[] = "Value_source";
    append_json_elem_str(root,
            acct_nm_key, sizeof(acct_nm_key),
            type_names[acct_type],
            strnlen(type_names[acct_type], MAX_TYPE_NAME_LEN)
            );

    // What does the "port" value signify?  A port?  A Dix?
    char acct_nm_type[] = "Value_type";
    append_json_elem_str(root,
            acct_nm_type, sizeof(acct_nm_type),
            type_ind_names[acct_type],
            strnlen(type_ind_names[acct_type], MAX_TYPE_IND_NAME_LEN)
            );

    // What port is this msg about?
    char port_word[] = "Port";
    append_json_elem_int(root,
            port_word, sizeof(port_word), port
            );

    // What was the count on the port?
    char count_word[] = "Count";
    append_json_elem_int(root,
            count_word, sizeof(count_word),
            get_acct_value(conn_type, acct_type, port)
            );


    size_t data_len = json_serialize(root, buf, buf_len);
    json_free(root);

    buf[data_len] = 0;
    return data_len;
}

void output_json_acct_results(json_output_func_type out_func) {
    char json_buf[BUF_LEN] = { 0 };
    for( size_t conn_type = 0; conn_type < 2; conn_type += 1 ) {
        for( size_t acct_type = 0; acct_type < ACCT_MAX; acct_type += 1 ) {
            for( size_t port = 0; port < NUM_PORTS; port += 1 ) {
                size_t data_len = json_acct_results(json_buf,
                        sizeof(json_buf) - 1, conn_type, acct_type, port);
                if( data_len > 0 ) {
                    json_buf[data_len] = '\n';
                    json_buf[data_len+1] = 0;
                    data_len += 1;

                    if( out_func(json_buf, data_len) < 0 ) {
                        eprintf("Failed outputting json results for conn type "
                                "%zi acct_type %zi port %zi\n",
                                conn_type, acct_type, port);
                    }
                }
            }
        }
    }
}

void reset_acct_results() {
    memset(new_connection_acct, 0, sizeof(new_connection_acct));
    memset(all_connection_acct, 0, sizeof(all_connection_acct));
}

void account_connection(uint32_t arr_ptr[][0x10000], account_type type,
        uint16_t dst_port) {
    if( type >= ACCT_MAX ) {
        eprintf("Invalid accounting type specified!");
        return;
    }
    if( arr_ptr[type][dst_port] != 0xffffffff ) {
        arr_ptr[type][dst_port] += 1;
    } else {
        DEBUG_PRINT("Value hit max!");
    }
}
void account_all_connection(account_type type, uint16_t dst_port) {
    account_connection(all_connection_acct, type, dst_port);
}
void account_new_connection(account_type type, uint16_t dst_port) {
    account_connection(new_connection_acct, type, dst_port);
    account_all_connection(type, dst_port);
}

unsigned int print_stats_freq = 0;
void (* print_stats_callback)(void) = 0;

void _set_alarm();

void handle_alarm(int sig_num) {
    print_stats_callback();
    reset_acct_results();
    _set_alarm();
}

void _set_alarm() {
    struct sigaction action = { 0 };
    action.sa_handler = handle_alarm;

    sigaction(SIGALRM, &action, 0);

    alarm(print_stats_freq);
}

void set_alarm(unsigned int freq, void (* callback)(void)) {
    print_stats_freq = freq;
    print_stats_callback = callback;
    _set_alarm();
}
