#include "fat.h"
#include "fat_file.h"
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <cassert>

// Little helper to show debug messages. Set 1 to 0 to silence.
#define DEBUG 1
inline void debug(const char * fmt, ...) {
#if DEBUG>0
	va_list args;
   va_start(args, fmt);
   vprintf(fmt, args);
   va_end(args);
#endif
}

// Delete index-th item from vector.
template<typename T>
static void vector_delete_index(std::vector<T> &vector, const int index) {
	vector.erase(vector.begin() + index);
	
}

// Find var and delete from vector.
template<typename T>
static bool vector_delete_value(std::vector<T> &vector, const T var) {
	for (int i=0; i<vector.size(); ++i) {
		if (vector[i] == var) {
			vector_delete_index(vector, i);
			return true;
		}
	}
	return false;
}

void mini_file_dump(const FAT_FILESYSTEM *fs, const FAT_FILE *file)
{
	printf("Filename: %s\tFilesize: %d\tBlock count: %d\n", file->name, file->size, (int)file->block_ids.size());
	printf("\tMetadata block: %d\n", file->metadata_block_id);
	printf("\tBlock list: ");
	for (int i=0; i<file->block_ids.size(); ++i) {
		printf("%d ", file->block_ids[i]);
	}
	printf("\n");

	printf("\tOpen handles: \n");
	for (int i=0; i<file->open_handles.size(); ++i) {
		printf("\t\t%d) Position: %d (Block %d, Byte %d), Is Write: %d\n", i,
			file->open_handles[i]->position,
			position_to_block_index(fs, file->open_handles[i]->position),
			position_to_byte_index(fs, file->open_handles[i]->position),
			file->open_handles[i]->is_write);
	}
}


/**
 * Find a file in loaded filesystem, or return NULL.
 */
FAT_FILE * mini_file_find(const FAT_FILESYSTEM *fs, const char *filename)
{
	for (int i=0; i<fs->files.size(); ++i) {
		if (strcmp(fs->files[i]->name, filename) == 0) // Match
			return fs->files[i];
	}
	return NULL;
}

/**
 * Create a FAT_FILE struct and set its name.
 */
FAT_FILE * mini_file_create(const char * filename)
{
	FAT_FILE * file = new FAT_FILE;
	file->size = 0;
	strcpy(file->name, filename);
	return file;
}


/**
 * Create a file and attach it to filesystem.
 * @return FAT_OPEN_FILE pointer on success, NULL on failure
 */
FAT_FILE * mini_file_create_file(FAT_FILESYSTEM *fs, const char *filename)
{
	assert(strlen(filename)< MAX_FILENAME_LENGTH);
	FAT_FILE *fd = mini_file_create(filename);

	int new_block_index = mini_fat_allocate_new_block(fs, FILE_ENTRY_BLOCK);
	if (new_block_index == -1)
	{
		fprintf(stderr, "Cannot create new file '%s': filesystem is full.\n", filename);
		return NULL;
	}
	fs->files.push_back(fd); // Add to filesystem.
	fd->metadata_block_id = new_block_index;
	return fd;
}

/**
 * Return filesize of a file.
 * @param  fs       filesystem
 * @param  filename name of file
 * @return          file size in bytes, or zero if file does not exist.
 */
int mini_file_size(FAT_FILESYSTEM *fs, const char *filename) {
	FAT_FILE * fd = mini_file_find(fs, filename);
	if (!fd) {
		fprintf(stderr, "File '%s' does not exist.\n", filename);
		return 0;
	}
	return fd->size;
}


/**
 * Opens a file in filesystem.
 * If the file does not exist, returns NULL, unless it is write mode, where
 * the file is created.
 * Adds the opened file to file's open handles.
 * @param  is_write whether it is opened in write (append) mode or read.
 * @return FAT_OPEN_FILE pointer on success, NULL on failure
 */
FAT_OPEN_FILE * mini_file_open(FAT_FILESYSTEM *fs, const char *filename, const bool is_write)
{
	FAT_FILE * fd = mini_file_find(fs, filename);	
	if (fd == NULL) {
		// TODO: check if it's write mode, and if so create it. Otherwise return NULL.
		if (is_write) {
			fd = mini_file_create_file(fs, filename);
		}else{
			return NULL;
		}
	}else{
		// TODO: check if other write handles are open.
		if(is_write && fd->open_handles.size() != 0){
			return NULL;
		}
	}

	FAT_OPEN_FILE * open_file;
	if (fd != NULL) {
		open_file = new FAT_OPEN_FILE;
		// TODO: assign open_file fields.
		FILE * fd_txt = fopen(filename, "w");
		open_file->file = fd;
		open_file->is_write = is_write;
		open_file->position = fd->metadata_block_id;

		// Add to list of open handles for fd:
		fd->open_handles.push_back(open_file);
		fclose(fd_txt);
	}else{
		return NULL;
	}
	return open_file;
}

/**
 * Close an existing open file handle.
 * @return false on failure (no open file handle), true on success.
 */
bool mini_file_close(FAT_FILESYSTEM *fs, const FAT_OPEN_FILE * open_file)
{
	if (open_file == NULL) return false;
	FAT_FILE * fd = open_file->file;
	if (vector_delete_value(fd->open_handles, open_file)) {
		return true;
	}

	fprintf(stderr, "Attempting to close file that is not open.\n");
	return false;
}

/**
 * Write size bytes from buffer to open_file, at current position.
 * @return           number of bytes written.
 */
int mini_file_write(FAT_FILESYSTEM *fs, FAT_OPEN_FILE * open_file, const int size, const void * buffer)
{
	int written_bytes = 0;
	int block_index = 0;

	// TODO: write to file.
	printf("%d", open_file->file->block_ids.size());
	if(open_file->file->block_ids.size() == 0){
		block_index = mini_fat_find_empty_block(fs);
		open_file->file->block_ids.push_back(block_index);
	}else{
		block_index = open_file->file->block_ids[0];
	}

	printf("%d", block_index);
	if(block_index == -1){
		printf("No empty block");
	}else{
		//setting open_file position to block_index*block_size to keep the current position of data writing
		open_file->position = block_index*(fs->block_size);
		open_file->position += open_file->file->size;
		//setting block's type to occupied
		fs->block_map[block_index] = FILE_DATA_BLOCK;

		while(written_bytes < size){
			//if current block's size is full, searches for new empty block
			//and sets current position to that blocks start point
			if(open_file->file->size % (fs->block_size + 1) == 0){
				block_index = mini_fat_find_empty_block(fs);
				//if(block_index == -1) return written_bytes;
				open_file->position = block_index*(fs->block_size);
				open_file->position += open_file->file->size;
				fs->block_map[block_index] = FILE_DATA_BLOCK;
			}
				//incrementing written bytes and current position as writing continues
				open_file->file->size++;
				written_bytes++;
				open_file->position += open_file->file->size;
			
		}
	}

	return written_bytes;
}

/**
 * Read up to size bytes from open_file into buffer.
 * @return           number of bytes read.
 */
int mini_file_read(FAT_FILESYSTEM *fs, FAT_OPEN_FILE * open_file, const int size, void * buffer)
{
	int read_bytes = 0;

	// TODO: read file.

	return read_bytes;
}


/**
 * Change the cursor position of an open file.
 * @param  offset     how much to change
 * @param  from_start whether to start from beginning of file (or current position)
 * @return            false if the new position is not available, true otherwise.
 */
bool mini_file_seek(FAT_FILESYSTEM *fs, FAT_OPEN_FILE * open_file, const int offset, const bool from_start)
{
	// TODO: seek and return true.

	return false;
}

/**
 * Attemps to delete a file from filesystem.
 * If the file is open, it cannot be deleted.
 * Marks the blocks of a deleted file as empty on the filesystem.
 * @return true on success, false on non-existing or open file.
 */
bool mini_file_delete(FAT_FILESYSTEM *fs, const char *filename)
{
	// TODO: delete file after checks.
	FAT_FILE * fd = mini_file_find(fs, filename);	
	if (fd == NULL) {
		printf("%s\n", "File is not found");
		return false;
	}else{
		if(fd->open_handles.size() == 0){
			for(int i = 0; i<fd->block_ids.size(); i++){
				fs->block_map[fd->block_ids[i]] = EMPTY_BLOCK;
			}
			return true;
		}else{
			return false;
		}
	}
}
