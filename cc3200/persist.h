#ifndef _PERSIST_H
#define _PERSIST_H

#include <stddef.h>
#include <stdint.h>

#define PAGE_SIZE 4096

struct persist {
	uint32_t next:24;
	uint32_t cold_status:1;
	uint32_t hot_status:1;
	uint32_t btn_status:1;
	uint32_t base_count;
	uint32_t timestamp;
	uint32_t event[(PAGE_SIZE - offsetof(struct persist, event)) / sizeof(uint32_t)];
};

#endif
