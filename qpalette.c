#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>

#include "bmp.h"
#include "colormap.h"

void print_usage(char *argv0)
{
	printf("\n-- qpalette Usage --\n\n");
	printf("%s <path to image>\n", argv0);
}

struct image_t *to_palette_rgb(struct image_t *src, int allow_fullbrights);

int main(int argc, char **argv)
{
	/* Argument handling */
	if(argc < 2)
	{
		print_usage(argv[0]);
		return 1;
	}

	struct image_t *img_src = load_bmp(argv[1]);
	if(img_src == NULL)
	{
		printf("Error: Failed to load image, exiting\n");
		return 1;
	}

	int allow_fullbrights = 0;

	for(int i=0; i<argc; i++)
	{
		if(strcmp(argv[i], "-fb") == 0)
		{
			printf("Fullbrights enabled\n");
			allow_fullbrights = 1;
		}
	}

	/* Print image stats */
	printf("Loaded image %s: %dx%dx%d\n", argv[1], img_src->info->width, img_src->info->height, img_src->info->bpp);

	/* Convert to indexed palette */
	struct image_t *img_dst = to_palette_rgb(img_src, allow_fullbrights);

	/* Write output BMP */
	//char *destpath = malloc(strlen(argv[1])+5);
	//strcpy(destpath, argv[1]);

	write_bmp(img_dst, "output.bmp");

	/* Cleanup and exit */
	free(img_src->info);
	free(img_src);
	free(img_dst->info);
	free(img_dst);

	return 0;
}

/* Simple RGB comparison */
struct image_t *to_palette_rgb(struct image_t *src, int allow_fullbrights)
{
	unsigned char *dst = malloc(src->info->width * src->info->height);

	unsigned int avail_colors = cmap_colors;
	if(allow_fullbrights < 1)
		avail_colors -= 32;
	
	for(unsigned int i=0; i<src->info->width * src->info->height; i++)
	{
		unsigned char r = src->data[i*3];
		unsigned char g = src->data[i*3+1];
		unsigned char b = src->data[i*3+2];

		/* Find closest match in colormap */
		unsigned int delta = UINT_MAX;
		unsigned int index = 0;

		for(unsigned int j=0; j<avail_colors; j++)
		{
			unsigned char cr = cmap[j*3];
			unsigned char cg = cmap[j*3+1];
			unsigned char cb = cmap[j*3+2];

			unsigned int d2 = abs(cr-r) + abs(cg-g) + abs(cb-b);
			if(d2 < delta)
			{
				delta = d2;
				index = j;
			}
		}

		dst[i] = index;
	}

	/* Create return structs */
	struct image_t *img_dst = malloc(sizeof(struct image_t));
	img_dst->info = malloc(sizeof(struct img_info_t));

	img_dst->info->bpp = 8;
	img_dst->info->channels = 1;
	img_dst->info->width = src->info->width;
	img_dst->info->height = src->info->height;
	img_dst->data = dst;

	return img_dst;
}