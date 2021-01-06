#ifndef BUILTIN_H
#define BUILTIN_H

enum enum_bool_t {
    false = 0,
    true = 1,
};

#define bool_t uint8_t

#define GLOBALS_STRUCT(name)        name##_globals_s
#define GLOBALS_TYPE(name)          name##_globals_t
#define GLOBALS(name)               name##_globals

#define GLOBALS_STRUCT_BEGIN(name)  typedef struct GLOBALS_STRUCT(name) {
#define GLOBALS_STRUCT_END(name) }  GLOBALS_TYPE(name);

#define GLOBALS_DECLARE(name)       GLOBALS_TYPE(name) GLOBALS(name);
#define GLOBALS_GET(name, value)    GLOBALS(name).value
#define GLOBALS_BULK(name)          &GLOBALS(name)

#endif
