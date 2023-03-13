#include <png.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "alttpcolorfix.h"

int main(int argc, char* argv[])
{
    if (check_arguments(argc) != 0) {
        return 1;
    }

    FILE* png_file = fopen(argv[1], "rb");
    if (png_file == NULL) {
        printf("Unable to open the file %s\n", argv[1]);
        return 1;
    }

    png_byte header[8];
    unsigned long header_size = sizeof(header);
    if (fread(header, 1, header_size, png_file) != header_size) {
        printf("Error reading the file %s\n", argv[1]);
        return 1;
    }

    if (png_sig_cmp(header, 0, header_size)) {
        printf("File %s is not a valid PNG file\n", argv[1]);
        return 1;
    }

    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
        printf("Unable to initialize PNG reading\n");
        return 1;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        printf("Unable to initialize PNG info\n");
        return 1;
    }

    png_init_io(png_ptr, png_file);
    png_set_sig_bytes(png_ptr, header_size);

    png_read_info(png_ptr, info_ptr);

    // Source info
    int width = 0;
    int height = 0;
    int bit_depth = png_get_bit_depth(png_ptr, info_ptr);
    int color_type = png_get_color_type(png_ptr, info_ptr);
    
    // Palette
    int palette_entry_count = 0;
    png_colorp palette_entries = NULL;

    // Row pointers
    png_bytepp source_row_pointers;

    if (bit_depth != 8) {
        printf("PNG format not supported for conversion. (8bit pixel depth required)\n");
        return 1;
    }

    // Read PNG width and height
    png_get_IHDR(
        png_ptr,
        info_ptr,
        (png_uint_32*)(&width),
        (png_uint_32*)(&height),
        &bit_depth,
        &color_type,
        NULL,
        NULL,
        NULL
    );
    
    if (color_type == PNG_COLOR_TYPE_PALETTE) {
        png_get_PLTE(png_ptr, info_ptr, &palette_entries, &palette_entry_count);
    }

    // Row pointers for PNG read
    unsigned char* image_bytes = (unsigned char*)malloc(sizeof(unsigned char) * width * height * 8);
    source_row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * height);
    for (int i = 0; i < height; ++i) {
        int row_length = i * width * 8;
        source_row_pointers[i] = (png_bytep)(image_bytes + row_length);
    }
    png_read_image(png_ptr, source_row_pointers);

    // Close PNG file
    png_read_end(png_ptr, NULL);
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    fclose(png_file);

    // Write pointers
    png_structp result_png_ptr = NULL;
    png_infop result_png_info_ptr = NULL;
    png_bytep result_row = NULL;
    FILE* result_file_ptr = NULL;

    // Open result file
    char* filename = "result.png";
    result_file_ptr = fopen(filename, "wb");
    if (result_file_ptr == NULL) {
        fprintf(stderr, "Could not open file %s from writing\n", filename);
        return 1;
    }

    // Initialize write structure
    result_png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (result_png_ptr == NULL) {
        fprintf(stderr, "Could not allocate write struct\n");
        return 1;
    }

    // Initialize info structure
    result_png_info_ptr = png_create_info_struct(result_png_ptr);
    if (result_png_info_ptr == NULL) {
        fprintf(stderr, "Could not allocate write info struct\n");
        return 1;
    }

    png_init_io(result_png_ptr, result_file_ptr);
    png_set_IHDR(
        result_png_ptr,
        result_png_info_ptr,
        width,
        height,
        bit_depth,
        color_type,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_BASE,
        PNG_FILTER_TYPE_BASE
    );
    
    if (color_type == PNG_COLOR_TYPE_PALETTE) {
        printf("source palette:\n");

        for (int i = 0; i < palette_entry_count; i++) {
            png_color color = palette_entries[i];
            printf("idx %i => %i, %i, %i\n", i, color.red, color.green, color.blue);
        }

        // Fix colors in the palette
        for (int i = 0; i < palette_entry_count; i++) {
            palette_entries[i] = get_fixed_color(palette_entries[i]);
        }

        printf("====================\n");
        printf("result palette:\n");

        for (int i = 0; i < palette_entry_count; i++) {
            png_color color = palette_entries[i];
            printf("idx %i => %i, %i, %i\n", i, color.red, color.green, color.blue);
        }

        // Set palette
        png_set_PLTE(result_png_ptr, result_png_info_ptr, palette_entries, palette_entry_count);

        // Write info of PNG
        png_write_info(result_png_ptr, result_png_info_ptr);

        // Just copy all row pointers to the file (preserve color indexes)
        for (int i = 0; i < height; i++) {
            png_write_row(result_png_ptr, source_row_pointers[i]);
        }
    } else {
        // TODO: rgba format
    }    
    
    png_write_end(result_png_ptr, result_png_info_ptr);

    fclose(result_file_ptr);
    png_free_data(result_png_ptr, result_png_info_ptr, PNG_FREE_ALL, -1);
    png_destroy_write_struct(&result_png_ptr, (png_infopp)NULL);
    free(result_row);

    return 0;
}

int check_arguments(int argc)
{
    if (argc < 2 || argc > 3) {
        printf("Usage: png2n64 file [format]\n");
        printf("format: can be 'rgba32' or 'rgba16'\n");
        return 1;
    }

    return 0;
}

png_color get_fixed_color(png_color source) {
    png_color result;

    if (source.red < 255) {
        result.red = source.red - 32;
    } else {
        result.red = 240;
    }

    if (source.green < 255) {
        result.green = source.green - 32;
    } else {
        result.green = 240;
    }

    if (source.blue < 255) {
        result.blue = source.blue - 32;
    } else {
        result.blue = 240;
    }

    return result;
}
