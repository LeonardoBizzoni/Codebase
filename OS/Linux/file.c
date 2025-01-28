#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <dirent.h>

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
  }

  OS_Handle res = {(u64)fd};
  return res;
}

fn String8 fs_read(Arena *arena, OS_Handle file) {
  int fd = file.h[0];
  String8 result = {0};
  if(!fd) { return result; }

  struct stat file_stat;
  if (!fstat(fd, &file_stat)) {
    u8 *buffer = New(arena, u8, file_stat.st_size);
    if(pread(fd, buffer, file_stat.st_size, 0) >= 0) {
      result.str = buffer;
      result.size = file_stat.st_size;
    }
  }

  return result;
}

inline fn bool fs_write(OS_Handle file, String8 content) {
  if(!file.h[0]) { return false; }
  return write(file.h[0], content.str, content.size) == (isize)content.size;
}

fn FS_Properties fs_getProp(OS_Handle file) {
  FS_Properties result = {0};
  if(!file.h[0]) { return result; }

  struct stat file_stat;
  if (fstat((i32)file.h[0], &file_stat) < 0) {
    return result;
  }

  result.ownerID = file_stat.st_uid;
  result.groupID = file_stat.st_gid;
  result.size = (usize)file_stat.st_size;
  result.last_access_time = (u64)file_stat.st_atime;
  result.last_modification_time = (u64)file_stat.st_mtime;
  result.last_status_change_time = (u64)file_stat.st_ctime;

  result.user = OS_Permissions_Unknown;
  if (file_stat.st_mode & S_IRUSR) { result.user |= OS_Permissions_Read; }
  if (file_stat.st_mode & S_IWUSR) { result.user |= OS_Permissions_Write; }
  if (file_stat.st_mode & S_IXUSR) { result.user |= OS_Permissions_Execute; }

  result.group = OS_Permissions_Unknown;
  if (file_stat.st_mode & S_IRGRP) { result.group |= OS_Permissions_Read; }
  if (file_stat.st_mode & S_IWGRP) { result.group |= OS_Permissions_Write; }
  if (file_stat.st_mode & S_IXGRP) { result.group |= OS_Permissions_Execute; }

  result.other = OS_Permissions_Unknown;
  if (file_stat.st_mode & S_IROTH) { result.other |= OS_Permissions_Read; }
  if (file_stat.st_mode & S_IWOTH) { result.other |= OS_Permissions_Write; }
  if (file_stat.st_mode & S_IXOTH) { result.other |= OS_Permissions_Execute; }

  return result;
}

// =============================================================================
// Memory mapping files for easier and faster handling

fn File fs_fopen(String8 filepath) {
  File file = {0};
  i32 fd = open((char *)filepath.str, O_RDWR | O_CREAT,
                S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

  if (fd >= 0) {
    file.handle.h[0] = fd;
    file.path = filepath;
    file.prop = fs_getProp(file.handle);
    file.content = (u8 *)mmap(0, ClampBot(file.prop.size, 1), PROT_READ | PROT_WRITE,
				MAP_SHARED, fd, 0);
  }

  return file;
}

fn File fs_fopenTmp(Arena *arena) {
  char path[] = "/tmp/base-XXXXXX";
  i32 fd = mkstemp(path);

  String8 pathstr = {
    .str = New(arena, u8, Arrsize(path)),
    .size = Arrsize(path),
  };
  memCopy(pathstr.str, path, Arrsize(path));

  File file = {0};
  file.handle.h[0] = fd;
  file.path = pathstr;
  file.prop = fs_getProp(file.handle);
  file.content = (u8*)mmap(0, ClampBot(file.prop.size, 1), PROT_READ | PROT_WRITE,
			   MAP_SHARED, fd, 0);
  return file;
}

inline fn bool fs_fclose(File *file) {
  return close(file->handle.h[0]) >= 0;
}

inline fn bool fs_fresize(File *file, usize size) {
  if (ftruncate(file->handle.h[0], size) < 0) {
    return false;
  }

  (void)munmap(file->content, file->prop.size);
  return (bool)(file->content = (u8*)mmap(0, size, PROT_READ | PROT_WRITE,
					  MAP_SHARED, file->handle.h[0], 0));
}

inline fn void fs_fwrite(File *file, String8 content) {
  if (fs_fresize(file, content.size)) { file->prop.size = content.size; }
  memZero(file->content + content.size, ClampBot(0, (isize)file->prop.size - (isize)content.size));
  (void)memCopy(file->content, content.str, content.size);
}

inline fn bool fs_fileHasChanged(File *file) {
  FS_Properties prop = fs_getProp(file->handle);
  return (file->prop.last_access_time != prop.last_access_time) ||
	 (file->prop.last_modification_time != prop.last_modification_time) ||
	 (file->prop.last_status_change_time != prop.last_status_change_time);
}

inline fn bool fs_fdelete(File *file) {
  return unlink((char *) file->path.str) >= 0 && fs_fclose(file);
}

inline fn bool fs_frename(File *file, String8 to) {
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

inline fn bool fs_mkdir(String8 path) {
  Assert(path.size != 0);
  return mkdir((char *)path.str,
               S_IRWXU | (S_IRGRP | S_IXGRP) | (S_IROTH | S_IXOTH)) >= 0;
}

inline fn bool fs_rmdir(String8 path) {
  Assert(path.size != 0);
  return rmdir((char *)path.str) >= 0;
}

fn FilenameList fs_iterFiles(Arena *arena, String8 dirname) {
  local const String8 currdir = StrlitInit(".");
  local const String8 parentdir = StrlitInit("..");

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

fn bool fs_rmIter(String8 dirname) {
  local const String8 currdir = StrlitInit(".");
  local const String8 parentdir = StrlitInit("..");

  Scratch scratch = ScratchBegin(0, 0);
  FilenameList dirstack = {0};
  FilenameList deletable = {0};
  FilenameNode *root = New(scratch.arena, FilenameNode);
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
      String8 fullpath = strFormat(scratch.arena, "%.*s/%.*s",
                                   Strexpand(current->value), Strexpand(str));

      if (entry->d_type == DT_DIR) {
        FilenameNode *childdir = New(scratch.arena, FilenameNode);
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

  ScratchEnd(scratch);
  return res;
}
