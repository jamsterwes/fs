#include "headers.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_volume_header(volume_header* vhead) {
    // Print the volume name
    printf("Volume name: '%s'\n", vhead->volume_name);
    // Print the total amount of location headers
    printf("Files/folders total: %llu\n\n", vhead->location_header_count);
}

void print_location_header(location_header* lh, fs_pointer i) {
    // Print the location's name
    printf("[%llu]: '%s'\n", i, lh->location_name);
    // Print the location's type
    printf("   Type: %s\n", lh->is_file ? "File" : "Folder");
    // If the location is a file, print its size
    if (lh->is_file == FS_TRUE) {
        printf("   Size: %.02fKB\n", lh->data_size / 1024.0);
    }
}

void new_file(char* file_name, fs_size size, volume_header* vhead, location_header* locs, char* file_data) {
    location_header* txt_file = new_header(location);
    init_header(location, txt_file);
    // Populate this location header
    strcpy_s(txt_file->location_name, 128, file_name);
    txt_file->is_file = FS_TRUE;
    txt_file->data_size = size;
    // Increment the global location header count
    // & set ID of location
    txt_file->this_id = ++vhead->location_header_count;
    // Add file_data to header
    txt_file->file_data = (char*)malloc(size);
    memcpy_s(txt_file->file_data, size, file_data, size);

    // Add new location header to list
    memcpy_s(&(locs[txt_file->this_id - 1]), sizeof(location_header), txt_file, sizeof(location_header));


    free(txt_file);
}
