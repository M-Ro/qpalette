#pragma once

#include "defs.h"

extern struct image_t *load_bmp(const char *path);

extern int write_bmp(struct image_t *image, const char *path);