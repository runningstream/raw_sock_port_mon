#ifndef JSON_SERIALIZE_H
#define JSON_SERIALIZE_H

struct json_elem_ll_struct;
typedef struct json_elem_ll_struct json_elem_ll;

typedef json_elem_ll * json_object;
typedef json_elem_ll * json_array;

// Not guaranteed to have a \0 at end of str,
// but guaranteed to have len bytes allocated to str
typedef struct {
    size_t len;
    char * str;
} json_string;
typedef long int json_int;
typedef double json_float;
typedef char json_bool;

typedef union {
    json_object obj_val;
    json_array  arr_val;
    json_string str_val;
    json_int    int_val;
    json_float  flt_val;
    json_bool   boo_val;
} json_element_union;

typedef enum {
    JSON_OBJECT,
    JSON_ARRAY,
    JSON_STRING,
    JSON_INT,
    JSON_FLOAT,
    JSON_BOOL,
    JSON_NULL
} json_type;

typedef struct {
    json_type typ;
    json_element_union val;
} json_element;

// Arrays have name set to null
struct json_elem_ll_struct {
    json_elem_ll *next;
    json_element val;
    json_string name;
};


/* Sample JSON representations
  [ 5, 3, 2.5, "ab", [1, 2] ]
  json_array with json_arr_entries:
    json_element typ JSON_INT val 5
    json_element typ JSON_INT val 3
    json_element typ JSON_FLOAT val 2.5
    json_element typ JSON_STRING val "ab"
    json_element typ JSON_ARRAY val:
        json_array with json_arr_entries:
            json_element typ JSON_INT val 1
            json_element typ JSON_INT val 2

  { "foo": 5, "bar": "ab" }
  json_object with json_obj_entries:
    "foo" content val typ JSON_INT val 5
    "bar" content val typ JSON_STRING val "ab"
 */


// ------- API HERE --------

// Create the base object
// json_type must be JSON_ARRAY or JSON_OBJECT
json_element * json_root(json_type top_type);

// Free the base object and all components
void json_free(json_element *elem);

// Serialize the base
size_t json_serialize(json_element *root, char *buf, size_t buf_len);

// The following add components to an existing element
// You can only add things to JSON_OBJECT and JSON_ARRAY elements
// Name and name_len are ignored if cur is an array, not an object
json_element * append_json_elem_str(json_element *cur,
        char * name, size_t name_len,
        char * val, size_t val_len);
json_element * append_json_elem_flt(json_element *cur,
        char * name, size_t name_len,
        double val);
json_element * append_json_elem_int(json_element *cur,
        char * name, size_t name_len,
        long int val);
json_element * append_json_elem_bool(json_element *cur,
        char * name, size_t name_len,
        char val);
json_element * append_json_elem_null(json_element *cur,
        char * name, size_t name_len);
json_element * append_json_elem_arr(json_element *cur,
        char * name, size_t name_len);
json_element * append_json_elem_obj(json_element *cur,
        char * name, size_t name_len);

#endif
