fn OS_Library os_lib_open(String8 path){
  OS_Library result = {0};
  Scratch scratch = ScratchBegin(0,0);
  String16 path16 = UTF16From8(scratch.arena, path);
  HMODULE module = LoadLibraryExW((WCHAR*)path16.str, 0, 0);
  if(module != 0){
    result.v[0] = (u64)module;
  }
  ScratchEnd(scratch);
  return result;
}

fn VoidFunc* os_lib_lookup(OS_Library lib, String8 symbol){
  Scratch scratch = ScratchBegin(0,0);
  char *symbol_cstr = (char*)str8_copy(scratch.arena, symbol).str;
  HMODULE module = (HMODULE)lib.v[0];
  VoidFunc *result = (VoidFunc*)GetProcAddress(module, symbol_cstr);
  ScratchEnd(scratch);
  return result;
}

fn void os_lib_close(OS_Library lib){
  HMODULE module = (HMODULE)lib.v[0];
  BOOL result = FreeLibrary(module);
  (void)result;
}
