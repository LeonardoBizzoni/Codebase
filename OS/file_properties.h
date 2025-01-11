#ifndef BASE_OS_FILE_PROPERITES
#define BASE_OS_FILE_PROPERITES

#include "../base.h"

typedef enum {
  ACF_Unknown = 0,
  ACF_Execute = 1 << 0,
  ACF_Read = 1 << 2,
  ACF_Write = 1 << 1,
} AccessFlag;

typedef struct {
  u32 ownerID;
  u32 groupID;
  usz size;

  u64 last_access_time;
  u64 last_modification_time;
  u64 last_status_change_time;

  union {
    AccessFlag permissions[3];

    struct {
      AccessFlag user;
      AccessFlag group;
      AccessFlag other;
    };
  };
} FileProperties;

#endif
