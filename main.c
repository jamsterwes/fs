#include "headers.h"
#include "tests.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STREAM_BUFFER_SIZE 2097152

void print_progress(double percent) {
    int width = 60;
    printf("[");
    int pos = width * percent;
    for (int i = 0; i < width; i++) {
        if (i < pos) printf("=");
        else if (i == pos) printf(">");
        else printf(" ");
    }
    printf("] %.2f%%\r", percent * 100.0);
    fflush(stdout);
}

void finish_progress() {
    printf("\n");
}

void list(int argc, char** argv) {
    // Allocate location amount and volume header storage
    fs_size loc_amount;
    volume_header* vhead = new_header(volume);

    // Open file
    FILE* fp;
    fopen_s(&fp, argv[2], "rb+");

    // Read volume header from file
    fread_s((void*)vhead, sizeof(volume_header), sizeof(volume_header), 1, fp);

    // Print volume header info
    print_volume_header(vhead);

    // Read amount of locations from volume header
    loc_amount = vhead->location_header_count;

    // Init pointer to store current location header in loop
    location_header *lh = new_header(location);
    init_header(location, lh);

    // Loop through each (location header + file data) pair
    for (fs_pointer i = 1; i <= loc_amount; i++) {
        // Read location header (minus data) from current position
        fread_s((void*)lh, sizeof(location_header_sizer), sizeof(location_header_sizer), 1, fp);
        // Print location header info
        print_location_header(lh, lh->this_id);

        // Skip past file data
        fseek(fp, lh->data_size, SEEK_CUR);
    }

    // Close file
    fclose(fp);

    // Free temporary location header pointer
    free(lh);
    // Free temporary volume header pointer
    free(vhead);
}

void extract(int argc, char** argv) {
    // Allocate volume header storage
    volume_header* vhead = new_header(volume);
    // Allocate extraction path storage
    char* extraction_path;

    // Open archive file
    FILE* afp;
    fopen_s(&afp, argv[2], "rb");

    // Read volume header from archive
    fread_s((void*)vhead, sizeof(volume_header), sizeof(volume_header), 1, afp);

    // Create temporary location header
    location_header* lh = new_header(location);
    init_header(location, lh);

    // Loop through each (location header + file data) pair
    for (fs_pointer i = 1; i <= vhead->location_header_count; i++) {
        // Read location header (minus data) from current position
        fread_s((void*)lh, sizeof(location_header_sizer), sizeof(location_header_sizer), 1, afp);

        // If this is the correct location & this is a file
        if (lh->this_id == strtoull(argv[3], NULL, 10) && lh->is_file == FS_TRUE) {
            // Set extraction path
            if (argc >= 5) {
                // Use user-supplied path
                extraction_path = argv[4];
            } else {
                // Use internal name
                extraction_path = lh->location_name;
            }

            // Open the output file
            FILE* tfp;
            fopen_s(&tfp, extraction_path, "wb");

            // Create new progress bar
            print_progress(0.0);

            // Stream contents of archive file at index into target file
            size_t last_read;
            size_t total_written = 0;
            fs_size amt_left = lh->data_size;
            void* buffer = malloc(STREAM_BUFFER_SIZE);
            do {
                if (amt_left < STREAM_BUFFER_SIZE) {
                    last_read = fread_s(buffer, STREAM_BUFFER_SIZE, 1, amt_left, afp);
                } else {
                    last_read = fread_s(buffer, STREAM_BUFFER_SIZE, 1, STREAM_BUFFER_SIZE, afp);
                }
                total_written += last_read;
                amt_left -= last_read;
                if (last_read > 0) {
                    fwrite(buffer, last_read, 1, tfp);
                }
                print_progress((double)total_written / (double)lh->data_size);
            } while (last_read == STREAM_BUFFER_SIZE);
            finish_progress();

            // Free streaming buffer
            free(buffer);

            // Close target file
            fclose(tfp);

            // End file searching
            break;
        }
        // Skip past file data
        fseek(afp, lh->data_size, SEEK_CUR);
    }

    // Close archive file
    fclose(afp);

    // Free temporary location header pointer
    free(lh);

    // Free temporary volume header pointer
    free(vhead);
}

void create(int argc, char** argv) {
    // Init the volume header
    volume_header* vhead = new_header(volume);
    init_header(volume, vhead);
    // Populate the volume header
    strcpy_s(vhead->volume_name, 128, argv[3]);

    // Open the output file
    FILE* fp;
    fopen_s(&fp, argv[2], "wb+");

    // Write the volume header
    fwrite(vhead, sizeof(volume_header), 1, fp);

    // Close the output file
    fclose(fp);

    // Free the volume header
    free(vhead);

    // Log action
    printf("Wrote new volume '%s' to %s\n", argv[3], argv[2]);
}

void insert(int argc, char** argv) {
    // Allocate volume header storage
    volume_header* vhead = new_header(volume);

    // Open archive file
    FILE* afp;
    fopen_s(&afp, argv[2], "rb+");

    // Open target file
    FILE* tfp;
    fopen_s(&tfp, argv[3], "rb");

    // Get size of target file
    _fseeki64(tfp, 0LL, SEEK_END);
    fs_size tf_size = _ftelli64(tfp);
    rewind(tfp);

    // Read volume header from archive
    fread_s((void*)vhead, sizeof(volume_header), sizeof(volume_header), 1, afp);
    rewind(afp);

    // Init pointer to store new file
    location_header *lh = new_header(location);
    init_header(location, lh);
    lh->data_size = tf_size;
    lh->this_id = ++vhead->location_header_count;
    strcpy_s(lh->location_name, 128, argv[3]);
    lh->is_file = FS_TRUE;

    // Overwrite new volume header
    fwrite((void*)vhead, sizeof(volume_header), 1, afp);
    fflush(afp);

    // Append new file's location header
    _fseeki64(afp, 0LL, SEEK_END);
    fwrite((void*)lh, sizeof(location_header_sizer), 1, afp);

    // Create new progress bar
    print_progress(0.0);

    // Stream contents of target file into archive file
    size_t last_read;
    size_t total_written = 0;
    void* buffer = malloc(STREAM_BUFFER_SIZE);
    do {
        last_read = fread_s(buffer, STREAM_BUFFER_SIZE, 1, STREAM_BUFFER_SIZE, tfp);
        total_written += last_read;
        if (last_read > 0) {
            fwrite(buffer, last_read, 1, afp);
        }
        print_progress((double)total_written / (double)tf_size);
    } while (last_read == STREAM_BUFFER_SIZE);
    finish_progress();

    // Close target file
    fclose(tfp);

    // Close archive file
    fclose(afp);

    // Free the buffer
    free(buffer);

    // Free temporary location header pointer
    free(lh);
    // Free temporary volume header pointer
    free(vhead);

    // Log action
    printf("Inserted file %s into archive %s\n", argv[3], argv[2]);
}

void delete(int argc, char** argv) {
    // Allocate volume header storage
    volume_header* vhead = new_header(volume);

    // Open archive file
    FILE* afp;
    fopen_s(&afp, argv[2], "rb");

    // Open temp file
    FILE* tfp;
    char* temp_filename = (char*)calloc(strlen(argv[2]) + 5, sizeof(char));
    strcat_s(temp_filename, strlen(argv[2]) + 5, argv[2]);
    strcat_s(temp_filename, strlen(argv[2]) + 5, ".tmp");
    fopen_s(&tfp, temp_filename, "wb");

    // Read volume header from archive
    fread_s((void*)vhead, sizeof(volume_header), sizeof(volume_header), 1, afp);

    // Decrement total file count
    vhead->location_header_count--;
    // Write volume header to temp file
    fwrite((void*)vhead, sizeof(volume_header), 1, tfp);

    // Create temporary location header
    location_header* lh = new_header(location);
    init_header(location, lh);

    // Loop through each header/data pair
    for (fs_pointer i = 1; i <= vhead->location_header_count; i++) {
        // Read current location header (minus memory-only data pointer)
        fread_s((void*)lh, sizeof(location_header_sizer), sizeof(location_header_sizer), 1, afp);

        // If this is the correct location and a file
        if (lh->this_id == strtoull(argv[3], NULL, 10) && lh->is_file == FS_TRUE) {
            // SKIP THIS PORTION OF THE FILE
            fseek(afp, lh->data_size, SEEK_CUR);
        } else {
            // Write this pair to the temp file
            fwrite((void*)lh, sizeof(location_header_sizer), 1, tfp);
            void* buffer = malloc(lh->data_size);
            fread_s((void*)buffer, lh->data_size, lh->data_size, 1, afp);
            fwrite((void*)buffer, lh->data_size, 1, tfp);
        }
    }

    // Delete original file
    fclose(afp);
    remove(argv[2]);

    // Rename temp file to original file
    fclose(tfp);
    rename(temp_filename, argv[2]);
}

#define ACTION(flag, alias, argcount) (strcmp(argv[1], flag) == 0 || strcmp(argv[1], alias) == 0) && argc >= (argcount + 2)

#define bin_name "fs"
#define usage "Usage:\n%s list <archive>\n%s extract <archive> <index>  [output name]\n%s create <archive> <volume name>\n%s insert <archive> <target file>\n%s delete <archive> <index>"

void print_usage() {
    printf(usage, bin_name, bin_name, bin_name, bin_name, bin_name);
}

int main(int argc, char** argv) {
    if (argc > 1) {
        if (ACTION("list", "ls", 1)) {  // list <filename>
            list(argc, argv);
        } else if (ACTION("extract", "e", 2)) {  // extract <filename> <index> [output name]
            printf("Extracting file %llu from %s\n", strtoull(argv[3], NULL, 10), argv[2]);
            extract(argc, argv);
        } else if (ACTION("create", "c", 2)) {  // create <filename> <volume name>
            printf("Creating new volume '%s' for %s\n", argv[3], argv[2]);
            create(argc, argv);
        } else if (ACTION("insert", "i", 2)) {
            printf("Inserting file %s into archive %s\n", argv[3], argv[2]);
            insert(argc, argv);
        } else if (ACTION("delete", "rm", 2)) {
            printf("Removing file %llu from archive %s\n", strtoull(argv[3], NULL, 10), argv[2]);
            delete(argc, argv);
        } else {
            print_usage();
        }
    } else {
        print_usage();
    }

    return 0;
}
