#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>

#include "bmp.h"
#include "png.h"
#include "colormap.h"

struct cli_options_t
{
	unsigned int allow_fullbrights; // set by -b
	unsigned char *output_dest; 	// set by -o
	unsigned int output_type;		// set by -t
	unsigned int input_type;		// discovered from file ext
	unsigned char *file_src;		// <path to src image>
};

struct cli_options_t arguments;

void print_usage(char *argv0)
{
	printf("\n-- Usage --\n");
	printf("%s [options] <path to image>\n", argv0);
	printf("\n-- Options --\n");
	printf("-b  -  Allow use of fullbright colors from Quake 1 colormap\n");
	printf("-o   -  Output file name, e.g -o out.png - default is input_conv.ext\n");
	printf("-t   -  Output file type, Valid values are bmp, png - default is input filetype\n");
}

int parse_typearg(char *arg)
{
	if(!arg)
		return -1;
	if(strlen(arg) != 3)
	{
		printf("Invalid file format: %s\n", arg);
		return -1;
	}

	if(!strcmp(arg, "bmp"))
		return 0;
	else if(!strcmp(arg, "png"))
		return 1;
	else
	{
		printf("Invalid file format: %s\n", arg);
		return -1;
	}
}

int parse_options(int argc, char **argv)
{
	if(argc < 2)
	{
		print_usage(argv[0]);
		return 0;
	}

	int tflag, oflag = 0;

	extern char *optarg;
	extern int optind;
	int c, err = 0;

	while ((c = getopt (argc, argv, "bho:t:")) != -1)
	{
		switch (c)
		{
			case 'b': arguments.allow_fullbrights = 1; break;
			case 'h': print_usage(argv[0]); return 0;
			case 't': tflag = 1; if(parse_typearg(optarg) < 0) return 0; arguments.output_type = parse_typearg(optarg); break;
			case 'o': arguments.output_dest = optarg; oflag = 1; break;
			case '?':
				if (optopt == 'o' || optopt == 't')
				  fprintf (stderr, "Option -%c requires an argument.\n", optopt);
				else
				  fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);
				return 0;
			default: break;
		}
	}

	// get file_src
	if ((optind + 1) > argc) 
	{	
		fprintf(stderr, "%s: missing source file argument\n", argv[0]);
		print_usage(argv[0]);
		return 0;
	}

	arguments.file_src = argv[optind];

	/* If Type/Dest not specified, set them here */

	/* Check input type */
	if(!arguments.file_src || strlen(arguments.file_src) < 4)
			return 0;

	char ext[3];
	memcpy(ext, arguments.file_src+strlen(arguments.file_src)-3, 3);

	arguments.input_type = parse_typearg(ext);
	if(arguments.input_type < 0)
		return 0;

	if(!tflag) // If -t wasn't specified, set output type to src filetype
		arguments.output_type = arguments.input_type;

	if(!oflag) // If -o wasn't specified, set output filename to filename_conv.ext
	{
		if(!arguments.file_src || strlen(arguments.file_src) < 4)
			return 0;

		unsigned int len = strlen(arguments.file_src) + 5;
		char *dest = malloc(len);
		memcpy(dest, arguments.file_src, strlen(arguments.file_src)-4);
		strcat(dest, "_conv");
		if(arguments.output_type == 0)
			strcat(dest, ".bmp");
		else if(arguments.output_type == 1)
			strcat(dest, ".png");

		arguments.output_dest = dest;
	}

	return 1;
}

struct image_t *to_palette_rgb(struct image_t *src, int allow_fullbrights);

int main(int argc, char **argv)
{
	/* Argument handling */
	if(!parse_options(argc, argv))
		return 1;

	/* Load input file */
	struct image_t *img_src = NULL;

	if(arguments.input_type == 0)
		img_src = load_bmp(arguments.file_src);
	else if(arguments.input_type == 1)
		img_src = load_png(arguments.file_src);

	if(img_src == NULL)
	{
		printf("Error: Failed to load image, exiting\n");
		return 1;
	}

	/* Print image stats */
	printf("Loaded image %s: %dx%dx%d\n", arguments.file_src, img_src->info->width, img_src->info->height, img_src->info->bpp);

	/* Convert to indexed palette */
	struct image_t *img_dst = to_palette_rgb(img_src, arguments.allow_fullbrights);

	/* Output converted file */
	unsigned int ret = -1;
	if(arguments.output_type == 0)
		ret = write_bmp(img_dst, arguments.output_dest);
	else if(arguments.output_type == 1)
		ret = write_png(img_dst, arguments.output_dest);

	if(ret)
		printf("Converted file: %s\n", arguments.output_dest);
	else
		printf("Error: Failed to convert file.\n");

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