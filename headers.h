#ifndef __FS_HEADER_H__
#define __FS_HEADER_H__

#include "specials.h"

#define new_header(t) (t ## _header*)malloc(sizeof(t ## _header))
#define init_header(t, x) memset(x, 0, sizeof(t ## _header))
#define new_list(t, c) (t*)calloc(c, sizeof(t));
#define FS_FALSE 0x0
#define FS_TRUE 0x1

typedef struct struct_volume_header {
    char volume_name[128];
    fs_size location_header_count;
} volume_header;

typedef struct struct_location_header_sizer {
    fs_pointer this_id;  // starts counting from 1
    fs_size data_size;
    fs_pointer parent_location;  // can be 0 to notate the root
    char location_name[128];
    char is_file;
} location_header_sizer;

typedef struct struct_location_header {
    fs_pointer this_id;  // starts counting from 1
    fs_size data_size;
    fs_pointer parent_location;  // can be 0 to notate the root
    char location_name[128];
    char is_file;
    char* file_data;
} location_header;

void print_volume_header(volume_header* vhead);
void print_location_header(location_header* lh, fs_pointer i);
void new_file(char* file_name, fs_size size, volume_header* vhead, location_header* list, char* file_data);

#endif  // __FS_HEADER_H__
