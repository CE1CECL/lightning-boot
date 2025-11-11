unsigned int do_dumpcontent(char *data, unsigned int imglen, char *target_file,
				char *output);
struct jffs2_raw_dirent *jffs2_resolve_dirent(char *data, unsigned int imglen,
		                unsigned char *path, unsigned int plen, uint32_t ino);
struct jffs2_raw_dirent *jffs2_resolve(char *data, unsigned int imglen,
		                unsigned char *path);
unsigned int jffs2_cat(char *data, unsigned int imglen,
		                unsigned char *path, char *output);
