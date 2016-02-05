#ifndef _PERSIST_H
#define _PERSIST_H

#include <stddef.h>
#include <stdint.h>

#define PAGE_SIZE 4096
#define MAXEV ((PAGE_SIZE - sizeof(struct hdr)) / sizeof(uint32_t))

struct persist {
	struct hdr {
		uint32_t next:24;
		uint32_t st_cld:1;
		uint32_t st_hot:1;
		uint32_t st_btn:1;
		uint32_t basecount;
		uint32_t timestamp;
	} hdr;
	uint32_t event[MAXEV];
};

#endif
