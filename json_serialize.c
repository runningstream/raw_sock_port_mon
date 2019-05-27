#include <stdlib.h>
#include <string.h>

#include "output_help.h"
#include "json_serialize.h"

void json_free_elem_ll(json_elem_ll *elem);

void json_free_elem_components(json_element *elem) {
    switch(elem->typ) {
        case JSON_OBJECT:
        case JSON_ARRAY:
            json_free_elem_ll(elem->val.obj_val);
            break;
        case JSON_STRING:
            free(elem->val.str_val.str);
            break;
        case JSON_INT:
        case JSON_FLOAT:
        case JSON_BOOL:
        case JSON_NULL:
            // No further mem alloc'd for these
            break;
        default:
            eprintf("Invalid elem type\n");
    }
}

void json_free(json_element *elem) {
    json_free_elem_components(elem);
    free(elem);
}

void json_free_elem_ll(json_elem_ll *elem) {
    if( elem == 0 ) {
        return;
    }
    json_free_elem_ll(elem->next);
    json_free_elem_components(&elem->val);
    if( elem->name.str != 0 ) {
        free(elem->name.str);
        elem->name.len = 0;
    }
    free(elem);
}


size_t json_serialize_raw_string(json_string *str_val, char *buf,
        size_t buf_len) {
    if( buf_len < (2 + str_val->len) ) {
        eprintf("Buffer too short\n");
        return 0;
    }

    if( memchr(str_val->str, 0, str_val->len) != 0 ) {
        eprintf("Found null in serialized string\n");
    }

    buf[0] = '"';
    memcpy(&buf[1], str_val->str, str_val->len);
    buf[str_val->len + 1] = '"';

    return str_val->len + 2;
}

size_t json_serialize_string(json_element *root, char *buf, size_t buf_len) {
    return json_serialize_raw_string(&root->val.str_val, buf, buf_len);
}

size_t json_serialize_elems(json_elem_ll *root, char *buf,
        size_t buf_len, int is_obj) {
    size_t cur_pos = 0;

    if( root == 0 ) {
        return 0;
    }

    // Add the element name if it's an object
    if( is_obj ) {
        cur_pos = json_serialize_raw_string(&root->name, buf, buf_len);
        if( buf_len - cur_pos < 1 ) {
            eprintf("Buffer too short\n");
            return cur_pos;
        }
        buf[cur_pos] = ':';
        cur_pos += 1;
    }

    // Add this element
    size_t elem_len = json_serialize(&root->val, &buf[cur_pos],
            buf_len - cur_pos);
    cur_pos += elem_len;

    // Add the next elements
    if( root->next != 0 ) {
        if( buf_len - cur_pos < 1 ) {
            eprintf("Buffer too short\n");
            return cur_pos;
        }
        buf[cur_pos] = ',';
        cur_pos += 1;

        elem_len = json_serialize_elems(root->next,
                &buf[cur_pos], buf_len - cur_pos, is_obj);
        cur_pos += elem_len;
    }
    return cur_pos;
}

size_t json_serialize_array(json_element *root, char *buf, size_t buf_len) {
    if( buf_len < 2 ) {
        eprintf("Buffer too short\n");
        return 0;
    }

    buf[0] = '[';
    
    size_t taken = json_serialize_elems(root->val.arr_val, &buf[1], buf_len-2, 0);
    
    buf[1 + taken] = ']';

    return 2 + taken;
}

size_t json_serialize_object(json_element *root, char *buf, size_t buf_len) {
    if( buf_len < 2 ) {
        eprintf("Buffer too short\n");
        return 0;
    }

    buf[0] = '{';
    
    size_t taken = json_serialize_elems(root->val.obj_val, &buf[1], buf_len-2, 1);
    
    buf[1 + taken] = '}';

    return 2 + taken;
}

size_t json_serialize_float(json_element *root, char *buf, size_t buf_len) {
    int wr_len = snprintf(buf, buf_len, "%f", root->val.flt_val);
    if( wr_len > buf_len ) {
        eprintf("Buffer too short\n");
        // Some things actually got printed, but just ignore them
        return 0;
    } else if( wr_len < 0 ) {
        eprintf("Float snprintf error\n");
        return 0;
    }
    return wr_len;
}

size_t json_serialize_int(json_element *root, char *buf, size_t buf_len) {
    int wr_len = snprintf(buf, buf_len, "%li", root->val.int_val);
    if( wr_len > buf_len ) {
        eprintf("Buffer too short\n");
        // Some things actually got printed, but just ignore them
        return 0;
    } else if( wr_len < 0 ) {
        eprintf("Int snprintf error\n");
        return 0;
    }
    return wr_len;
}

size_t json_serialize_bool(json_element *root, char *buf, size_t buf_len) {
    if( root->val.boo_val == 0 ) {
        if( buf_len < 5 ) {
            eprintf("Buffer too short\n");
            return 0;
        }
        memcpy(buf, "false", 5);
        return 5;
    } else {
        if( buf_len < 4 ) { 
            eprintf("Buffer too short\n");
            return 0;
        }
        memcpy(buf, "true", 4);
        return 4;
    }
}

size_t json_serialize_null(json_element *root, char *buf, size_t buf_len) {
    if( buf_len < 4 ) {
        eprintf("Buffer too short\n");
        return 0;
    }
    memcpy(buf, "null", 4);
    return 4;
}

size_t json_serialize(json_element *root, char *buf, size_t buf_len) {
    size_t space_taken = 0;
    switch(root->typ) {
        case JSON_OBJECT:
            space_taken = json_serialize_object(root, buf, buf_len);
            break;
        case JSON_ARRAY:
            space_taken = json_serialize_array(root, buf, buf_len);
            break;
        case JSON_STRING:
            space_taken = json_serialize_string(root, buf, buf_len);
            break;
        case JSON_INT:
            space_taken = json_serialize_int(root, buf, buf_len);
            break;
        case JSON_FLOAT:
            space_taken = json_serialize_float(root, buf, buf_len);
            break;
        case JSON_BOOL:
            space_taken = json_serialize_bool(root, buf, buf_len);
            break;
        case JSON_NULL:
            space_taken = json_serialize_null(root, buf, buf_len);
            break;
        default:
            eprintf("Invalid json element type\n");
            break;
    }

    if( space_taken + 1 < buf_len ) {
        buf[space_taken] = 0;
    } else {
        eprintf("No space for null terminator in json serialize\n");
    }
    return space_taken;
}

json_element * json_root(json_type top_type) {
    // Roots can be only two types
    if( top_type != JSON_OBJECT && top_type != JSON_ARRAY ) {
        eprintf("Invalid JSON root type\n");
        return 0;
    }

    json_element *root = malloc(sizeof(*root));
    if( root == 0 ) {
        eprintf("Error allocating\n");
        return 0;
    }
    memset(root, 0, sizeof(*root));
    root->typ = top_type;

    return root;
}

void place_elem_in_ll(json_element *cur, json_elem_ll *new_elem) {
    json_elem_ll *iter = 0;
    if( cur->typ == JSON_OBJECT ) {
        iter = cur->val.obj_val;
    } else if( cur->typ == JSON_ARRAY ) {
        iter = cur->val.arr_val;
    } else {
        eprintf("Called place_elem_in_ll without _ll type\n");
        return;
    }

    // If the list is currently empty...
    if( iter == 0 ) {
        if( cur->typ == JSON_OBJECT ) {
            cur->val.obj_val = new_elem;
        } else if( cur->typ == JSON_ARRAY ) {
            cur->val.arr_val = new_elem;
        }
    } else {
        while( iter->next != 0 ) {
            iter = iter->next;
        }
        iter->next = new_elem;
    }
}

json_elem_ll * build_json_elem_to_append(json_element *cur, json_type typ,
        char *name, size_t name_len) {
    json_elem_ll *new_elem = malloc(sizeof(*new_elem));
    if( new_elem == 0 ) {
        eprintf("Error allocating element\n");
        return 0;
    }
    memset(new_elem, 0, sizeof(*new_elem));

    if( cur->typ == JSON_OBJECT ) {
        // Drop the final null on strings if they're present
        if( name_len > 0 && name[name_len-1] == '\0' ) {
            name_len -= 1;
        }

        // Alloc and build the name
        new_elem->name.len = name_len;
        new_elem->name.str = malloc(name_len);
        if( new_elem->name.str == 0 ) {
            eprintf("Error allocating obj name\n");
            free(new_elem);
            return 0;
        }
        memcpy(new_elem->name.str, name, name_len);
    }

    new_elem->val.typ = typ;
    return new_elem;
}

// TODO: make sure name isn't already in object?
// Name and name_len are ignored if cur isn't an object
json_element * append_json_elem_str(json_element *cur,
        char * name, size_t name_len,
        char * val, size_t val_len) {
    if( cur->typ != JSON_OBJECT && cur->typ != JSON_ARRAY ) {
        eprintf("Cannot append to non-object or non-array\n");
        return 0;
    }

    // Drop the final null on strings if they're present
    if( val[val_len-1] == '\0' ) {
        val_len -= 1;
    }

    // Allocate the new str buffs early, so we fail early
    char *new_str_buff = malloc(val_len);
    if( new_str_buff == 0 ) {
        eprintf("Error allocating new str buff\n");
        return 0;
    }
    // Copy the contents into place
    memcpy(new_str_buff, val, val_len);

    json_elem_ll *new_elem = build_json_elem_to_append(cur, JSON_STRING,
            name, name_len);
    if( new_elem == 0 ) {
        free(new_str_buff);
        return 0;
    }

    // Setup the new element
    json_string *new_json_str = &new_elem->val.val.str_val;
    new_json_str->len = val_len;
    new_json_str->str = new_str_buff;

    // Place the element in the linked list
    place_elem_in_ll(cur, new_elem);

    return & new_elem->val;
}

json_element * append_json_elem_flt(json_element *cur, 
        char * name, size_t name_len,
        double val) {
    if( cur->typ != JSON_OBJECT && cur->typ != JSON_ARRAY ) {
        eprintf("Cannot append to non-object or non-array\n");
        return 0;
    }

    json_elem_ll *new_elem = build_json_elem_to_append(cur, JSON_FLOAT,
            name, name_len);
    
    new_elem->val.val.flt_val = val;

    place_elem_in_ll(cur, new_elem);

    return & new_elem->val;
}

json_element * append_json_elem_int(json_element *cur, 
        char * name, size_t name_len,
        long int val) {
    if( cur->typ != JSON_OBJECT && cur->typ != JSON_ARRAY ) {
        eprintf("Cannot append to non-object or non-array\n");
        return 0;
    }

    json_elem_ll *new_elem = build_json_elem_to_append(cur, JSON_INT,
            name, name_len);
    
    new_elem->val.val.int_val = val;

    place_elem_in_ll(cur, new_elem);

    return & new_elem->val;
}

json_element * append_json_elem_bool(json_element *cur, 
        char * name, size_t name_len,
        char val) {
    if( cur->typ != JSON_OBJECT && cur->typ != JSON_ARRAY ) {
        eprintf("Cannot append to non-object or non-array\n");
        return 0;
    }

    json_elem_ll *new_elem = build_json_elem_to_append(cur, JSON_BOOL,
            name, name_len);
    
    new_elem->val.val.boo_val = val;

    place_elem_in_ll(cur, new_elem);

    return & new_elem->val;
}

json_element * append_json_elem_null(json_element *cur, 
        char * name, size_t name_len) {
    if( cur->typ != JSON_OBJECT && cur->typ != JSON_ARRAY ) {
        eprintf("Cannot append to non-object or non-array\n");
        return 0;
    }

    json_elem_ll *new_elem = build_json_elem_to_append(cur, JSON_NULL,
            name, name_len);
    
    place_elem_in_ll(cur, new_elem);

    return & new_elem->val;
}

json_element * append_json_elem_arr(json_element *cur, 
        char * name, size_t name_len) {
    if( cur->typ != JSON_OBJECT && cur->typ != JSON_ARRAY ) {
        eprintf("Cannot append to non-object or non-array\n");
        return 0;
    }

    json_elem_ll *new_elem = build_json_elem_to_append(cur, JSON_ARRAY,
            name, name_len);
    
    place_elem_in_ll(cur, new_elem);

    return & new_elem->val;
}

json_element * append_json_elem_obj(json_element *cur, 
        char * name, size_t name_len) {
    if( cur->typ != JSON_OBJECT && cur->typ != JSON_ARRAY ) {
        eprintf("Cannot append to non-object or non-array\n");
        return 0;
    }

    json_elem_ll *new_elem = build_json_elem_to_append(cur, JSON_OBJECT,
            name, name_len);
    
    place_elem_in_ll(cur, new_elem);

    return & new_elem->val;
}
