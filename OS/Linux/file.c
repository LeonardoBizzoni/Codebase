#ifndef BASE_OS_FILE
#define BASE_OS_FILE

#include "../file.h"

#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

#include <dirent.h>
#include <sys/types.h>

// =============================================================================
// File reading and writing/appending
fn String8 fs_read(Arena *arena, String8 filepath) {
  Assert(arena);
  if (filepath.size == 0) {
    return (String8) {0};
  }

  i32 fd = open((char *)filepath.str, O_RDONLY);
  if (fd < 0) {
    (void)close(fd);
    return (String8) {0};
  }

  struct stat file_stat;
  if (stat((char *)filepath.str, &file_stat) < 0) {
    (void)close(fd);
    return (String8) {0};
  }

  String8 res = { .str = Newarr(arena, u8, file_stat.st_size) };
  res.size = read(fd, res.str, file_stat.st_size);

  (void)close(fd);
  return res;
}

fn String8 fs_fread(Arena *arena, isize fd) {
  Assert(arena);

  struct stat file_stat;
  if (fstat(fd, &file_stat) < 0) {
    (void)close(fd);
    return (String8) {0};
  }

  String8 res = { .str = Newarr(arena, u8, file_stat.st_size) };
  res.size = read(fd, res.str, file_stat.st_size);

  (void)close(fd);
  return res;
}

fn isize fs_write(String8 filepath, String8 content) {
  if (filepath.size == 0) {
    return -1;
  }

  i32 fd = open((char *)filepath.str, O_WRONLY | O_CREAT | O_TRUNC,
                S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  if (fd < 0) {
    (void)close(fd);
    return -1;
  }

  if (write(fd, content.str, content.size) != content.size) {
    (void)close(fd);
    return -1;
  }

  return fd;
}

fn isize fs_writeStream(String8 filepath, StringStream content) {
  StringNode *start = content.first++;
  isize fd = fs_write(filepath, start->value);
  if (fd < 0) {
    return -1;
  }

  for (; start < content.first + content.size; ++start) {
    if (!fs_fappend(fd, start->value)) {
      (void)close(fd);
      return -1;
    }
  }

  return fd;
}

fn isize fs_append(String8 filepath, String8 content) {
  Assert(filepath.size);
  i32 fd = open((char *)filepath.str, O_WRONLY | O_CREAT | O_APPEND,
                S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  if (fd < 0) {
    (void)close(fd);
    return -1;
  }

  if (write(fd, content.str, content.size) != content.size) {
    (void)close(fd);
    return -1;
  }

  return fd;
}

fn bool fs_fappend(isize fd, String8 content) {
  if (write(fd, content.str, content.size) != content.size) {
    (void)close(fd);
    return false;
  }

  return true;
}

fn isize fs_appendStream(String8 filepath, StringStream content) {
  StringNode *start = content.first;
  isize fd = fs_append(filepath, start->value);

  for (start = start->next; start; start = start->next) {
    if (!fs_fappend(fd, start->value)) {
      (void)close(fd);
      return -1;
    }
  }

  return fd;
}

fn FileProperties fs_getProp(String8 filepath) {
  FileProperties res = {0};
  if (filepath.size == 0) {
    return res;
  }

  struct stat file_stat;
  if (stat((char *)filepath.str, &file_stat) < 0) {
    return res;
  }

  res.ownerID = file_stat.st_uid;
  res.groupID = file_stat.st_gid;
  res.size = (usize)file_stat.st_size;
  res.last_access_time = (u64)file_stat.st_atime;
  res.last_modification_time = (u64)file_stat.st_mtime;
  res.last_status_change_time = (u64)file_stat.st_ctime;

  res.user = ACF_Unknown;
  if (file_stat.st_mode & S_IRUSR) {
    res.user = ACF_Read;
  }
  if (file_stat.st_mode & S_IWUSR) {
    res.user = (AccessFlag)(res.user | ACF_Write);
  }
  if (file_stat.st_mode & S_IXUSR) {
    res.user = (AccessFlag)(res.user | ACF_Execute);
  }

  res.group = ACF_Unknown;
  if (file_stat.st_mode & S_IRGRP) {
    res.group = ACF_Read;
  }
  if (file_stat.st_mode & S_IWGRP) {
    res.group = (AccessFlag)(res.group | ACF_Write);
  }
  if (file_stat.st_mode & S_IXGRP) {
    res.group = (AccessFlag)(res.group | ACF_Execute);
  }

  res.other = ACF_Unknown;
  if (file_stat.st_mode & S_IROTH) {
    res.other = ACF_Read;
  }
  if (file_stat.st_mode & S_IWOTH) {
    res.other = (AccessFlag)(res.other | ACF_Write);
  }
  if (file_stat.st_mode & S_IXOTH) {
    res.other = (AccessFlag)(res.other | ACF_Execute);
  }

  return res;
}

// =============================================================================
// Temporary files

#define TMP_FILE_TEMPLATE "/tmp/base-XXXXXX"

inline fn isize fs_makeTmpFd() {
  char path[] = TMP_FILE_TEMPLATE;
  isize res = mkstemp(path);
  printf("\t\t%s\n", path);
  return res;
}

fn String8 fs_makeTmpFile(Arena *arena, isize *fd) {
  char path[] = TMP_FILE_TEMPLATE;
  isize _fd = mkstemp(path);

  if (!fd) {
    (void)close(_fd);
  } else {
    *fd = _fd;
  }

  u8 *pathstr = Newarr(arena, u8, Arrsize(path));
  memCopy(pathstr, path, Arrsize(path));
  return (String8) {
    .str = pathstr,
    .size = Arrsize(path),
  };
}

fn String8 fs_writeTmpFile(Arena* arena, String8 content) {
  char path[] = TMP_FILE_TEMPLATE;
  i32 fd = mkstemp(path);

  (void)write(fd, content.str, content.size);
  (void)close(fd);

  u8 *pathstr = Newarr(arena, u8, Arrsize(path));
  memCopy(pathstr, path, Arrsize(path));
  return (String8) {
    .str = pathstr,
    .size = Arrsize(path),
  };
}

// =============================================================================
// Memory mapping files for easier and faster handling

fn File *fs_open(Arena *arena, String8 filepath, void *location) {
  Assert(arena);
  if (filepath.size == 0) {
    return 0;
  }

  i32 fd = open((char *)filepath.str, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  if (fd < 0) {
    (void)close(fd);
    return 0;
  }

  File *memfile = New(arena, File);
  memfile->path = filepath;
  memfile->descriptor = fd;
  memfile->prop = fs_getProp(filepath);
  memfile->content = str8(mmap(location, memfile->prop.size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0), memfile->prop.size);

  return memfile;
}

inline fn void fs_sync(File *file, usize newsize) {
  if (!newsize) {
    newsize = file->content.size;
  }

  (void)ftruncate(file->descriptor, newsize);
  (void)msync(file->content.str, newsize, MS_ASYNC | MS_INVALIDATE);
}

inline fn void fs_close(File *file) {
  Assert(file);
  (void)munmap(file->content.str, file->prop.size);
  (void)close(file->descriptor);
}

inline fn bool fs_hasChanged(File *file) {
  FileProperties prop = fs_getProp(file->path);
  return (prop.last_access_time != file->prop.last_access_time) ||
	 (prop.last_modification_time != file->prop.last_modification_time) ||
	 (prop.last_status_change_time != file->prop.last_status_change_time);
}

// =============================================================================
// Misc operation on the filesystem
inline fn bool fs_delete(String8 filepath) {
  Assert(filepath.size != 0);
  return unlink((char *)filepath.str) >= 0;
}

inline fn bool fs_deleteFile(File *f) {
  Assert(f && f->path.size != 0);
  return unlink((char *)f->path.str) >= 0;
}

inline fn bool fs_rename(String8 filepath, String8 to) {
  Assert(filepath.size != 0 && to.size != 0);
  return rename((char *)filepath.str, (char *)to.str) >= 0;
}

inline fn bool fs_renameFile(File *f, String8 to) {
  Assert(f && f->path.size != 0 && to.size != 0);
  return rename((char *)f->path.str, (char *)to.str) >= 0;
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

fn FilenameList fs_iterFiles(Arena *arena, String8 dirname) {
  const String8 currdir = Strlit(".");
  const String8 parentdir = Strlit("..");

  FilenameList res = {0};

  DIR *dir = opendir((char *)dirname.str);
  if (!dir) {
    return res;
  }

  struct dirent *entry;
  while ((entry = readdir(dir))) {
    String8 str = strFromCstr(entry->d_name);
    if (strEq(str, currdir) || strEq(str, parentdir)) {
      continue;
    }

    FilenameNode *node = New(arena, FilenameNode);
    node->value = str;
    DLLPushBack(res.first, res.last, node);
  }

  (void)closedir(dir);
  return res;
}

fn bool fs_rmIter(Arena *temp_arena, String8 dirname) {
  const String8 currdir = Strlit(".");
  const String8 parentdir = Strlit("..");
  usize prev_head = temp_arena->head;

  FilenameList dirstack = {0};
  FilenameList deletable = {0};
  FilenameNode *root = New(temp_arena, FilenameNode);
  root->value = dirname;
  StackPush(dirstack.first, root);

  while (dirstack.first) {
    FilenameNode *current = dirstack.first;
    StackPop(dirstack.first);

    DIR *dir = opendir((char *)current->value.str);
    Assert(dir);

    struct dirent *entry;
    bool is_empty = true;
    while ((entry = readdir(dir))) {
      String8 str = strFromCstr(entry->d_name);
      if (strEq(str, currdir) || strEq(str, parentdir)) {
        continue;
      }

      is_empty = false;
      String8 fullpath = strFormat(temp_arena, "%.*s/%.*s",
                                   Strexpand(current->value), Strexpand(str));

      if (entry->d_type == DT_DIR) {
        FilenameNode *childdir = New(temp_arena, FilenameNode);
        childdir->value = fullpath;
        StackPush(dirstack.first, childdir);
      } else {
        Assert(fs_delete(fullpath));
      }
    }

    (void)closedir(dir);
    if (is_empty) {
      (void)fs_rmdir(current->value);
    } else {
      StackPush(deletable.first, current);
    }
  }

  bool res = true;
  while (deletable.first && res) {
    res = fs_rmdir(deletable.first->value);
    StackPop(deletable.first);
  }

  temp_arena->head = prev_head;
  return res;
}

#endif
