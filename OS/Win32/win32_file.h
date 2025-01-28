#ifndef WIN32_FILE_H
#define WIN32_FILE_H

typedef struct OS_W32_FileIter OS_W32_FileIter;
struct OS_W32_FileIter
{
  HANDLE handle;
  WIN32_FIND_DATAW file_data;
  bool done;
};

StaticAssert(sizeof(OS_W32_FileIter) <= sizeof(OS_FileIter), file_iter_size_check);

#endif //WIN32_FILE_H
