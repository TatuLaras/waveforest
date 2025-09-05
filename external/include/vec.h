#ifndef _VEC
#define _VEC

/*
This file includes two macros to generate dynamic array implementations for any
arbitrary fixed-size datatype.

VEC_DECLARE(datatype, name)
This will generate the struct and function declarations for the implementation.
You usually want this in a header file. The functions will be prefixed with
`prefix`, for example prefix value of 'intvec' will have functions such as
intvec_init, intvec_append, intvec_get and intvec_free. The generated type will
have name `name`.

VEC_IMPLEMENT(datatype, name)
This will provide implementations for the functions declared using the previous
macro. You usually want this in a source file.


Functions:

{prefix}_init(void)
Will initialize the datatype. Use this before any of the other functions.

{prefix}_append({name} *vec, {datatype} data)
Will append `data` to the end of the dynamic array `vec`.

{prefix}_get({name} *vec, size_t {index})
Will get an element from the dynamic array `vec` at `index`. Returns a null
pointer if index is out of range.

{prefix}_free({name} *vec)
Will free memory associated with this datatype.

*/

#include <stddef.h>
#include <stdlib.h>

#define VEC_DECLARE(datatype, name, prefix)                                    \
    typedef struct {                                                           \
        datatype *data;                                                        \
        size_t data_allocated;                                                 \
        size_t data_used;                                                      \
    } name;                                                                    \
                                                                               \
    name prefix##_init(void);                                                  \
    size_t prefix##_append(name *vec, datatype data);                          \
    datatype *prefix##_get(name *vec, size_t index);                           \
    void prefix##_free(name *vec);

#define VEC_IMPLEMENT(datatype, name, prefix)                                  \
    name prefix##_init(void) {                                                 \
        name vec = {                                                           \
            .data = malloc(4 * sizeof(datatype)),                              \
            .data_allocated = 4,                                               \
        };                                                                     \
        if (!vec.data)                                                         \
            exit(1);                                                           \
        return vec;                                                            \
    }                                                                          \
                                                                               \
    size_t prefix##_append(name *vec, datatype data) {                         \
        if (vec->data_used >= vec->data_allocated) {                           \
            vec->data_allocated *= 2;                                          \
            vec->data =                                                        \
                realloc(vec->data, vec->data_allocated * sizeof(datatype));    \
            if (!vec->data)                                                    \
                exit(1);                                                       \
        }                                                                      \
                                                                               \
        vec->data[vec->data_used++] = data;                                    \
        return vec->data_used - 1;                                             \
    }                                                                          \
                                                                               \
    datatype *prefix##_get(name *vec, size_t index) {                          \
        if (index >= vec->data_used)                                           \
            return 0;                                                          \
                                                                               \
        return vec->data + index;                                              \
    }                                                                          \
                                                                               \
    void prefix##_free(name *vec) {                                            \
        if (vec->data) {                                                       \
            free(vec->data);                                                   \
            vec->data = 0;                                                     \
        }                                                                      \
    }

#endif
