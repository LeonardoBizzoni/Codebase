fn os_Handle fs_open(String8 filepath, os_AccessFlags flags) {
  os_Handle result = {0};
  Scratch scratch = ScratchBegin(0, 0);
  String16 path = UTF16From8(scratch.arena, filepath);
  SECURITY_ATTRIBUTES security_attributes = {sizeof(SECURITY_ATTRIBUTES), 0, 0};
  DWORD access_flags = 0;
  DWORD share_mode = 0;
  DWORD creation_disposition = OPEN_EXISTING;
  
  if(flags & os_acfRead) { access_flags |= GENERIC_READ;}
  if(flags & os_acfWrite) { access_flags |= GENERIC_WRITE; creation_disposition = CREATE_ALWAYS; }
  if(flags & os_acfExecute) { access_flags |= GENERIC_EXECUTE; }
  if(flags & os_acfAppend) { creation_disposition = OPEN_ALWAYS; access_flags |= FILE_APPEND_DATA; }
  if(flags & os_acfShareRead) { share_mode |= FILE_SHARE_READ; }
  if(flags & os_acfShareWrite) { share_mode |= FILE_SHARE_WRITE; }
  
  HANDLE file = CreateFileW((WCHAR*)path.str, access_flags, share_mode, 
                            &security_attributes, creation_disposition, FILE_ATTRIBUTE_NORMAL, 0);
  if(file != INVALID_HANDLE_VALUE) {
    result.fd[0] = (u64)file;
  }
  
  ScratchEnd(scratch);
  return result;
}

fn String8 fs_read(Arena *arena, os_Handle file) {
  String8 result = {0};
  
  if(file.fd[0] == 0) { return result; }
  
  LARGE_INTEGER file_size = {0};
  HANDLE handle = (HANDLE)file.fd[0];
  
  GetFileSizeEx(handle, &file_size);
  u64 to_read = file_size.QuadPart;
  u64 total_read = 0;
  u8 *ptr = New(arena, u8, to_read);
  for(;total_read < to_read;)
  {
    u64 amount64 = to_read - total_read;
    u32 amount32 = amount64 > U32_MAX ? U32_MAX : (u32)amount64;
    u32 bytes_read;
    OVERLAPPED overlapped = {0};
    BOOL read_result = ReadFile(handle, ptr + total_read, amount32, &bytes_read, &overlapped);
    total_read += bytes_read;
    
    if(bytes_read != amount32 && !read_result)
    {
      break;
    }
  }
  
  if(total_read == to_read)
  {
    result.str = ptr;
    result.size = to_read;
  }
  
  return result;
}

fn bool fs_write(os_Handle file, String8 content) {
  bool result = false;
  
  if(file.fd[0] == 0) { return result; }
  
  HANDLE handle = (HANDLE)file.fd[0];
  u64 to_write = content.size;
  u64 total_write = 0;
  for(;total_write < to_write;)
  {
    u64 amount64 = to_write - total_write;
    u32 amount32_to_write = amount64 > U32_MAX ? U32_MAX : (u32)amount64;
    u32 bytes_written = 0;
    BOOL write_result = WriteFile(handle, content.str + total_write, amount32_to_write, &bytes_written, 0);
    total_write += bytes_written;
    if(bytes_written != amount32_to_write && !write_result)
    {
      break;
    }
  }
  
  if(total_write == to_write)
  {
    result = true;
  }
  
  return result;
}

fn fs_Properties fs_getProp(os_Handle file) {
  // TODO(km): need to fill owner,group, and permission flags
  fs_Properties properties = {0};
  
  return properties;
}

fn File fs_fileOpen(Arena *arena, String8 filepath) {
  File result = {0};
  Scratch scratch = ScratchBegin(&arena, 1);
  String16 path = UTF16From8(scratch.arena, filepath);
  
  SECURITY_ATTRIBUTES security_attributes = {sizeof(SECURITY_ATTRIBUTES), 0, 0};
  DWORD access_flags = GENERIC_READ | GENERIC_WRITE;
  DWORD share_mode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
  HANDLE file = CreateFileW((WCHAR*)path.str, access_flags, share_mode, 
                            &security_attributes, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
  
  if(file != INVALID_HANDLE_VALUE) {
    LARGE_INTEGER file_size;
    if(GetFileSizeEx(file, &file_size)) {
      HANDLE file_mapped = CreateFileMappingW(file, 0, PAGE_READWRITE, 0, 0, 0);
      if(file_mapped != INVALID_HANDLE_VALUE) {
        result.handle.fd[0] = (u64)file_mapped;
        result.path = filepath;
        result.prop = fs_getProp(result.handle);
        result.content = (u8*)MapViewOfFileEx(file_mapped, FILE_MAP_ALL_ACCESS, 0, 0, 0, 0);
      }
    }
  }
  return result;
}

fn OS_FileIter* 
os_file_iter_begin(Arena *arena, String8 path)
{
  Scratch scratch = ScratchBegin(&arena, 1);
  StringStream list = {0};
  
  stringstreamAppend(scratch.arena, &list, path);
  stringstreamAppend(scratch.arena, &list, Strlit("\\*"));
  path = str8FromStream(scratch.arena, list);
  
  String16 path16 = UTF16From8(scratch.arena, path);
  WIN32_FIND_DATAW file_data = {0};
  
  OS_W32_FileIter *iter = New(arena, OS_W32_FileIter);
  iter->handle = FindFirstFileW((WCHAR*)path16.str, &file_data);
  iter->file_data = file_data;
  
  OS_FileIter *result = (OS_FileIter*)iter;
  return result;
}

fn bool 
os_file_iter_next(Arena *arena, OS_FileIter *iter, OS_FileInfo *info_out)
{
  bool result = false;
  OS_W32_FileIter *w32_iter = (OS_W32_FileIter*)iter;
  for(;!w32_iter->done;)
  {
    WCHAR *file_name = w32_iter->file_data.cFileName;
    bool is_dot = file_name[0] == '.';
    bool is_dotdot = file_name[0] == '.' && file_name[1] == '.';
    
    bool valid = (!is_dot && !is_dotdot);
    WIN32_FIND_DATAW data;
    if(valid)
    {
      memcpy(&data, &w32_iter->file_data, sizeof(data));
    }
    
    if(!FindNextFileW(w32_iter->handle, &w32_iter->file_data)) 
    {
      w32_iter->done = 1;
    }
    
    if(valid)
    {
      info_out->name = UTF8From16(arena, str16_cstr((u16*)data.cFileName));
      result = true;
      break;
    }
  }
  
  return result;
}

fn void 
os_file_iter_end(OS_FileIter *iter)
{
  OS_W32_FileIter *w32_iter = (OS_W32_FileIter*)iter;
  FindClose(w32_iter->handle);
}
