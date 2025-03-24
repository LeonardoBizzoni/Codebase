#ifndef BASE_INC_H
#define BASE_INC_H

#include "base.h"
#include "list.h"
#include "memory.h"
#include "arena.h"
#include "thread_ctx.h"
#include "time.h"
#include "string.h"

#if CPP
#  include "array.hpp"
#  include "hashmap.hpp"
#endif

// TODO(lb): find a place for this
#if !OS_NONE
#  include "../image.h"
#endif

#endif
