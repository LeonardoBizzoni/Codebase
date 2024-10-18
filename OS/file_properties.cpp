#ifndef BASE_OS_FILE_PROPERITES
#define BASE_OS_FILE_PROPERITES

#include "../base.cpp"

namespace Base::OS {
enum AccessFlag {
  Unknown = 0,
  Execute = 1 << 0,
  Read = 1 << 2,
  Write = 1 << 1,
};

struct FileProperties {
  u32 ownerID;
  u32 groupID;
  AccessFlag permissions;
  size_t byte_size;

  u64 last_access_time;
  u64 last_modification_time;
  u64 last_status_change_time;
};
}

#endif
