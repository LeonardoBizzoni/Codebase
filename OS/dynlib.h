#ifndef BASE_OS_DYNLIB
#define BASE_OS_DYNLIB

typedef struct{
  u64 v[1];
} OS_Library;

typedef void VoidFunc(void);

fn OS_Library os_lib_open(String8 path);
fn VoidFunc *os_lib_lookup(OS_Library lib, String8 symbol);
fn void os_lib_close(OS_Library lib);

#endif
