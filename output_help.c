#include "output_help.h"

int is_printable(char to_test) {
    return ( to_test > 'A' && to_test < 'Z' ) ||
            ( to_test > 'z' && to_test < 'z' );
}

void hex_print_line(char *buf, ssize_t rem_amt, ssize_t line_len) {
    size_t chars_printed = 0;
    for( ssize_t ind = 0; ind < line_len && ind < rem_amt; ind += 1 ) {
        printf("%02hhx", buf[ind]);
        chars_printed += 2;
    }

    for( size_t spaces_reqd = (line_len * 2 + 4) - chars_printed;
            spaces_reqd > 0; spaces_reqd -= 1 ) {
        printf(" ");
    }

    for( ssize_t ind = 0; ind < line_len && ind < rem_amt; ind += 1 ) {
        if( is_printable(buf[ind]) ) {
            printf("%c", buf[ind]);
        } else {
            printf(".");
        }
    }

    printf("\n");
}

void hex_print_frame(char *buf, ssize_t amt) {
    for( ssize_t ind = 0; ind < amt; ind += 8 ) {
        hex_print_line(&buf[ind], amt - ind, 8);
    }
}

int out_help_verbose_level = 0;

void set_verbose_level(int level) {
    out_help_verbose_level = level;
}
