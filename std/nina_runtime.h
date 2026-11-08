#ifndef __NINA_RUNTIME_H
#define __NINA_RUNTIME_H

#include <assert.h>
#include <stdarg.h>
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

typedef __nina_value (*__nina_method)(__nina_value self, __nina_value va_args);

typedef struct __nina_global_object {
    uint32_t num_fields, num_methods;
    const char *name, **field_names, **method_names;
    __nina_value *field_values;
    __nina_method *method_values;
} __nina_global_object;

typedef struct __nina_instance {
    __nina_global_object *_class_metadata;
    __nina_value *fields;
} __nina_instance;

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
    assert(a != NULL && "OUT OF MEMORY");
    __nina_value v = { .a = a };
    a->buf = p, a->len = a->cap = l, v.i |= __NINA_TYPE_ARRAY;
    return v;
}

static uint64_t __NINA_GLOBAL_OBJECT_COUNTER = 0;
static uint64_t __NINA_GLOBAL_OBJECT_ALLOC = 0;
static __nina_global_object **__NINA_GLOBAL_OBJECT_LIST;

__nina_global_object *__NINA_DEFINE_GLOBAL_OBJECT(const char *name) {
    if (__NINA_GLOBAL_OBJECT_ALLOC == 0) {
        __NINA_GLOBAL_OBJECT_LIST = malloc(sizeof(__nina_global_object*) * (__NINA_GLOBAL_OBJECT_ALLOC = 255));
    } else if (__NINA_GLOBAL_OBJECT_COUNTER >= __NINA_GLOBAL_OBJECT_ALLOC) {
        __NINA_GLOBAL_OBJECT_LIST = realloc(__NINA_GLOBAL_OBJECT_LIST,
                sizeof(__nina_global_object*) * (__NINA_GLOBAL_OBJECT_ALLOC *= 2));
    }
    __nina_global_object *obj = malloc(sizeof(__nina_global_object));
    assert(obj != NULL && "OUT OF MEMORY");
    obj->name = strdup(name);
    obj->num_fields = obj->num_methods = 0;
    obj->field_names = obj->method_names = NULL;
    obj->field_values = NULL, obj->method_values = NULL;
    return __NINA_GLOBAL_OBJECT_LIST[__NINA_GLOBAL_OBJECT_COUNTER++] = obj;
}

__nina_global_object *__NINA_GET_GLOBAL_OBJECT(uint64_t id) {
    assert(id < __NINA_GLOBAL_OBJECT_COUNTER && "INVALID OBJECT ID");
    return __NINA_GLOBAL_OBJECT_LIST[id];
}

__nina_value __NINA_INSTANTIATE_OBJECT(uint64_t id, __nina_value args) {
    __nina_global_object *class_data = __NINA_GET_GLOBAL_OBJECT(id);
    __nina_instance *obj = malloc(sizeof(__nina_instance));
    assert(obj != NULL && "OUT OF MEMORY");
    const size_t fields_sz = sizeof(__nina_value) * class_data->num_fields;
    obj->_class_metadata = class_data, obj->fields = malloc(fields_sz);
    assert(obj->fields != NULL && "OUT OF MEMORY");
    memcpy(obj->fields, class_data->field_values, fields_sz);
    if (class_data->num_methods) {
        class_data->method_values[0](__NINA_FROM_POINTER(obj), args);
    }
    return __NINA_FROM_POINTER(obj);
}

void __NINA_REGISTER_OBJECT_FIELD(__nina_global_object *obj, const char *name, __nina_value value) {
    if ((obj->num_fields++) == 0) {
        obj->field_names = malloc(sizeof(char*) * obj->num_fields);
        obj->field_values = malloc(sizeof(__nina_value) * obj->num_fields);
    } else {
        obj->field_names = realloc(obj->field_names, obj->num_fields);
        obj->field_values = realloc(obj->field_values, obj->num_fields);
    }
    obj->field_names[obj->num_fields-1] = strdup(name);
    obj->field_values[obj->num_fields-1] = value;
}

void __NINA_REGISTER_OBJECT_METHOD(__nina_global_object *obj, const char *name, __nina_method method) {
    if ((obj->num_methods++) == 0) {
        obj->method_names = malloc(sizeof(char*) * obj->num_methods);
        obj->method_values = malloc(sizeof(__nina_method) * obj->num_methods);
    } else {
        obj->method_names = realloc(obj->method_names, obj->num_methods);
        obj->method_values = realloc(obj->method_values, obj->num_methods);
    }
    obj->method_names[obj->num_methods-1] = strdup(name);
    obj->method_values[obj->num_methods-1] = method;
}

__nina_method __NINA_GET_OBJECT_METHOD(__nina_global_object *obj, const char *name) {
    assert(obj->num_methods != 0 && "NO METHODS FOR OBJECT");
    for (uint32_t i = 0; i < obj->num_methods; ++i) {
        if (strcmp(obj->method_names[i], name)) {
            return obj->method_values[i];
        }
    }
    assert(0 && "INVALID METHOD NAME");
    return NULL;
}

__nina_value *__NINA_GET_INSTANCE_FIELD(__nina_value val, const char *name) {
    assert(__NINA_TYPE(val) == __NINA_TYPE_POINTER);
    __nina_instance *obj = __NINA_TO_POINTER(val);
    for (uint32_t i = 0; i < obj->_class_metadata->num_fields; ++i) {
        if (strcmp(obj->_class_metadata->field_names[i], name)) {
            return &obj->fields[i];
        }
    }
    assert(0 && "INVALID FIELD NAME");
    return NULL;
}

__nina_value __NINA_SET_INSTANCE_FIELD(__nina_value val, const char *name, __nina_value value) {
    return *__NINA_GET_INSTANCE_FIELD(val, name) = value;
}

__nina_value __NINA_CALL_INSTANCE_METHOD(__nina_value val, const char *name, __nina_value args) {
    assert(__NINA_TYPE(val) == __NINA_TYPE_POINTER);
    __nina_instance *obj = __NINA_TO_POINTER(val);
    __nina_method method = __NINA_GET_OBJECT_METHOD(obj->_class_metadata, name);
    return method(__NINA_FROM_POINTER(obj), args);
}

__nina_value __NINA_WRITE_VALUE(__nina_value val) {
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
            __NINA_WRITE_VALUE(a->buf[i]);
            if (is_str) printf("\"");
        }
        printf("]");
    } break;
    }
    return __NINA_FROM_INTEGER(0);
}

__nina_value write(__nina_value va_args) {
    assert(__NINA_TYPE(va_args) == __NINA_TYPE_ARRAY);
    __nina_array *args = __NINA_TO_ARRAY(va_args);
    for (uint32_t i = 0; i < args->len; ++i) __NINA_WRITE_VALUE(args->buf[i]);
    return __NINA_FROM_INTEGER(0);
}

__nina_value length(__nina_value val) {
    switch(__NINA_TYPE(val)){
    case __NINA_TYPE_STRING: return __NINA_FROM_INTEGER(strlen(__NINA_TO_STRING(val)));
    case __NINA_TYPE_ARRAY: return __NINA_FROM_INTEGER(__NINA_TO_ARRAY(val)->len);
    default: return __NINA_FROM_INTEGER(0);
    }
}

__nina_value append(__nina_value array, __nina_value va_args) {
    assert(__NINA_TYPE(array) == __NINA_TYPE_ARRAY);
    assert(__NINA_TYPE(va_args) == __NINA_TYPE_ARRAY);
    __nina_array *arr = __NINA_TO_ARRAY(array), *args = __NINA_TO_ARRAY(va_args);
    if (arr->len+args->len >= arr->cap) arr->buf = realloc(arr->buf, (arr->cap += args->cap) * sizeof(__nina_value));
    for (uint32_t i = 0; i < args->len; ++i) arr->buf[arr->len++] = args->buf[i];
    return array;
}

__nina_value pop(__nina_value array) {
    assert(__NINA_TYPE(array) == __NINA_TYPE_ARRAY);
    __nina_array *arr = __NINA_TO_ARRAY(array);
    assert(arr->len > 0);
    return arr->buf[--arr->len];
}

__nina_value __NINA_MAKE_ARRAY(uint32_t cap, ...) {
    __nina_value *arr = malloc(sizeof(*arr) * cap);
    va_list ap;
    va_start(ap, cap);
    for (uint32_t i = 0; i < cap; ++i) {
        /* this is hacky, but it should work for compilers that don't
         * support `va_arg` with non-scalar types (*ahem cproc*). */
        arr[i] = (__nina_value) { .i = va_arg(ap, int64_t) };
    }
    va_end(ap);
    return __NINA_FROM_ARRAY(cap, arr);
}

__nina_value __NINA_SET_ARRAY(__nina_value arr, __nina_value idx, __nina_value val) {
    assert(__NINA_TYPE(arr) == __NINA_TYPE_ARRAY);
    assert(__NINA_TYPE(idx) == __NINA_TYPE_INTEGER);
    __nina_array *a = __NINA_TO_ARRAY(arr);
    int64_t i = __NINA_TO_INTEGER(idx);
    assert(i >= 0 && i < a->len);
    return a->buf[i] = val;
}

__nina_value __NINA_GET_ARRAY(__nina_value arr, __nina_value idx) {
    assert(__NINA_TYPE(arr) == __NINA_TYPE_ARRAY);
    assert(__NINA_TYPE(idx) == __NINA_TYPE_INTEGER);
    __nina_array *a = __NINA_TO_ARRAY(arr);
    int64_t i = __NINA_TO_INTEGER(idx);
    assert(i >= 0 && i < a->len);
    return a->buf[i];
}

__nina_value __NINA_TRUTHY(__nina_value val) {
    switch (__NINA_TYPE(val)) {
    case __NINA_TYPE_INTEGER: return __NINA_FROM_INTEGER(__NINA_TO_INTEGER(val) != 0);
    case __NINA_TYPE_FLOAT:   return __NINA_FROM_INTEGER(__NINA_TO_FLOAT(val) != 0.0);
    case __NINA_TYPE_STRING:  return __NINA_FROM_INTEGER(strlen(__NINA_TO_STRING(val)) != 0);
    case __NINA_TYPE_POINTER: return __NINA_FROM_INTEGER(__NINA_TO_POINTER(val) != NULL);
    case __NINA_TYPE_ARRAY:   return __NINA_FROM_INTEGER(__NINA_TO_ARRAY(val)->len != 0);
    default: return __NINA_FROM_INTEGER(0);
    }
}

__nina_value __NINA_EQUALS(__nina_value a, __nina_value b) {
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
            if (__NINA_EQUALS(aa->buf[i], ab->buf[i]).i == __NINA_FALSE) return f;
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
__nina_value __NINA_BINARY(int op, __nina_value a, __nina_value b) {
    assert(op >= __NINA_BINOP_ADD && op <= __NINA_BINOP_BOR);
    switch(op){
    case __NINA_BINOP_BAND: return __NINA_FROM_INTEGER(__NINA_TRUTHY(a).i == __NINA_TRUE && __NINA_TRUTHY(b).i == __NINA_TRUE);
    case __NINA_BINOP_BOR:  return __NINA_FROM_INTEGER(__NINA_TRUTHY(a).i == __NINA_TRUE || __NINA_TRUTHY(b).i == __NINA_TRUE);
    case __NINA_BINOP_EQ:   return __NINA_EQUALS(a, b);
    case __NINA_BINOP_NE:   return __NINA_FROM_INTEGER(__NINA_EQUALS(a, b).i == __NINA_FALSE);
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
