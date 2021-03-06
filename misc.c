#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

#include "libs/lodepng.h"

#include "main.h"
#include "misc.h"
#include "parser.h"
#include "logging.h"
#include "palettes.h"

void *safe_malloc(size_t n) {
    void* p = malloc(n);
    if (!p) { errorf("out of memory."); }
    return p;
}

void *safe_calloc(size_t n, size_t m) {
    void* p = calloc(n, m);
    if (!p) { errorf("out of memory."); }
    return p;
}

void *safe_realloc(void *a, size_t n) {
    void* p = realloc(a, n);
    if (!p) { errorf("out of memory."); }
    return p;
}

uint16_t rgb1555(const uint8_t r8, const uint8_t g8, const uint8_t b8) {
    uint8_t r5 = round((int)r8 * 31 / 255.0);
    uint8_t g6 = round((int)g8 * 63 / 255.0);
    uint8_t b5 = round((int)b8 * 31 / 255.0);
    return ((g6 & 1) << 15) | (r5 << 10) | ((g6 >> 1) << 5) | b5;
}

uint16_t rgb565(const uint8_t r8, const uint8_t g8, const uint8_t b8) {
    uint8_t r5 = round(((int)r8 * 31 + 127) / 255.0);
    uint8_t g6 = round(((int)g8 * 63 + 127) / 255.0);
    uint8_t b5 = round(((int)b8 * 31 + 127) / 255.0);
    return (b5 << 11) | (g6 << 5) | r5;
}

char *str_dup(const char *s) {
    char *d = safe_malloc(strlen(s)+1);  // allocate memory
    if (d) strcpy(d, s);                 // copy string if okay
    return d;                            // return new memory
}

// encodes a PNG image (used for creating global palettes)
void encodePNG(const char* filename, const unsigned char* image, unsigned width, unsigned height) {
    unsigned char* png;
    size_t pngsize;

    unsigned error = lodepng_encode32(&png, &pngsize, image, width, height);
    if(!error) { lodepng_save_file(png, pngsize, filename); }
    
    /* if there's an error, display it */
    if(error) { printf("error %u: %s\n", error, lodepng_error_text(error)); }

    free(png);
}

// builds an image of the palette
void build_image_palette(const liq_palette *pal, const unsigned length, const char *filename) {
    uint8_t *image = safe_malloc(length * 4);
    unsigned int x;
    for (x = 0; x < length; x++) {
        unsigned int o = x << 2;
        image[o + 0] = pal->entries[x].r;
        image[o + 1] = pal->entries[x].g;
        image[o + 2] = pal->entries[x].b;
        image[o + 3] = 255;
    }
    encodePNG(filename, image, length, 1);
    free(image);
    lof("Saved palette (%s)\n",filename);
}

// create an icon for the C toolchain
int create_icon(void) {
    liq_image *image = NULL;
    liq_result *res = NULL;
    liq_attr *attr = NULL;
    uint8_t *rgba = NULL;
    uint8_t *data = NULL;
    unsigned int width,height,size,error,x,y,h;
    liq_color rgba_color;
    char **icon_options;
    
    int num = make_args(convpng.iconc, &icon_options, ",");
    if(num < 2) { errorf("not enough options."); }
    
    error = lodepng_decode32_file(&rgba, &width, &height, icon_options[0]);
    if(error) { lof("[error] could not open %s for conversion\n", icon_options[0]); exit(1); }
    if(width != ICON_WIDTH || height != ICON_HEIGHT) { errorf("icon image dimensions are not 16x16."); }
    
    attr = liq_attr_create();
    if(!attr) { errorf("could not create image attributes."); }
    image = liq_image_create_rgba(attr, rgba, ICON_WIDTH, ICON_HEIGHT, 0);
    if(!image) { errorf("could not create icon."); }

    size = width * height;

    for (h = 0; h < MAX_PAL_LEN; h++) {
        unsigned int o = h << 2;
        rgba_color.r = xlibc_palette[o + 0];
        rgba_color.g = xlibc_palette[o + 1];
        rgba_color.b = xlibc_palette[o + 2];
        rgba_color.a = xlibc_palette[o + 3];
        
        liq_image_add_fixed_color(image, rgba_color);
    }
    
    data = safe_malloc(size + 1);
    res = liq_quantize_image(attr, image);
    if(!res) {errorf("could not quantize icon."); }
    liq_write_remapped_image(res, image, data, size);

    FILE *out = fopen("iconc.asm", "w");
    if (convpng.icon_zds) {
        fprintf(out, " define .icon,space=ram\n segment .icon\n xdef __icon_begin\n xdef __icon_end\n xdef __program_description\n xdef __program_description_end\n");
    
        fprintf(out,"\n db 1\n db %u,%u\n__icon_begin:",width,height);
        for (y = 0; y < height; y++) {
            fputs("\n db ",out);
            for (x = 0; x < width; x++) {
                fprintf(out, "0%02Xh%s", data[x+(y*width)], x + 1 == width ? "" : ",");
            }
        }

        fprintf(out,"\n__icon_end:\n__program_description:\n");
        fprintf(out," db \"%s\",0\n__program_description_end:\n", icon_options[1]);
        lof("Converted icon '%s'\n", icon_options[0]);
    } else {
        fprintf(out, "__icon_begin:\n .db 1,%u,%u", width, height);
        for (y = 0; y < height; y++) {
            fputs("\n .db ",out);
            for (x = 0; x < width; x++) {
                fprintf(out, "0%02Xh%s", data[x+(y*width)], x + 1 == width ? "" : ",");
            }
        }

        fprintf(out, "\n__icon_end:\n__program_description:\n");
        fprintf(out, " .db \"%s\",0\n__program_description_end:\n", icon_options[1]);
        lof("Converted icon '%s' -> 'iconc.asm'\n", icon_options[0]);
    }
    
    liq_attr_destroy(attr);
    liq_image_destroy(image);
    free_args(&convpng.iconc, &icon_options, num);
    free(data);
    free(rgba);
    fclose(out);
    return 0;
}
