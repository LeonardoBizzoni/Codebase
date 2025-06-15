#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

#include <dirent.h>
#include <sys/types.h>

// =============================================================================
// File reading and writing/appending
fn OS_Handle fs_open(String8 filepath, OS_AccessFlags flags) {
  i32 access_flags = 0;

  if((flags & OS_acfRead) && (flags & OS_acfWrite)) {
    access_flags |= O_RDWR;
  } else if(flags & OS_acfRead) {
    access_flags |= O_RDONLY;
  } else if(flags & OS_acfWrite) {
    access_flags |= O_WRONLY | O_CREAT | O_TRUNC;
  }
  if(flags & OS_acfAppend) { access_flags |= O_APPEND | O_CREAT; }

  i32 fd = open((char*)filepath.str, access_flags,
                S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  if(fd < 0) {
    fd = 0;
  } else {
    // NOTE(lb): there is no other way that i know of to keep track of the path
    // other than tracking it myself. Getting the fullpath would require either
    // allocating more memory (preferred but still shit) or using the system arena
    // (can't remove the path from the arena without introducing internal fragmentation),
    // so paths will be relative to the cwd until a better solution.
    bsd_state.filemap[fd] = filepath;
  }

  OS_Handle res = {(u64)fd};
  return res;
}

fn bool fs_close(OS_Handle fd) {
  if (close(fd.h[0]) == 0) {
    bsd_state.filemap[fd.h[0]].str = 0;
    bsd_state.filemap[fd.h[0]].size = 0;
    return true;
  } else {
    return false;
  }
}

fn String8 fs_readVirtual(Arena *arena, OS_Handle file, usize size) {
  int fd = file.h[0];
  String8 result = {0};
  if(!fd) { return result; }

  u8 *buffer = New(arena, u8, size);
  if(pread(fd, buffer, size, 0) >= 0) {
    result.str = buffer;
    result.size = str8len((char *)buffer);
  }

  return result;
}

fn String8 fs_read(Arena *arena, OS_Handle file) {
  String8 result = {0};
  int fd = file.h[0];
  if(!fd) { return result; }

  struct stat file_stat;
  if (fstat(fd, &file_stat) == 0) {
    u8 *buffer = New(arena, u8, file_stat.st_size);
    if(pread(fd, buffer, file_stat.st_size, 0) >= 0) {
      result.str = buffer;
      result.size = file_stat.st_size;
    }
  }

  return result;
}

fn bool fs_write(OS_Handle file, String8 content) {
  if(!file.h[0]) { return false; }
  return write(file.h[0], content.str, content.size) == (isize)content.size;
}

fn FS_Properties fs_getProp(OS_Handle file) {
  FS_Properties res = {0};
  if(!file.h[0]) { return res; }

  struct stat file_stat;
  if (fstat((i32)file.h[0], &file_stat) == 0) {
    res = bsd_propertiesFromStat(&file_stat);
  }
  return res;
}

fn String8 fs_pathFromHandle(Arena *arena, OS_Handle fd) {
  return bsd_state.filemap[fd.h[0]];
}

// =============================================================================
// Memory mapping files for easier and faster handling

fn File fs_fopen(Arena *arena, OS_Handle fd) {
  File file = {0};
  file.file_handle = fd;
  file.path = fs_pathFromHandle(arena, fd);
  file.prop = fs_getProp(file.file_handle);
  file.content = (u8 *)mmap(0, ClampBot(file.prop.size, 1), PROT_READ | PROT_WRITE,
                            MAP_SHARED, fd.h[0], 0);
  file.mmap_handle.h[0] = (u64)file.content;

  return file;
}

fn File fs_fopenTmp(Arena *arena) {
  char path[] = "/tmp/base-XXXXXX";
  i32 fd = mkstemp(path);

  String8 pathstr = {
    .str = New(arena, u8, Arrsize(path)),
    .size = Arrsize(path),
  };
  memcopy(pathstr.str, path, Arrsize(path));

  File file = {0};
  file.file_handle.h[0] = fd;
  file.path = pathstr;
  file.prop = fs_getProp(file.file_handle);
  file.content = (u8*)mmap(0, ClampBot(file.prop.size, 1), PROT_READ | PROT_WRITE,
                           MAP_SHARED, fd, 0);
  file.mmap_handle.h[0] = (u64)file.content;
  return file;
}

fn bool fs_fclose(File *file) {
  return munmap((void *)file->mmap_handle.h[0], file->prop.size) == 0 &&
         close(file->file_handle.h[0]) >= 0;
}

inline fn bool fs_fresize(File *file, usize size) {
  if (ftruncate(file->file_handle.h[0], size) < 0) {
    return false;
  }

  (void)munmap(file->content, file->prop.size);
  return (bool)(file->content = (u8*)mmap(0, size, PROT_READ | PROT_WRITE,
                                          MAP_SHARED, file->file_handle.h[0], 0));
}

inline fn void fs_fwrite(File *file, String8 content) {
  if (fs_fresize(file, content.size)) { file->prop.size = content.size; }
  memzero(file->content + content.size, ClampBot(0, (isize)file->prop.size - (isize)content.size));
  (void)memcopy(file->content, content.str, content.size);
}

inline fn bool fs_fileHasChanged(File *file) {
  FS_Properties prop = fs_getProp(file->file_handle);
  return (file->prop.last_access_time != prop.last_access_time) ||
         (file->prop.last_modification_time != prop.last_modification_time) ||
         (file->prop.last_status_change_time != prop.last_status_change_time);
}

fn bool fs_fdelete(File *file) {
  return unlink((char *) file->path.str) >= 0 && fs_fclose(file);
}

fn bool fs_frename(File *file, String8 to) {
  return rename((char *) file->path.str, (char *) to.str) >= 0;
}

// =============================================================================
// Misc operation on the filesystem
inline fn bool fs_delete(String8 filepath) {
  Assert(filepath.size != 0);
  return unlink((char *)filepath.str) >= 0;
}

inline fn bool fs_rename(String8 filepath, String8 to) {
  Assert(filepath.size != 0 && to.size != 0);
  return rename((char *)filepath.str, (char *)to.str) >= 0;
}

fn bool fs_mkdir(String8 path) {
  Assert(path.size != 0);
  return mkdir((char *)path.str,
               S_IRWXU | (S_IRGRP | S_IXGRP) | (S_IROTH | S_IXOTH)) >= 0;
}

fn bool fs_rmdir(String8 path) {
  Assert(path.size != 0);
  return rmdir((char *)path.str) >= 0;
}

// =============================================================================
// File iteration
