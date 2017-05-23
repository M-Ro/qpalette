#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defs.h"
#include "colormap.h"

#pragma pack(push, 1)
struct bmp_header_t
{
	unsigned short header_field;
	unsigned int size;
	unsigned char reserved1[2];
	unsigned char reserved2[2];
	unsigned int data_offset;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct bmp_dib_header_t
{
	unsigned int dib_length;
	int width;
	int height;
	unsigned short planes;
	unsigned short bpp;
	unsigned int compression;
	unsigned int image_size;
	int res_h;
	int res_v;
	unsigned int palette_colors;
	unsigned int imp_colors;
};
#pragma pack(pop)

struct bmp_header_t *read_bmp_header(FILE *file)
{
	struct bmp_header_t *header = malloc(sizeof(struct bmp_header_t));
	fread(header, sizeof(struct bmp_header_t), 1, file);

	/* Validate BMP header */
	if(header->header_field != 0x4D42)
	{
		printf("Error: Invalid BMP header field\n");
		free(header);
		return NULL;
	}

	return header;
}

struct bmp_dib_header_t *read_bmp_dib_header(FILE *file)
{
	struct bmp_dib_header_t *dib_header = malloc(sizeof(struct bmp_dib_header_t));
	fread(dib_header, sizeof(struct bmp_dib_header_t), 1, file);

	/* Do basic sanity checks */
	if(dib_header->width > 8192 || dib_header->height > 8192)
	{
		printf("Error: BMP has invalid size dimensions.\n");
		free(dib_header);
		return NULL;
	}

	if(dib_header->planes != 1)
	{
		printf("Error: BMP dib_header->planes != 0.\n");
		free(dib_header);
		return NULL;
	}

	if(dib_header->bpp <= 8)
	{
		printf("Error: BMP is already 8bpp format.\n");
		free(dib_header);
		return NULL;
	}

	if(dib_header->bpp != 24)
	{
		printf("Error: Currently only 24bit RGB images are supported.\n");
		free(dib_header);
		return NULL;
	}

	if(dib_header->compression != 0)
	{
		printf("Error: This reader does not support compressed BMP files.\n");
		free(dib_header);
		return NULL;
	}

	return dib_header;
}

struct image_t *load_bmp(const char *path)
{
	FILE *f = fopen(path, "rb");
	if(f == NULL)
		return NULL;

	struct bmp_header_t *header = read_bmp_header(f);
	if(header == NULL)
	{
		fclose(f);
		return NULL;
	}

	struct bmp_dib_header_t *dib_header = read_bmp_dib_header(f);
	if(dib_header == NULL)
	{
		fclose(f);
		return NULL;
	}

	/* Begin reading actual data */
	unsigned char *image_data = malloc(dib_header->width * dib_header->height * dib_header->bpp / 8);
	if(!image_data)
	{
		printf("Failed to malloc image_data\n");
		fclose(f);
		return NULL;
	}

	fseek(f, header->data_offset, SEEK_SET);
	
	/* Read BMP data row-by-row, taking row padding into consideration */
	unsigned int padcount = 0;
	if((dib_header->width * dib_header->bpp / 8) % 4 != 0)
		padcount = 4 - ((dib_header->width * dib_header->bpp / 8) % 4);

	for(int y=0; y<dib_header->height; y++)
	{
		unsigned int index = y * dib_header->width * dib_header->bpp / 8;
		fread(image_data+index, dib_header->width * dib_header->bpp / 8, 1, f);
		fseek(f, padcount, SEEK_CUR);
	}

	/* .BMP files are BGR, Iterate through data and swap BGR data to RGB */
	for(unsigned int i=0; i<dib_header->width*dib_header->height*dib_header->bpp / 8; i+=3) // FIXME
	{
		unsigned char stor = image_data[i];
		image_data[i] = image_data[i+2];
		image_data[i+2] = stor;
	}

	/* Setup return structures */
	struct image_t *img = malloc(sizeof(struct image_t));

	if(!img)
		printf("Failed to malloc image_t\n");

	img->info = malloc(sizeof(struct img_info_t));
	img->info->bpp = dib_header->bpp;
	img->info->channels = 3;
	img->info->width = dib_header->width;
	img->info->height = dib_header->height;
	img->data = image_data;

	/* Cleanup and return loaded image */
	free(header);
	free(dib_header);
	fclose(f);

	return img;
}

int write_bmp(struct image_t *image, const char *path)
{
	/* Prepare BMP header */
	struct bmp_header_t header;
	header.header_field = 0x4D42;
	header.size = sizeof(struct bmp_header_t) + sizeof(struct bmp_dib_header_t) + 256*4 + image->info->width * image->info->height * image->info->bpp / 8;
	memset(&header+6, 0, 4); // Set reserved bytes to 0
	header.data_offset = sizeof(struct bmp_header_t) + sizeof(struct bmp_dib_header_t) + 256*4; // 256*4 = colormap len * 4

	/* Prepare BMP DIB header */
	struct bmp_dib_header_t dib_header;
	dib_header.dib_length = 40;
	dib_header.width = image->info->width;
	dib_header.height = image->info->height;
	dib_header.planes = 1;
	dib_header.bpp = image->info->bpp;
	dib_header.compression = 0;
	dib_header.image_size = 0;
	dib_header.res_h = 0;
	dib_header.res_v = 0;
	dib_header.palette_colors = 0; // 0 = 2^n
	dib_header.imp_colors = 0;

	/* NOTE: Should swap image data RGB to BGR if 24bit here - but since we only ever output paletted images, don't bother */

	/* Open file for writing */
	FILE *f = fopen(path, "wb");
	if(!f)
	{
		printf("Failed to open %s for writing\n", path);
		return 0;
	}

	/* Write headers */
	fwrite(&header, sizeof(struct bmp_header_t), 1, f);
	fwrite(&dib_header, sizeof(struct bmp_dib_header_t), 1, f);

	/* Write colormap, BMP requires 4 byte R, G, B, 0x00 */
	for(int i=0; i<256; i++)
	{
		/* memcpy would be faster here, but we have to convert RGB to BGR */
		unsigned char color[4];
		color[0] = cmap[i*3+2];
		color[1] = cmap[i*3+1];
		color[2] = cmap[i*3+0];
		color[3] = 0x00;

		fwrite(&color, 4, 1, f);
	}

	// Write rows to file. Making sure to pad row length to 4 bytes
	unsigned int padcount = 0;
	if(image->info->width % 4 != 0)
		padcount = 4 - (image->info->width % 4);

	for(int y=0; y<image->info->height; y++)
	{
		unsigned int index = y * image->info->width * image->info->bpp / 8;
		fwrite(image->data+index, image->info->width * image->info->bpp / 8, 1, f);
		for(int p=0; p<padcount; p++)
			putc(0, f);
	}

	fclose(f);
	return 1;
}