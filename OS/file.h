#ifndef BASE_OS_FILE
#define BASE_OS_FILE

typedef u8 os_Permissions;
enum {
  os_Permissions_Unknown = 0,
  os_Permissions_Execute = 1 << 0,
  os_Permissions_Write   = 1 << 1,
  os_Permissions_Read    = 1 << 2,
};

typedef u32 os_AccessFlags;
enum {
  os_acfRead       = 1 << 0,
  os_acfWrite      = 1 << 1,
  os_acfExecute    = 1 << 2,
  os_acfAppend     = 1 << 3,
  os_acfShareRead  = 1 << 4,
  os_acfShareWrite = 1 << 5,
};

typedef struct {
  u32 ownerID;
  u32 groupID;
  usize size;
  
  u64 last_access_time;
  u64 last_modification_time;
  u64 last_status_change_time;
  
  union {
    os_Permissions permissions[3];
    
    struct {
      os_Permissions user;
      os_Permissions group;
      os_Permissions other;
    };
  };
} fs_Properties;

typedef struct {
  u64 fd[1];
} os_Handle;

typedef struct{
  String8 name;
  fs_Properties *properties;
} OS_FileInfo;

typedef struct{
  u8 memory[640];
} OS_FileIter;

typedef struct {
  os_Handle handle;
  String8 path;
  fs_Properties prop;
  u8 *content;
} File;

// =============================================================================
// File reading and writing/appending
       fn os_Handle fs_open(String8 filepath, os_AccessFlags flags);
       fn String8 fs_read(Arena *arena, os_Handle file);
inline fn bool fs_write(os_Handle file, String8 content);

fn fs_Properties fs_getProp(os_Handle file);

// =============================================================================
// Memory mapping files for easier and faster handling

       fn File fs_fopen(String8 filepath);
       fn File fs_fopenTmp(Arena *arena);
inline fn bool fs_fclose(File *file);
inline fn bool fs_fresize(File *file, usize size);
inline fn void fs_fwrite(File *file, String8 str);

inline fn bool fs_fileHasChanged(File *file);
inline fn bool fs_fdelete(File *file);
inline fn bool fs_frename(File *file, String8 to);

// =============================================================================
// Misc operation on the filesystem
inline fn bool fs_delete(String8 filepath);
inline fn bool fs_rename(String8 filepath, String8 to);

inline fn bool fs_mkdir(String8 path);
inline fn bool fs_rmdir(String8 path);

// =============================================================================
// File iteration
typedef struct FilenameNode {
  String8 value;
  struct FilenameNode *next;
  struct FilenameNode *prev;
} FilenameNode;

typedef struct {
  FilenameNode *first;
  FilenameNode *last;
} FilenameList;

fn FilenameList fs_iterFiles(Arena *arena, String8 dirname);
fn bool         fs_rmIter(String8 dirname);

fn OS_FileIter* os_file_iter_begin(Arena *arena, String8 path);
fn bool         os_file_iter_next(Arena *arena, OS_FileIter *iter, OS_FileInfo *info_out);
fn void         os_file_iter_end(OS_FileIter *iter);

#endif
