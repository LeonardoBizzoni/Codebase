#ifndef BASE_IMAGE_H
#define BASE_IMAGE_H

/* TODO: handmade image loading maybe? */
#define STB_IMAGE_IMPLEMENTATION
#include "extern/stb_image.h"
/* Failes to include `<string.h>` */

inline fn u8 *loadImg(String8 path, i32 *width, i32 *height, i32 *componentXpixel) {
  return stbi_load((char *)path.str, width, height, componentXpixel, 0);
  /* return 0; */
}

inline fn void destroyImg(u8 *imgdata) {
  stbi_image_free(imgdata);
}

#endif
