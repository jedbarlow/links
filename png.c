/* png.c
 * PNG decoding
 * (c) 2002 Karel 'Clock' Kulhavy
 * This is a part of the Links program, released under GPL.
 */
#include "cfg.h"

#ifdef G
#include "links.h"

#ifdef HAVE_ENDIAN_H
#include <endian.h>
#endif /* #ifdef HAVE_ENDIAN_H */

#ifdef REPACK_16
#undef REPACK_16
#endif /* #ifdef REPACK_16 */

#if SIZEOF_UNSIGNED_SHORT != 2
#define REPACK_16
#endif /* #if SIZEOF_UNSIGNED_SHORT != 2 */

#ifndef REPACK_16
#ifndef C_LITTLE_ENDIAN
#ifndef C_BIG_ENDIAN
#define REPACK_16
#endif /* #ifndef C_BIG_ENDIAN */
#endif /* #ifndef C_LITTLE_ENDIAN */
#endif /* #ifndef REPACK_16 */
	  
/* Decoder structs */

/* Warning for from-web PNG images */
static void img_my_png_warning(png_structp a, png_const_charp b)
{
}

/* Error for from-web PNG images. */
static void img_my_png_error(png_structp png_ptr, png_const_charp error_string)
{
#if (PNG_LIBPNG_VER < 10500)
	longjmp(png_ptr->jmpbuf,1);
#else
	png_longjmp(png_ptr,1);
#endif
}

static void png_info_callback(png_structp png_ptr, png_infop info_ptr)
{
	int bit_depth, color_type, intent;
	double gamma;
	unsigned char bytes_per_pixel=3;
	struct cached_image *cimg;

	cimg=global_cimg;

	bit_depth=png_get_bit_depth(png_ptr, info_ptr);
	color_type=png_get_color_type(png_ptr, info_ptr);
	if (color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_expand(png_ptr);
    	if (color_type == PNG_COLOR_TYPE_GRAY &&
		bit_depth < 8) png_set_expand(png_ptr);
	if (png_get_valid(png_ptr, info_ptr,
		PNG_INFO_tRNS)){
		png_set_expand(png_ptr); /* Legacy version of
		png_set_tRNS_to_alpha(png_ptr); */
		bytes_per_pixel++;
	}
	if (color_type == PNG_COLOR_TYPE_GRAY ||
		color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb(png_ptr);
	if (bit_depth==16){
#ifndef REPACK_16
#ifdef C_LITTLE_ENDIAN
		/* We use native endianity only if unsigned short is 2-byte
		 * because otherwise we have to reassemble the buffer so we
		 * will leave in the libpng-native big endian.
		 */
		png_set_swap(png_ptr);
#endif /* #ifdef C_LITTLE_ENDIAN */
#endif /* #ifndef REPACK_16 */
		bytes_per_pixel*=(int)sizeof(unsigned short);
	}
	png_set_interlace_handling(png_ptr);
	if (color_type==PNG_COLOR_TYPE_RGB_ALPHA
		||color_type==PNG_COLOR_TYPE_GRAY_ALPHA){
		if (bytes_per_pixel==3
			||bytes_per_pixel==3*sizeof(unsigned short))
			bytes_per_pixel=4*bytes_per_pixel/3;
	}
	cimg->width=(int)png_get_image_width(png_ptr,info_ptr);
	cimg->height=(int)png_get_image_height(png_ptr,info_ptr);
	cimg->buffer_bytes_per_pixel=bytes_per_pixel;
	if (png_get_sRGB(png_ptr, info_ptr, &intent)){
		gamma=sRGB_gamma;
	}
	else
 	{
  		if (!png_get_gAMA(png_ptr, info_ptr, &gamma)){
			gamma=sRGB_gamma;
		}
	}
	if (gamma < 0.01 || gamma > 100)
		gamma = sRGB_gamma;
	cimg->red_gamma=(float)gamma;
	cimg->green_gamma=(float)gamma;
	cimg->blue_gamma=(float)gamma;
	png_read_update_info(png_ptr,info_ptr);
	cimg->strip_optimized=0;
	if (header_dimensions_known(cimg))
		img_my_png_error(png_ptr, "bad image size");
}

#ifdef REPACK_16
/* Converts unsigned shorts to doublechars (in big endian) */
static void a2char_from_unsigned_short(unsigned char *chr, unsigned short *shrt, int len)
{
	unsigned short s;

	for (;len;len--,shrt++,chr+=2){
		s=*shrt;
		*chr=s>>8;
		chr[1]=s;
	}
}

/* Converts doublechars (in big endian) to unsigned shorts */
static void unsigned_short_from_2char(unsigned short *shrt, unsigned char *chr, int len)
{
	unsigned short s;
	
	for (;len;len--,shrt++,chr+=2){
		s=((*chr)<<8)|chr[1];
		*shrt=s;
	}
}
#endif

static void png_row_callback(png_structp png_ptr, png_bytep new_row, png_uint_32
	row_num, int pass)
{
	struct cached_image *cimg;
#ifdef REPACK_16
	unsigned char *tmp;
	int channels;
#endif /* #ifdef REPACK_16 */

	cimg=global_cimg;
#ifdef REPACK_16
	if (cimg->buffer_bytes_per_pixel>4)
	{
		channels=cimg->buffer_bytes_per_pixel/sizeof(unsigned
			short);
		if (PNG_INTERLACE_NONE==png_get_interlace_type(png_ptr,
			((struct png_decoder *)cimg->decoder)->info_ptr))
		{
			unsigned_short_from_2char((unsigned short *)(cimg->buffer+cimg
				->buffer_bytes_per_pixel *cimg->width
				*row_num), new_row, cimg->width
				*channels);
		}else{
			if ((unsigned)cimg->width > (unsigned)MAXINT / 2 / channels) overalloc();
			tmp=mem_alloc(cimg->width*2*channels);
			a2char_from_unsigned_short(tmp, (unsigned short *)(cimg->buffer
				+cimg->buffer_bytes_per_pixel
				*cimg->width*row_num), cimg->width*channels);
			png_progressive_combine_row(png_ptr, tmp, new_row);
			unsigned_short_from_2char((unsigned short *)(cimg->buffer
				+cimg->buffer_bytes_per_pixel
				*cimg->width*row_num), tmp, cimg->width*channels);
			mem_free(tmp);
		}
	}else
#endif /* #ifdef REPACK_16 */
	{
		png_progressive_combine_row(png_ptr,
			cimg->buffer+cimg->buffer_bytes_per_pixel
			*cimg->width*row_num, new_row);
	}
	cimg->rows_added=1;
}

static void png_end_callback(png_structp png_ptr, png_infop info)
{
	end_callback_hit=1;
}

/* Decoder structs */

void png_start(struct cached_image *cimg)
{
	png_structp png_ptr;
	png_infop info_ptr;
	struct png_decoder *decoder;

	retry1:
#ifdef PNG_USER_MEM_SUPPORTED
	png_ptr=png_create_read_struct_2(PNG_LIBPNG_VER_STRING,
			NULL, img_my_png_error, img_my_png_warning,
			NULL, my_png_alloc, my_png_free);
#else
	png_ptr=png_create_read_struct(PNG_LIBPNG_VER_STRING,
			NULL, img_my_png_error, img_my_png_warning);
#endif
	if (!png_ptr) {
		if (out_of_memory(0, NULL, 0)) goto retry1;
		fatal_exit("png_create_read_struct failed");
	}
	retry2:
	info_ptr=png_create_info_struct(png_ptr);
	if (!info_ptr) {
		if (out_of_memory(0, NULL, 0)) goto retry2;
		fatal_exit("png_create_info_struct failed");
	}
	if (setjmp(png_jmpbuf(png_ptr))){
error:
		png_destroy_read_struct(&png_ptr, &info_ptr,
			(png_infopp)NULL);
		img_end(cimg);
		return;
	}
	png_set_progressive_read_fn(png_ptr, NULL,
				    png_info_callback, &png_row_callback,
				    png_end_callback);
   	if (setjmp(png_jmpbuf(png_ptr))) goto error;
	decoder=mem_alloc(sizeof(*decoder));
	decoder->png_ptr=png_ptr;
	decoder->info_ptr=info_ptr;
	cimg->decoder=decoder;
}

void png_restart(struct cached_image *cimg, unsigned char *data, int length)
{
	png_structp png_ptr;
	png_infop info_ptr;

#ifdef DEBUG
	if (!cimg->decoder)
		internal("decoder NULL in png_restart\n");
#endif /* #ifdef DEBUG */
	png_ptr=((struct png_decoder *)(cimg->decoder))->png_ptr;
	info_ptr=((struct png_decoder *)(cimg->decoder))->info_ptr;
	end_callback_hit=0;
	if (setjmp(png_jmpbuf(png_ptr))){
		img_end(cimg);
		return;
	}
	png_process_data(png_ptr, info_ptr, data, length);
	if (end_callback_hit) img_end(cimg);
}

void add_png_version(unsigned char **s, int *l)
{
	add_to_str(s, l, cast_uchar "PNG (");
#ifdef HAVE_PNG_GET_LIBPNG_VER
	add_to_str(s, l, cast_uchar png_get_libpng_ver(NULL));
#else
	add_to_str(s, l, cast_uchar PNG_LIBPNG_VER_STRING);
#endif
	add_to_str(s, l, cast_uchar ")");
}

#endif /* #ifdef G */
