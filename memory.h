#pragma once

#include "base.h"
#include "arena.h"

void *memcpy(Arena *arena, void *dest, void *src, size_t size);