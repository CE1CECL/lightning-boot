#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>

#define u32 unsigned int
#define NAND_EB_SIZE	0x20000

struct tfs_hdr {
	u32 magic;
	u32 num_files;
} __attribute__((__packed__));

struct tfs_sum {
	char file_name[64];
	unsigned int num_blocks;
} __attribute__((__packed__));

static struct {
	struct tfs_hdr *header;
	struct tfs_sum *summary;
	u32 data_block;
} tfs_info;

int tfs_load_summary(char *buf)
{
	tfs_info.header = (struct tfs_hdr*)buf;
	tfs_info.summary = buf+sizeof(struct tfs_hdr);
	tfs_info.data_block = (u32)buf+NAND_EB_SIZE;

	printf("magic: 0x%08X, %d files\n", tfs_info.header->magic,
			tfs_info.header->num_files);

	return 0;
}

int tfs_get_file(char *buf, char *name)
{
	struct tfs_sum *s;
	char *p;
	int i;

	s = tfs_info.summary;

	for(i = 0; i < tfs_info.header->num_files; i++) {
		printf("file: \"%s\" (%d blocks)\n", s->file_name, 
				s->num_blocks);
		p = (char *)s;
		p += sizeof(struct tfs_sum);
		s = (struct tfs_sum *)p;

		if(!strncmp(s->file_name, name, 64)) {
			printf("found\n");
			return 0;
		}
	}

	return -1;
}

int main(int argc, char **argv)
{
	int fd;
	char *tfs_buf;
	char *out_buf;
	struct stat st;

	if(argc < 3) {
		printf("usage: %s <image> <file_name>\n", argv[0]);
		return 0;
	}

	fd = open(argv[1], O_RDONLY);
	if(fd == -1) {
		perror("open input file");
		return 1;
	}

	if(fstat(fd, &st)) {
		perror("get image size");
		close(fd);
		return 1;
	}

	tfs_buf = malloc((size_t) st.st_size);
	if(!tfs_buf) {
		perror("out of memory");
		close(fd);
		return 1;
	}

	read(fd, tfs_buf, st.st_size);
	close(fd);

	if(tfs_load_summary(tfs_buf) == 0 ) {
		printf("PASS: load summary\n");
	}
	else {
		printf("FAIL: load summary\n");
		free(tfs_buf);
		exit(1);
	}

	if(tfs_get_file(tfs_buf, argv[2]) == 0) {
		printf("PASS: find file \"%s\"\n", argv[2]);
	}
	else {
		printf("ERROR: %s not found in image\n", argv[2]);
	}

	free(tfs_buf);
	return 0;
}
