/*
 *  libjffs2.c -- basic read-only JFFS2 support.
 *
 *  This code is heavily based on jffs2dump.c, copyright Thomas Gleixner and 
 *  distributed as part of mtd-utils (http://www.linux-mtd.infradead.org/)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <mtd/jffs2-user.h>
#include <endian.h>
#include <byteswap.h>

#include "crc32.h"
#include "summary.h"

#define PAD(x) (((x)+3)&~3)

/* For outputting a byte-swapped version of the input image. */
#define cnv_e32(x) ((jint32_t){bswap_32(x.v32)})
#define cnv_e16(x) ((jint16_t){bswap_16(x.v16)})

#define t32_backwards(x) ({ uint32_t __b = (x); (target_endian==__BYTE_ORDER)?bswap_32(__b):__b; })
#define cpu_to_e32(x) ((jint32_t){t32_backwards(x)})

/*
 *	Dump image contents
 */
unsigned int do_dumpcontent(char *data, unsigned int imglen, char *target_file,                                char *output)
{
	char *p = data, *p_free_begin;
	union jffs2_node_union *node;
	int empty = 0, dirty = 0;
	char name[256];
	uint32_t crc, target_pino;
	uint16_t type;
	int bitchbitmask = 0;
	int obsolete;
	int len = 0;

	p_free_begin = NULL;
	while (p < (data + imglen)) {
	node = (union jffs2_node_union *) p;

	/* Skip empty space */
	if (!p_free_begin)
		p_free_begin = p;
	if (je16_to_cpu(node->u.magic) == JFFS2_EMPTY_BITMASK
		&& je16_to_cpu(node->u.nodetype) == JFFS2_EMPTY_BITMASK) {
		p += 4;
		empty += 4;
		continue;
	}

	if (p != p_free_begin)
		printf("Empty space found from 0x%08x to 0x%08x\n",
		   p_free_begin - data, p - data);
	p_free_begin = NULL;

	if (je16_to_cpu(node->u.magic) != JFFS2_MAGIC_BITMASK) {
		if (!bitchbitmask++)
		printf("Wrong bitmask  at  0x%08x, 0x%04x\n", p - data,
			   je16_to_cpu(node->u.magic));
		p += 4;
		dirty += 4;
		continue;
	}
	bitchbitmask = 0;

	type = je16_to_cpu(node->u.nodetype);
	if ((type & JFFS2_NODE_ACCURATE) != JFFS2_NODE_ACCURATE) {
		obsolete = 1;
		type |= JFFS2_NODE_ACCURATE;
	} else
		obsolete = 0;
	/* Set accurate for CRC check */
	node->u.nodetype = cpu_to_je16(type);

	crc = crc32(0, node, sizeof(struct jffs2_unknown_node) - 4);
	if (crc != je32_to_cpu(node->u.hdr_crc)) {
		printf("Wrong hdr_crc  at  0x%08x, 0x%08x instead of 0x%08x\n",
		   p - data, je32_to_cpu(node->u.hdr_crc), crc);
		p += 4;
		dirty += 4;
		continue;
	}

	switch (je16_to_cpu(node->u.nodetype)) {

	case JFFS2_NODETYPE_INODE:
		printf
		("%8s Inode	  node at 0x%08x, totlen 0x%08x, #ino  %5d, version %5d, isize %8d, csize %8d, dsize %8d, offset %8d\n",
		 obsolete ? "Obsolete" : "", p - data,
		 je32_to_cpu(node->i.totlen), je32_to_cpu(node->i.ino),
		 je32_to_cpu(node->i.version), je32_to_cpu(node->i.isize),
		 je32_to_cpu(node->i.csize), je32_to_cpu(node->i.dsize),
		 je32_to_cpu(node->i.offset));

		if(je32_to_cpu(node->d.pino) == target_pino) {
			len = je32_to_cpu(node->i.isize);
			strncpy(output, p+sizeof(struct jffs2_raw_inode), len);
		}

		crc = crc32(0, node, sizeof(struct jffs2_raw_inode) - 8);
		if (crc != je32_to_cpu(node->i.node_crc)) {
		printf
			("Wrong node_crc at  0x%08x, 0x%08x instead of 0x%08x\n",
			 p - data, je32_to_cpu(node->i.node_crc), crc);
		p += PAD(je32_to_cpu(node->i.totlen));
		dirty += PAD(je32_to_cpu(node->i.totlen));;
		continue;
		}

		crc =
		crc32(0, p + sizeof(struct jffs2_raw_inode),
			  je32_to_cpu(node->i.csize));
		if (crc != je32_to_cpu(node->i.data_crc)) {
		printf
			("Wrong data_crc at  0x%08x, 0x%08x instead of 0x%08x\n",
			 p - data, je32_to_cpu(node->i.data_crc), crc);
		p += PAD(je32_to_cpu(node->i.totlen));
		dirty += PAD(je32_to_cpu(node->i.totlen));;
		continue;
		}

		p += PAD(je32_to_cpu(node->i.totlen));
		break;

	case JFFS2_NODETYPE_DIRENT:
		memcpy(name, node->d.name, node->d.nsize);
		name[node->d.nsize] = 0x0;
		printf("%8s Dirent	 node at 0x%08x, totlen 0x%08x, #pino %5d, version %5d, #ino  %8d, nsize %8d, name %s\n",
			obsolete ? "Obsolete" : "", p - data,
			je32_to_cpu(node->d.totlen), je32_to_cpu(node->d.pino),
			je32_to_cpu(node->d.version), je32_to_cpu(node->d.ino),
			node->d.nsize, name);

		if(!strcmp(name, target_file))
			target_pino = je32_to_cpu(node->d.ino);

		crc = crc32(0, node, sizeof(struct jffs2_raw_dirent) - 8);
		if (crc != je32_to_cpu(node->d.node_crc)) {
			printf("Wrong node_crc at  0x%08x, 0x%08x instead of 0x%08x\n",
				p - data, je32_to_cpu(node->d.node_crc), crc);
			p += PAD(je32_to_cpu(node->d.totlen));
			dirty += PAD(je32_to_cpu(node->d.totlen));;
			continue;
		}

		crc = crc32(0, p + sizeof(struct jffs2_raw_dirent), 
				node->d.nsize);
		if (crc != je32_to_cpu(node->d.name_crc)) {
			printf("Wrong name_crc at  0x%08x, 0x%08x instead of 0x%08x\n",
			 p - data, je32_to_cpu(node->d.name_crc), crc);
			p += PAD(je32_to_cpu(node->d.totlen));
			dirty += PAD(je32_to_cpu(node->d.totlen));;
			continue;
		}

		p += PAD(je32_to_cpu(node->d.totlen));
		break;

	case JFFS2_NODETYPE_SUMMARY:
		{

		int i;
		struct jffs2_sum_marker *sm;

		printf
			("%8s Inode Sum  node at 0x%08x, totlen 0x%08x, sum_num  %5d, cleanmarker size %5d\n",
			 obsolete ? "Obsolete" : "", p - data,
			 je32_to_cpu(node->s.totlen),
			 je32_to_cpu(node->s.sum_num),
			 je32_to_cpu(node->s.cln_mkr));

		crc = crc32(0, node, sizeof(struct jffs2_raw_summary) - 8);
		if (crc != je32_to_cpu(node->s.node_crc)) {
			printf
			("Wrong node_crc at  0x%08x, 0x%08x instead of 0x%08x\n",
			 p - data, je32_to_cpu(node->s.node_crc), crc);
			p += PAD(je32_to_cpu(node->s.totlen));
			dirty += PAD(je32_to_cpu(node->s.totlen));;
			continue;
		}

		crc =
			crc32(0, p + sizeof(struct jffs2_raw_summary),
			  je32_to_cpu(node->s.totlen) -
			  sizeof(struct jffs2_raw_summary));
		if (crc != je32_to_cpu(node->s.sum_crc)) {
			printf
			("Wrong data_crc at  0x%08x, 0x%08x instead of 0x%08x\n",
			 p - data, je32_to_cpu(node->s.sum_crc), crc);
			p += PAD(je32_to_cpu(node->s.totlen));
			dirty += PAD(je32_to_cpu(node->s.totlen));;
			continue;
		}

		void *sp;
		sp = (p + sizeof(struct jffs2_raw_summary));

		for (i = 0; i < je32_to_cpu(node->s.sum_num); i++) {

			switch (je16_to_cpu
				(((struct jffs2_sum_unknown_flash *) sp)->
				 nodetype)) {
			case JFFS2_NODETYPE_INODE:
			{

				struct jffs2_sum_inode_flash *spi;
				spi = sp;

				printf
				("%14s #ino  %5d,  version %5d, offset 0x%08x, totlen 0x%08x\n",
				 "", je32_to_cpu(spi->inode),
				 je32_to_cpu(spi->version),
				 je32_to_cpu(spi->offset),
				 je32_to_cpu(spi->totlen));

				sp += JFFS2_SUMMARY_INODE_SIZE;
				break;
			}

			case JFFS2_NODETYPE_DIRENT:
			{

				char name[255];
				struct jffs2_sum_dirent_flash *spd;
				spd = sp;

				memcpy(name, spd->name, spd->nsize);
				name[spd->nsize] = 0x0;

				printf
				("%14s dirent offset 0x%08x, totlen 0x%08x, #pino  %5d,  version %5d, #ino  %8d, nsize %8d, name %s \n",
				 "", je32_to_cpu(spd->offset),
				 je32_to_cpu(spd->totlen),
				 je32_to_cpu(spd->pino),
				 je32_to_cpu(spd->version),
				 je32_to_cpu(spd->ino), spd->nsize, name);

				sp += JFFS2_SUMMARY_DIRENT_SIZE(spd->nsize);
				break;
			}

			default:
			printf("Unknown summary node!\n");
			break;
			}

			sm = (struct jffs2_sum_marker *) ((char *) p +
							  je32_to_cpu(node->s.
								  totlen) -
							  sizeof(struct
								 jffs2_sum_marker));

			printf
			("%14s Sum Node Offset  0x%08x, Magic 0x%08x, Padded size 0x%08x\n",
			 "", je32_to_cpu(sm->offset),
			 je32_to_cpu(sm->magic),
			 je32_to_cpu(node->s.padded));
		}

		p += PAD(je32_to_cpu(node->s.totlen));
		break;
		}

	case JFFS2_NODETYPE_CLEANMARKER:
		printf("%8s Cleanmarker	 at 0x%08x, totlen 0x%08x\n",
		   obsolete ? "Obsolete" : "",
		   p - data, je32_to_cpu(node->u.totlen));
		p += PAD(je32_to_cpu(node->u.totlen));
		break;

	case JFFS2_NODETYPE_PADDING:
		printf("%8s Padding	node at 0x%08x, totlen 0x%08x\n",
		   obsolete ? "Obsolete" : "",
		   p - data, je32_to_cpu(node->u.totlen));
		p += PAD(je32_to_cpu(node->u.totlen));
		break;

	case JFFS2_EMPTY_BITMASK:
		p += 4;
		empty += 4;
		break;

	default:
		printf("%8s Unknown	node at 0x%08x, totlen 0x%08x\n",
		   obsolete ? "Obsolete" : "",
		   p - data, je32_to_cpu(node->u.totlen));
		p += PAD(je32_to_cpu(node->u.totlen));
		dirty += PAD(je32_to_cpu(node->u.totlen));

	}
	}

	printf("Empty space: %d, dirty space: %d\n", empty, dirty);

	return len;
}

/* jffs2_resolve_inode -- find an inode */
struct jffs2_raw_inode *jffs2_resolve_inode(char *data, unsigned int imglen,
		uint32_t ino)
{
	struct jffs2_raw_inode *inode = NULL;
	union jffs2_node_union *node = (union jffs2_node_union *)data;
	uint32_t crc;
	char *p;

	printf("start @ %p (len=0x%X)\n", (char *)node, imglen);

	do {
		/* find a JFFS2 node */
		while((char *)node < data+imglen && 
			je16_to_cpu(node->u.magic) != JFFS2_MAGIC_BITMASK) {
			p = (char *)node;
			p += 4;
			node = (union jffs2_node_union *)p;
		}

		/* still at a valid node? */
		if((char *)node < data+imglen) {
			printf("CHECKING @ %p\n", (char *)node);

			/* node is a INODE
			 * with matching inode
			 * (and better version, if we have one)
			 * with valid CRC */
			if(je16_to_cpu(node->u.nodetype) == JFFS2_NODETYPE_INODE &&
				ino == je32_to_cpu(node->i.ino) &&
				(!inode || 
				 (je32_to_cpu(inode->version) <= je32_to_cpu(node->i.version)))
				 ) {
				/* now check CRC, if valid then
				 * we have a potential match */
				crc = crc32(0, node, sizeof(struct jffs2_unknown_node) - 4);
				if(crc == je32_to_cpu(node->u.hdr_crc)) {
					printf("changing pointer\n");
					inode =  &(node->i);
					/* XXX */
					if(je32_to_cpu(inode->dsize) < je32_to_cpu(inode->isize))
						return inode;
				}
			}
	
			printf("PADDING...\n");	
			/* skip past this one to check the next possible node */
			p = (char *)node;
			p += PAD(je32_to_cpu(node->s.totlen));
			node = (union jffs2_node_union *)p;
		} else { /* we're done, return the best match */
			printf("done @ %p\n", (char *)node);
			return inode;
		}
	} while(1);
}

void jffs2_retrieve_data(char *data, unsigned int imglen, 
		struct jffs2_raw_inode *start, char *output)
{
	union jffs2_node_union *node = (union jffs2_node_union *)start;
	char *p = (char *)start;
	uint32_t crc;
	unsigned int version = je32_to_cpu(start->version);
	int size = je32_to_cpu(start->isize);
	unsigned int ino = je32_to_cpu(start->ino);

	do {
		/* find a JFFS2 node */
		while((char *)node < data+imglen && 
			je16_to_cpu(node->u.magic) != JFFS2_MAGIC_BITMASK) {
			p = (char *)node;
			p += 4;
			node = (union jffs2_node_union *)p;
		}


		/* still at a valid node? */
		if((char *)node < data+imglen) {

			/* node is an INODE
			 * with matching ino
			 * with version we're looking for
			 * with valid CRC */
			if(je16_to_cpu(node->u.nodetype) == JFFS2_NODETYPE_INODE &&
				je32_to_cpu(node->i.ino) == ino &&
				je32_to_cpu(node->i.version) == version) {
				/* now check CRC, if valid then
				 * we have a potential match */
				crc = crc32(0, node, sizeof(struct jffs2_unknown_node) - 4);
				if(crc == je32_to_cpu(node->u.hdr_crc)) {
					/* copy the contents */
					memcpy(output,
						p+sizeof(struct jffs2_raw_inode), 
						je32_to_cpu(node->i.csize));
					/* adjust pointers and indexes */
					output += je32_to_cpu(node->i.csize);
					size -= je32_to_cpu(node->i.csize);
					version++;
				}
			}
		
			/* skip past this one to check the next possible node */
			p = (char *)node;
			p += PAD(je32_to_cpu(node->s.totlen));
			node = (union jffs2_node_union *)p;
		} else { /* we're done, return the best match */
			return;
		}
	} while(size > 0);
}

/* jffs2_resolve_dirent -- find a directory entry given a path name */
struct jffs2_raw_dirent *jffs2_resolve_dirent(char *data, unsigned int imglen,
		unsigned char *path, unsigned int plen, uint32_t ino)
{
	struct jffs2_raw_dirent *dd = NULL;
	union jffs2_node_union *node = (union jffs2_node_union *)data;
	uint32_t crc, v, vmax = 0;
	char *p;

	do {
		/* find a JFFS2 node */
		while((char *)node < data+imglen && 
			je16_to_cpu(node->u.magic) != JFFS2_MAGIC_BITMASK) {
			p = (char *)node;
			p += 4;
			node = (union jffs2_node_union *)p;
		}

		/* still at a valid node? */
		if((char *)node < data+imglen) {

			/* node is a DIRENT
			 * with matching name length
			 * and matching name
			 * with >= version 
			 * with valid CRC */
			if(je16_to_cpu(node->u.nodetype) == JFFS2_NODETYPE_DIRENT) {

				printf("dirent: checking @ %d\n",
						(unsigned int)node-(unsigned int)data);
				if((ino == 0 || je32_to_cpu(node->d.pino) == ino) &&
				node->d.nsize == plen &&
				!memcmp(&node->d.name, path, node->d.nsize) &&
				(v = je32_to_cpu(node->d.version)) >= vmax) {
				/* now check CRC, if valid and this is a better version, then
				 * we have a potential match */
				crc = crc32(0, node, sizeof(struct jffs2_unknown_node) - 4);
				if(crc == je32_to_cpu(node->u.hdr_crc) && vmax <= v) {
					vmax = v;
					dd = &(node->d);
				}
			}
 } /*XXX*/
		
			/* skip past this one to check the next possible node */
			p = (char *)node;
			p += PAD(je32_to_cpu(node->s.totlen));
			node = (union jffs2_node_union *)p;
		} else { /* we're done, return the best match */
			return dd;
		}
	} while(1);
}

unsigned int jffs2_cat(char *data, unsigned int imglen,
				unsigned char *path, char *output)
{
	uint32_t ino = 0;
	unsigned char *p = path;
	unsigned char *e, *i;
	struct jffs2_raw_dirent *dir = NULL;
	struct jffs2_raw_inode *inode = NULL;

	while(*p != '\0') {
		if(*p == '/') {
				p++;
				continue;
		}
		for(e = p+1; *e != '\0' && *e != '/'; e++);
	
		dir = jffs2_resolve_dirent(data, imglen, p, (unsigned int)(e-p), ino);
		if(dir) {
				printf("found dirent \"");
				for(i = p; i < e; i++)
						putchar(*i);
				printf("\" with pino=%d, ino=%d\n", ino, je32_to_cpu(dir->ino));
				ino = je32_to_cpu(dir->ino);
		}
		else {
				printf("ERROR: didn't find dirent \"");
				for(i = p; i < e; i++)
						putchar(*i);
				printf("\" with pino=%d\n", ino);
				return 0;
		}

		p = e;
	}

	inode = jffs2_resolve_inode(data, imglen, ino);
	if(inode) {
		printf("found inode, isize=%d, version=%d\n",
			je32_to_cpu(inode->isize),
			je32_to_cpu(inode->version));
		if(je32_to_cpu(inode->isize) > je32_to_cpu(inode->csize)) {
			printf("data is split into multiple inodes\n");
			jffs2_retrieve_data(data, imglen, inode, output);
		}
		else {
			printf("data fits into single inode\n");
			memcpy(output,
				(char*)inode+sizeof(struct jffs2_raw_inode), 
				je32_to_cpu(inode->isize));
		}
	}
	else {
			printf("ERROR: didn't find the inode\n");
			return 0;
	}

	return je32_to_cpu(inode->isize);
}
