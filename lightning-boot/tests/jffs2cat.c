
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "libjffs2.h"

#define OUTBUF_SIZE	(1024*1024*5)

int target_endian = __LITTLE_ENDIAN;

/*
 * Main program
 */
int main(int argc, char **argv)
{
	int fd, fdo;
	unsigned int len;
	struct stat st;
	char *data, *output;

	if(argc < 4) {
		printf("usage: %s <image> <file_path> <output_path>\n", 
				argv[0]);
		exit(1);
	}

	fd = open(argv[1], O_RDONLY);
	if(fd == -1) {
		perror("open input file");
		exit(1);
	}

	if(fstat(fd, &st)) {
		perror("get image size");
		close(fd);
		exit(2);
	}

	data = malloc((size_t) st.st_size);
	if(!data) {
		perror("out of memory");
		close(fd);
		exit(3);
	}

	output = malloc(OUTBUF_SIZE);
	if(!output) {
		perror("out of memory");
		free(data);
		close(fd);
		exit(4);
	}

	read(fd, data, st.st_size);
	close(fd);

	len = jffs2_cat(data, (unsigned int)st.st_size, (unsigned char *)argv[2],
			output);

	if(len > 0) {
		fdo = open(argv[3], O_WRONLY|O_TRUNC|O_CREAT);
		if(fdo == -1) {
			perror("open output file");
			exit(5);
		}
		write(fdo, output, len);
		close(fdo);
	}

	free(data);
	free(output);

	exit(0);
}
