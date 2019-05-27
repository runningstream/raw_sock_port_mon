#include <string.h>

#include "output_help.h"
#include "json_serialize.h"

int main() {
    char name1[] = "FOO", name2[] = "BAR", str1[] = "foo str", str2[] = "bar str";
    char name3[] = "flt", name4[] = "int", name5[] = "boolt", name6[] = "null",
            name7[] = "boolf", name8[] = "arr obj", name9[] = "obj obj";

    json_element * root = json_root(JSON_OBJECT);

    append_json_elem_str(root, name1, sizeof(name1), str1, sizeof(str1));
    append_json_elem_str(root, name2, sizeof(name2), str2, sizeof(str2));
    append_json_elem_flt(root, name3, sizeof(name3), 15.3);

    json_element *arrobj = append_json_elem_arr(root, name8, sizeof(name8));
    append_json_elem_int(arrobj, name4, sizeof(name4), 22);
    append_json_elem_bool(arrobj, name5, sizeof(name5), 1);
    append_json_elem_null(arrobj, name6, sizeof(name6));
    append_json_elem_bool(arrobj, name7, sizeof(name7), 0);

    json_element *objobj = append_json_elem_obj(root, name9, sizeof(name9));
    append_json_elem_int(objobj, name4, sizeof(name4), 22);
    append_json_elem_bool(objobj, name5, sizeof(name5), 1);
    append_json_elem_null(objobj, name6, sizeof(name6));
    append_json_elem_bool(objobj, name7, sizeof(name7), 0);

    char outbuf[4096] = { 0 };
    size_t buf_vals = json_serialize(root, outbuf, sizeof(outbuf));

    if( buf_vals >= 4096 ) {
        buf_vals = 4095;
    }
    outbuf[buf_vals] = 0;

    size_t str_len = strlen(outbuf);

    printf("outbuf len %zi: %s\n", buf_vals, outbuf);
    printf("strlen %zi\n", str_len);
    if( str_len != buf_vals ) {
        printf("ALERT!  INVALID buf len\n");
    }

    json_free(root);

    return 0;
}
