#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <png.h>

#include "defs.h"
#include "colormap.h"

struct image_t *load_png(const char *path)
{
	FILE *f = fopen(path, "rb");
	if(!f)
		return NULL;

	png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if(png == NULL)
	{
		printf("Failed to create PNG read struct\n");
		fclose(f);
		return NULL;
	}

	png_infop info = png_create_info_struct(png);
	if(info == NULL)
	{
		printf("Failed to create PNG info struct\n");
		fclose(f);
		return NULL;
	}

	if(setjmp(png_jmpbuf(png))) 
	{
		printf("Failed to set PNG jmp\n");
		fclose(f);
		return NULL;
	}

	png_init_io(png, f);
	png_read_info(png, info);

	struct img_info_t *img_info = malloc(sizeof(struct img_info_t));
	img_info->width      = png_get_image_width(png, info);
	img_info->height     = png_get_image_height(png, info);
	img_info->channels	 = 3;
	img_info->bpp		 = 24;

	png_byte color_type = png_get_color_type(png, info);
	png_byte bit_depth  = png_get_bit_depth(png, info);

	if(bit_depth == 16)	// Strip 16 bpc images down to 8bpc
		png_set_strip_16(png);

	if(color_type == PNG_COLOR_TYPE_PALETTE) // Convert indexed images to RGB
		png_set_palette_to_rgb(png);

	if(color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) // Expand narrow greyscale to 8bpc
		png_set_expand_gray_1_2_4_to_8(png);

	if(png_get_valid(png, info, PNG_INFO_tRNS))
		png_set_tRNS_to_alpha(png);

	/* Convert all non-RGBA images to RGBA for easier reading later on */
	if(color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_filler(png, 0xFF, PNG_FILLER_AFTER);

	if(color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) // Greyscale to RGB
		png_set_gray_to_rgb(png);

	png_read_update_info(png, info);

	/* Start actually reading the image */
	png_bytep *row_pointers = malloc(sizeof(png_bytep) * img_info->height);
	for(int y = 0; y < img_info->height; y++)
    	row_pointers[y] = malloc(png_get_rowbytes(png, info));

    png_read_image(png, row_pointers);

    /* Create return structs and convert the img into the RGB format we need */
    struct image_t *img = malloc(sizeof(struct image_t));
    img->info = img_info;
    img->data = malloc(img_info->height * img_info->width * img_info->bpp/8);

    for(int y=0; y<img_info->height; y++)
    {
    	unsigned int siy = (img_info->height-1-y) * img_info->width * img_info->bpp/8;
    	unsigned int sx = 0;
    	for(int dx=0; dx<img_info->width * 4; dx+=4, sx+=3)
    		memcpy(img->data+siy+sx, row_pointers[y]+dx, 3);
    }

    for(int y = 0; y < img_info->height; y++)
    	free(row_pointers[y]);
	free(row_pointers);

	fclose(f);

	return img;
}

int write_png(struct image_t *image, const char *path)
{
	FILE *f = fopen(path, "wb");
	if(!f)
		return 0;

	/* Initialize and configure libPNG */

	png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if(png == NULL)
	{
		printf("Failed to create PNG write struct\n");
		fclose(f);
		return 0;
	}

	png_infop info = png_create_info_struct(png);
	if(info == NULL)
	{
		printf("Failed to create PNG info struct\n");
		fclose(f);
		return 0;
	}

	if(setjmp(png_jmpbuf(png))) 
	{
		printf("Failed to set PNG jmp\n");
		fclose(f);
		return 0;
	}

	png_init_io(png, f);

	png_set_IHDR(png, info, image->info->width, image->info->height, 8, 
		PNG_COLOR_TYPE_PALETTE, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

	/* Convert colormap to PNG palette */
	png_color* palette = png_malloc(png, cmap_colors*sizeof(png_color));

	for (unsigned int i=0; i<cmap_colors; i++)
	{
		png_color* col = &palette[i];
		col->red = cmap[i*3];
		col->green = cmap[i*3+1];
		col->blue = cmap[i*3+2];
	}

	png_set_PLTE(png, info, palette, cmap_colors);

	/* Write PNG Header Data */
	png_write_info(png, info);

	/* Start writing image data */
	png_bytep row = malloc(image->info->width * sizeof(png_byte));

	for(unsigned int y=0; y<image->info->height; y++)
	{
		unsigned int yindex = (image->info->height-1-y) * image->info->width;
		for(int x=0; x<image->info->width; x++)
			row[x] = image->data[yindex + x];

		png_write_row(png, row);
	}

	png_write_end(png, NULL);


	/* Cleanup */
	free(row);
	png_destroy_write_struct(&png, &info);
	png_free(png, palette);
	fclose(f);
	return 1;
}