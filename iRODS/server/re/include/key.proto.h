/* For copyright information please refer to files in the COPYRIGHT directory
 */

#ifndef KEY_PROTO_H
#define KEY_PROTO_H

#define KEY_SIZE 1024

#endif

#define RE_STRUCT_FUNC_PROTO(T) \
    void CONCAT(key, T)(T *node, char key[KEY_SIZE])

#define RE_STRUCT_GENERIC_FUNC_PROTO(T, cpfn) \
    RE_STRUCT_FUNC_PROTO(T)

