#pragma once

#include "defs.h"

extern struct image_t *load_png(const char *path);

extern int write_png(struct image_t *image, const char *path);