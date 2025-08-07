#ifndef BASE_INC_H
#define BASE_INC_H

#include "base.h"
#include "list.h"
#include "arena.h"
#include "thread_ctx.h"
#include "time.h"
#include "string.h"
#include "vector.h"

#if CPP
#  include "array.hpp"
#  include "hashmap.hpp"
#endif

// TODO(lb): find a place for this
#ifndef PLATFORM_CODERBOT
#  include "../image.h"
#endif

#endif
