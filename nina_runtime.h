#ifndef __NINA_RUNTIME_H
#define __NINA_RUNTIME_H

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gc.h>

#define malloc(X) (GC_malloc(X))
#define calloc(X,Y) (GC_malloc((X) * (Y)))
#define realloc(X,Y) (GC_realloc((X), (Y)))
#define free(X) (GC_free(X))

#define __NINA_TRUE 0x3
#define __NINA_FALSE 0x1
enum { __NINA_TYPE_POINTER = 0x0, __NINA_TYPE_INTEGER = 0x1, __NINA_TYPE_FLOAT = 0x2, __NINA_TYPE_STRING = 0x4, __NINA_TYPE_ARRAY = 0x6 };

// ..000 = pointer
// ....1 = integer
// ..010 = float
// ..100 = string
// ..110 = array
typedef union __nina_value { int64_t i; float f; void *p; char *s; struct __nina_array *a; } __nina_value;
typedef struct __nina_array {
    uint32_t len, cap;
    __nina_value *buf;
} __nina_array;

#define __NINA_TYPE(V) (((V).i & 0x1)? __NINA_TYPE_INTEGER : ((V).i & 0x7))

#define __NINA_TO_INTEGER(V) ((V).i >> 1)
#define __NINA_FROM_INTEGER(I) ((__nina_value) { .i = ((I) << 1) | 1 })

float __NINA_TO_FLOAT(__nina_value v) { v.i >>= 2; return v.f; }
__nina_value __NINA_FROM_FLOAT(float f) { __nina_value v = { .f = f }; v.i = (v.i << 3) | __NINA_TYPE_FLOAT; return v; }

#define __NINA_TO_POINTER(V) ((V).p)
__nina_value __NINA_FROM_POINTER(void *p) { __nina_value v = { .p = p }; v.i |= __NINA_TYPE_POINTER; return v; }

char *__NINA_TO_STRING(__nina_value v) { v.i ^= __NINA_TYPE_STRING; return v.s; }
__nina_value __NINA_FROM_STRING(char *p) { __nina_value v = { .s = p }; v.i |= __NINA_TYPE_STRING; return v; }

__nina_array *__NINA_TO_ARRAY(__nina_value v) { v.i ^= __NINA_TYPE_ARRAY; return v.a; }
__nina_value __NINA_FROM_ARRAY(uint32_t l, __nina_value *p) {
    __nina_array *a = malloc(sizeof(*a));
    __nina_value v = { .a = a };
    a->buf = p, a->len = a->cap = l, v.i |= __NINA_TYPE_ARRAY;
    return v;
}

__nina_value write(__nina_value val) {
    switch(__NINA_TYPE(val)){
    case __NINA_TYPE_INTEGER: printf("%ld", __NINA_TO_INTEGER(val)); break;
    case __NINA_TYPE_FLOAT:   printf("%g",  __NINA_TO_FLOAT(val));   break;
    case __NINA_TYPE_STRING:  printf("%s",  __NINA_TO_STRING(val));  break;
    case __NINA_TYPE_POINTER: printf("%p",  __NINA_TO_POINTER(val)); break;
    case __NINA_TYPE_ARRAY: {
        __nina_array *a = __NINA_TO_ARRAY(val);
        printf("[");
        for (int i = 0; i < a->len; ++i) {
            if (i > 0) printf(", ");
            const char is_str = __NINA_TYPE(a->buf[i]) == __NINA_TYPE_STRING;
            if (is_str) printf("\"");
            write(a->buf[i]);
            if (is_str) printf("\"");
        }
        printf("]");
    } break;
    }
    return __NINA_FROM_INTEGER(0);
}

__nina_value __nina_truthy(__nina_value val) {
    switch (__NINA_TYPE(val)) {
    case __NINA_TYPE_INTEGER: return __NINA_FROM_INTEGER(__NINA_TO_INTEGER(val) != 0);
    case __NINA_TYPE_FLOAT:   return __NINA_FROM_INTEGER(__NINA_TO_FLOAT(val) != 0.0);
    case __NINA_TYPE_STRING:  return __NINA_FROM_INTEGER(strlen(__NINA_TO_STRING(val)) != 0);
    case __NINA_TYPE_POINTER: return __NINA_FROM_INTEGER(__NINA_TO_POINTER(val) != NULL);
    case __NINA_TYPE_ARRAY:   return __NINA_FROM_INTEGER(__NINA_TO_ARRAY(val)->len != 0);
    default: return __NINA_FROM_INTEGER(0);
    }
}

__nina_value __nina_equals(__nina_value a, __nina_value b) {
    const int t = __NINA_TYPE(a);
    const __nina_value f = { .i = __NINA_FALSE };
    if (t != __NINA_TYPE(b)) return f;
    switch (t) {
    case __NINA_TYPE_INTEGER: return __NINA_FROM_INTEGER(a.i == b.i);
    case __NINA_TYPE_FLOAT:   return __NINA_FROM_INTEGER(a.i == b.i);
    case __NINA_TYPE_POINTER: return __NINA_FROM_INTEGER(a.i == b.i);
    case __NINA_TYPE_STRING:  return __NINA_FROM_INTEGER(!strcmp(__NINA_TO_STRING(a), __NINA_TO_STRING(b)));
    case __NINA_TYPE_ARRAY: {
        const __nina_array *aa = __NINA_TO_ARRAY(a), *ab = __NINA_TO_ARRAY(b);
        if (aa->len != ab->len) return f;
        for (uint32_t i = 0; i < aa->len; ++i)
            if (__nina_equals(aa->buf[i], ab->buf[i]).i == __NINA_FALSE) return f;
        return __NINA_FROM_INTEGER(1);
    }
    default: return f;
    }
}

enum {
    __NINA_BINOP_ADD, __NINA_BINOP_SUB, __NINA_BINOP_MUL, __NINA_BINOP_DIV, __NINA_BINOP_MOD,
    __NINA_BINOP_LT, __NINA_BINOP_GT, __NINA_BINOP_LE, __NINA_BINOP_GE, __NINA_BINOP_EQ, __NINA_BINOP_NE,
    __NINA_BINOP_AND, __NINA_BINOP_OR, __NINA_BINOP_XOR, __NINA_BINOP_SHL, __NINA_BINOP_SHR,
    __NINA_BINOP_BAND, __NINA_BINOP_BOR,
};

#define __NINA_IOP(V) (((V).i & 0x1)? __NINA_TO_INTEGER(V) : (int64_t)__NINA_TO_FLOAT(V))
#define __NINA_FOP(V) (((V).i & 0x1)? (float)__NINA_TO_INTEGER(V) : __NINA_TO_FLOAT(V))
#define __NINA_BIN(OP) (is_float) ? __NINA_FROM_FLOAT(__NINA_FOP(a) OP __NINA_FOP(b)) \
                                  : __NINA_FROM_INTEGER(__NINA_IOP(a) OP __NINA_IOP(b))
__nina_value __nina_binary(int op, __nina_value a, __nina_value b) {
    assert(op >= __NINA_BINOP_ADD && op <= __NINA_BINOP_BOR);
    switch(op){
    case __NINA_BINOP_BAND: return __NINA_FROM_INTEGER(__nina_truthy(a).i == __NINA_TRUE && __nina_truthy(b).i == __NINA_TRUE);
    case __NINA_BINOP_BOR:  return __NINA_FROM_INTEGER(__nina_truthy(a).i == __NINA_TRUE || __nina_truthy(b).i == __NINA_TRUE);
    case __NINA_BINOP_EQ:   return __nina_equals(a, b);
    case __NINA_BINOP_NE:   return __NINA_FROM_INTEGER(__nina_equals(a, b).i == __NINA_FALSE);
    }
    const int atype = __NINA_TYPE(a), btype = __NINA_TYPE(b);
    assert(atype == __NINA_TYPE_INTEGER || atype == __NINA_TYPE_FLOAT);
    assert(btype == __NINA_TYPE_INTEGER || btype == __NINA_TYPE_FLOAT);
    const int is_float = atype == __NINA_TYPE_FLOAT || btype == __NINA_TYPE_FLOAT;
    switch(op){
    case __NINA_BINOP_AND:  assert(!is_float); return __NINA_FROM_INTEGER(__NINA_TO_INTEGER(a)  & __NINA_TO_INTEGER(b));
    case __NINA_BINOP_OR:   assert(!is_float); return __NINA_FROM_INTEGER(__NINA_TO_INTEGER(a)  | __NINA_TO_INTEGER(b));
    case __NINA_BINOP_XOR:  assert(!is_float); return __NINA_FROM_INTEGER(__NINA_TO_INTEGER(a)  ^ __NINA_TO_INTEGER(b));
    case __NINA_BINOP_SHL:  assert(!is_float); return __NINA_FROM_INTEGER(__NINA_TO_INTEGER(a) << __NINA_TO_INTEGER(b));
    case __NINA_BINOP_SHR:  assert(!is_float); return __NINA_FROM_INTEGER(__NINA_TO_INTEGER(a) >> __NINA_TO_INTEGER(b));
    case __NINA_BINOP_MOD:  assert(!is_float); return __NINA_FROM_INTEGER(__NINA_TO_INTEGER(a)  % __NINA_TO_INTEGER(b));
    case __NINA_BINOP_ADD:  return __NINA_BIN(+);
    case __NINA_BINOP_SUB:  return __NINA_BIN(-);
    case __NINA_BINOP_MUL:  return __NINA_BIN(*);
    case __NINA_BINOP_DIV:  return __NINA_BIN(/);
    case __NINA_BINOP_LT:   return __NINA_BIN(<);
    case __NINA_BINOP_GT:   return __NINA_BIN(>);
    case __NINA_BINOP_LE:   return __NINA_BIN(<=);
    case __NINA_BINOP_GE:   return __NINA_BIN(>=);
    }
    return __NINA_FROM_INTEGER(0);
}
#undef __NINA_IOP
#undef __NINA_FOP
#undef __NINA_BIN

#endif
