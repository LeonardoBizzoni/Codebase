#ifndef BASE_IMAGE_H
#define BASE_IMAGE_H

#include "base.h"
#include "string.h"

/* TODO: handmade image loading maybe? */
#define STB_IMAGE_IMPLEMENTATION
#include "extern/stb_image.h"

inline u8 *loadImg(String8 path, i32 *width, i32 *height, i32 *componentXpixel) {
  return stbi_load((char *)path.str, width, height, componentXpixel, 0);
}

inline void destroyImg(u8 *imgdata) {
  stbi_image_free(imgdata);
}

#endif
