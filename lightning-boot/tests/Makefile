ifeq ($(PROJECT_PATH),)
PROJECT_PATH=~/Brio2/LinuxDist
endif

CPPFLAGS=-I$(PROJECT_PATH)/packages/mtd-utils/mtd-utils/include
CFLAGS=-std=gnu99 -Wall

ifneq ($(USE_EBS),)
CFLAGS += -DUSE_EBS
endif

jffs2cat: jffs2cat.c libjffs2.c crc32.c
	gcc $(CFLAGS) $(CPPFLAGS) -o jffs2cat jffs2cat.c libjffs2.c crc32.c 

test_tfs: test_tfs.c
	gcc -o test_tfs test_tfs.c

clean:
	rm -f *.o jffs2cat
