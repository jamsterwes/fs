#include "tests.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "headers.h"

void write_test_file(char* file_path) {
    // Init the volume header
    volume_header* vhead = new_header(volume);
    init_header(volume, vhead);
    // Populate the volume header
    strcpy_s(vhead->volume_name, 128, "wes_volume");

    // Create new list to keep up with location headers
    location_header* locs = new_list(location_header, 1);

    // Init a new location header
    char* data = "HELLO THERE MY NAME IS WESLEY, TESTING 1 2 3";
    new_file("test_file.txt", strlen(data), vhead, locs, data);

    /**** PRINT TO TERMINAL ****/

    print_volume_header(vhead);

    for (fs_pointer i = 1LL; i < vhead->location_header_count; i++) {
        print_location_header(&(locs[i - 1LL]), i);
    }

    /**** WRITE TO FILE ****/

    // Open file for writing
    FILE* fp;
    fopen_s(&fp, file_path, "wb+");

    // Write file header
    fwrite(vhead, sizeof(volume_header), 1, fp);

    for (fs_pointer i = 0LL; i < vhead->location_header_count; i++) {
        // Write everything but the data from the location header
        fwrite(&(locs[i]), sizeof(location_header_sizer), 1, fp);
        // Write the data from the location header
        fwrite(locs[i].file_data, locs[i].data_size, 1, fp);
        // Free the file data from memory
        free(locs[i].file_data);
    }

    free(locs);
    free(vhead);
    fclose(fp);
}
