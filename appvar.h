#ifndef APPVAR_H
#define APPVAR_H

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "main.h"

#define MAX_APPVARS 20
#define MAX_OFFSETS 512

typedef struct {
    uint8_t *output;
    uint8_t curr_image;
    uint8_t max_images;
    uint16_t offsets[MAX_OFFSETS];
    unsigned offset;
    char name[9];
    group_t *g;
} appvar_t;

unsigned image_is_in_an_appvar(const char *image_name);
void output_appvar_init(appvar_t *a, int num_images);
void add_appvars_data(const unsigned i, const uint8_t *data, const size_t size);
void add_appvar_data(appvar_t *a, const uint8_t *data, const size_t size);
void output_appvar_complete(appvar_t *a);

extern appvar_t appvar[MAX_APPVARS];
extern appvar_t *appvar_ptrs[MAX_APPVARS];

#endif