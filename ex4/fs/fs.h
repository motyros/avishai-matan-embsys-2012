#ifndef __EMBSYS_FS_H_
#define __EMBSYS_FS_H_

typedef enum {
	SUCCESS =							0x00000000,
	FS_NOT_READY =						0x00000001,
	FS_IS_BUSY =						0x00000002,
	FAILURE_ACCESSING_FLASH =			0x00000003,
	COMMAND_PARAMETERS_ERROR =			0x00000004,
	FILE_NOT_FOUND = 					0x00000005,
	MAXIMUM_FILE_SIZE_EXCEEDED =		0x00000006,
	MAXIMUM_FLASH_SIZE_EXCEEDED =		0x00000007,
	MAXIMUM_FILES_CAPACITY_EXCEEDED =	0x00000008,
	FAILURE =							0x80000000,
} FS_STATUS;

typedef struct  {
	
	// number of flash blocks available to the file system.
	// each block is of 4KBytes size
	unsigned block_count;

} FS_SETTINGS;

/*

  Description:
	Initialize the file system. 
	Must be called before any other operation on the file system.

  Arguments:
	settings - initialization information required to initialize the file system.

*/
FS_STATUS fs_init(const FS_SETTINGS settings);

/*

  Description:
	Write a file.

  Arguments:
	filename - the name of the file.
	length - the size of the 'data' input buffer.
	data - a buffer holding the file content.

*/
FS_STATUS fs_write(const char* filename, unsigned length, const char* data);

/*

  Description:
	Return the size of a file.
	File size is defined as the length of the file content only.

  Arguments:
	filename - the name of the file.
	length - an out parameter to hold the file size.

*/
FS_STATUS fs_filesize(const char* filename, unsigned* length);

/*

  Description:
	Read the content of a file.

  Arguments:
	filename - the name of the file.
	length - when calling the function this argument should hold the size of the 'data' input buffer.
			 when the function return this argument will hold the file size, i.e. the actual used space size of the 'data' buffer.
	data - a pointer for a buffer to hold the file content.


*/
FS_STATUS fs_read(const char* filename, unsigned* length, char* data);

/*

  Description:
	Read the content of a file.

  Arguments:
	index  - the index of the desired file (zero based)
	length - when calling the function this argument should hold the size of the 'data' input buffer.
			 when the function return this argument will hold the file size, i.e. the actual used space size of the 'data' buffer.
	data - a pointer for a buffer to hold the file content.


*/
FS_STATUS fs_read_by_index(unsigned index,unsigned* length, char* data);

/*

  Description:
	Erase a file.
  Arguments:
	filename - the name of the file.

*/
FS_STATUS fs_erase(const char* filename);

/*

  Description:
	Return the file count in the file system
  Arguments:
	file_count - an out argument to hold the number of files in the file system.

*/
FS_STATUS fs_count(unsigned* file_count);

/*

  Description:
	List all the files exist in the file system.
  Arguments:
	length - when calling the function this argument should hold the size of the "files" buffer.
                 when the function return this argument will hold the the actual used space size of the 'files' buffer, including the last null byte.
	files - a series of continuous null terminated strings, each representing a file name in the file system

*/
FS_STATUS fs_list(unsigned* length, char* files);

/* 
	==========================================================================
									Usage Sample
	   (a naive fs usage, with all buffers declared with max expected size)
	==========================================================================
	
	FS_SETTINGS settings;
	const char* file1data = "hello";
	const char* file2data = "bye";
	char files[MAX_FILES_COUNT*MAX_FILE_SIZE];
	char data[MAX_FILE_SIZE];
	unsigned count;
	char *p;

	settings.block_count = 16;

	if (SUCCESS != fs_init(settings)) {
		... error handling ... 
	}

	if (SUCCESS != fs_write("file1", strlen(file1data), file1data){
		... error handling ... 
	}

	if (SUCCESS != fs_write("file2", strlen(file2data), file2data){
		... error handling ... 
	}

	if (SUCCESS != fs_list(&count, files){
		... error handling ... 
	}

	for( p=files ; count>0 ; count-- ) {
		
		unsigned length = sizeof(data);
		printf("%s\n", p);
		
		if (SUCCESS != fs_read(p, &length, data){
			... error handling ... 
		}

		for (int i=0; i<length; i++) {
			printf("%c", data[i]);
		}
		printf("\n============================================\n");
		p+=strlen(p)+1;

	}

*/

#endif