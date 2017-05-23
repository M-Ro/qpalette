#pragma once

struct img_info_t
{
	unsigned int bpp;
	unsigned int channels;
	unsigned int width;
	unsigned int height;
};

struct image_t
{
	struct img_info_t *info;
	unsigned char *data;
};
