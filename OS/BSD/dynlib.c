#include <dlfcn.h>

inline fn void *dynlib_open(String8 path) {
  return dlopen((char *)path.str, RTLD_LAZY);
}

inline fn void *dynlib_lookup(void *handle, String8 symbol) {
  Assert(!dlerror());
  Assert(handle);
  void *res = dlsym(handle, (char *)symbol.str);
  Assert(res);
  Assert(!dlerror());

  return res;
}

inline fn void dynlib_close(void *handle) {
  Assert(handle);
  i8 res = dlclose(handle);
  Assert(!res);
}
