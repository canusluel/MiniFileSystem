#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <cassert>
#include <list>
#include <stdlib.h>
#include <fstream>
#include <iostream>

#include "fat.h"
#include "fat_file.h"

using namespace std;
/**
 * Write inside one block in the filesystem.
 * @param  fs           filesystem
 * @param  block_id     index of block in the filesystem
 * @param  block_offset offset inside the block
 * @param  size         size to write, must be less than BLOCK_SIZE
 * @param  buffer       data buffer
 * @return              written byte count
 */
int mini_fat_write_in_block(FAT_FILESYSTEM *fs, const int block_id, const int block_offset, const int size, const void * buffer) {
	assert(block_offset >= 0);
	assert(block_offset < fs->block_size);
	assert(size + block_offset <= fs->block_size);

	int written = 0;

	// TODO: write in the real file.

	return written;
}

/**
 * Read inside one block in the filesystem
 * @param  fs           filesystem
 * @param  block_id     index of block in the filesystem
 * @param  block_offset offset inside the block
 * @param  size         size to read, must fit inside the block
 * @param  buffer       buffer to write the read stuff to
 * @return              read byte count
 */
int mini_fat_read_in_block(FAT_FILESYSTEM *fs, const int block_id, const int block_offset, const int size, void * buffer) {
	assert(block_offset >= 0);
	assert(block_offset < fs->block_size);
	assert(size + block_offset <= fs->block_size);

	int read = 0;

	// TODO: read from the real file.
	
	return read;
}


/**
 * Find the first empty block in filesystem.
 * @return -1 on failure, index of block on success
 */
int mini_fat_find_empty_block(const FAT_FILESYSTEM *fat) {
	// TODO: find an empty block in fat and return its index.
	int i = 0;
	for(i; i<fat->block_map.size(); i++){
		if(fat->block_map[i] == EMPTY_BLOCK) return i;
	}
	return -1;
}

/**
 * Find the first empty block in filesystem, and allocate it to a type,
 * i.e., set block_map[new_block_index] to the specified type.
 * @return -1 on failure, new_block_index on success
 */
int mini_fat_allocate_new_block(FAT_FILESYSTEM *fs, const unsigned char block_type) {
	int new_block_index = mini_fat_find_empty_block(fs);
	if (new_block_index == -1)
	{
		fprintf(stderr, "Cannot allocate block: filesystem is full.\n");
		return -1;
	}
	fs->block_map[new_block_index] = block_type;
	return new_block_index;
}

void mini_fat_dump(const FAT_FILESYSTEM *fat) {
	printf("Dumping fat with %d blocks of size %d:\n", fat->block_count, fat->block_size);
	for (int i=0; i<fat->block_count;++i) {
		printf("%d ", (int)fat->block_map[i]);
	}
	printf("\n");

	for (int i=0; i<fat->files.size(); ++i) {
		mini_file_dump(fat, fat->files[i]);
	}
}

static FAT_FILESYSTEM * mini_fat_create_internal(const char * filename, const int block_size, const int block_count) {
	FAT_FILESYSTEM * fat = new FAT_FILESYSTEM;
	fat->filename = filename;
	fat->block_size = block_size;
	fat->block_count = block_count;
	fat->block_map.resize(fat->block_count, EMPTY_BLOCK); // Set all blocks to empty.
	fat->block_map[0] = METADATA_BLOCK;
	return fat;
}

/**
 * Create a new virtual disk file.
 * The file should be of the exact size block_size * block_count bytes.
 * Overwrites existing files. Resizes block_map to block_count size.
 * @param  filename    name of the file on real disk
 * @param  block_size  size of each block
 * @param  block_count number of blocks
 * @return             FAT_FILESYSTEM pointer with parameters set.
 */
FAT_FILESYSTEM * mini_fat_create(const char * filename, const int block_size, const int block_count) {

	FAT_FILESYSTEM * fat = mini_fat_create_internal(filename, block_size, block_count);

	// TODO: create the corresponding virtual disk file with appropriate size
	ofstream ofs(filename, ios::binary | ios::out | ios::in);
	ofs.seekp((block_count*block_size) << 3);
    ofs.write("", 1);
	ofs.close();
	return fat;
}

/**
 * Save a virtual disk (filesystem) to file on real disk.
 * Stores filesystem metadata (e.g., block_size, block_count, block_map, etc.)
 * in block 0.
 * Stores file metadata (name, size, block map) in their corresponding blocks.
 * Does not store file data (they are written directly via write API).
 * @param  fat virtual disk filesystem
 * @return     true on success
 */
bool mini_fat_save(const FAT_FILESYSTEM *fat) {
	FILE * fat_fd = fopen(fat->filename, "wb+");
	if (fat_fd == NULL) {
		perror("Cannot save fat to file");
		return false;
	}
 	char buffer[100];
	// TODO: save all metadata (filesystem metadata, file metadata).
	int block_size = fat->block_size;
	int block_count = fat->block_count;

	string block_count_str = to_string(block_count);
	char *block_count_char = strcat(strdup(block_count_str.c_str())," ");

	string block_size_str = to_string(block_size);
	char *block_size_char = strcat(strdup(block_size_str.c_str())," "); 


	// saving block_count and block size to virtual disk file
	fwrite(block_count_char, strlen(block_count_char), 1, fat_fd);
	fwrite(block_size_char , strlen(block_size_char), 1 , fat_fd);
	//saving filesystem block map
	for(int i = 0; i<block_count; i++){
		string fat_map_index_str = to_string(fat->block_map[i]);
		char *fat_block_map_char = strcat(strdup(fat_map_index_str.c_str())," "); 
		fwrite(fat_block_map_char , strlen(fat_block_map_char) , 1 , fat_fd); 
	}

	//saving file metadata
	for(int i = 0; i<fat->files.size(); i++){
		FAT_FILE *current_file = fat->files[i];
		fseek(fat_fd, current_file->metadata_block_id*block_size, SEEK_SET);
		//writing the size of current file
		string curr_file_size_str = to_string(current_file->size);
		char *current_file_size_char = strcat(strdup(curr_file_size_str.c_str())," "); 
		fwrite(current_file_size_char , strlen(current_file_size_char) , 1 , fat_fd); 

		//writing the name of current file
		char *current_file_name = strcat(current_file->name, " ");
		fwrite(current_file_name, strlen(current_file_name) , 1 , fat_fd); 
		//writing block ids of current file
		for(int j = 0; j<current_file->block_ids.size(); j++){
			string curr_file_bid_str = to_string(current_file->block_ids[j]);
			char *current_file_bid_char = strcat(strdup(curr_file_bid_str.c_str())," "); 
			fwrite(current_file_bid_char , strlen(current_file_bid_char) , 1 , fat_fd); 
		}
	}
	fclose(fat_fd);
	return true;
}

FAT_FILESYSTEM * mini_fat_load(const char *filename) {
	FILE * fat_fd = fopen(filename, "rb+");
	int c;
	char fat_metadata[100];

	if (fat_fd == NULL) {
		perror("Cannot load fat from file");
		exit(-1);
	}
	// TODO: load all metadata (filesystem metadata, file metadata) and create filesystem.
	int block_size, block_count;
	char * fat_map_index;	
	int i = 0;
	fread(fat_metadata, 100, 1, fat_fd);
		//getting block size and count from file
		block_count = atoi(strtok (fat_metadata, " "));
		block_size = atoi(strtok (NULL, " "));
		fat_map_index = strtok (NULL," ");

		FAT_FILESYSTEM * fat = mini_fat_create(filename, block_size, block_count);
		//assigning block map
		int val;
		while (fat_map_index != NULL)
		{	
			val = atoi(fat_map_index);
			fat->block_map.at(i) = val;
			i++;
			fat_map_index = strtok (NULL, " ");

		}
	//for every file in system, adding their metadata
	for (int j = 0; j<block_count; j++){
		if(fat->block_map.at(j) == 1){
			char current_file_data[100];
			char * current_file_blockids;
			fseek(fat_fd, block_size*j, SEEK_SET);
			fread(current_file_data, 100, 1, fat_fd);
			int current_file_size = atoi(strtok(current_file_data," "));
			char * current_file_name = strtok (NULL, " ");
			current_file_blockids = strtok (NULL, " ");
			//erasing the metadata block id data from blockmap so it can be declared in mini_file_create_file function
			fat->block_map.at(j) = EMPTY_BLOCK;
			FAT_FILE *current_file = mini_file_create_file(fat, current_file_name);
			current_file->size = current_file_size;
				//assigning block map
				while (current_file_blockids != NULL)
				{	
					current_file->block_ids.push_back(atoi(current_file_blockids));
					current_file_blockids = strtok (NULL, " ");
				}

		}
	}
	fclose(fat_fd);
	return fat;
}
