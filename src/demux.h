#pragma once

#include <stdint.h>

#define MAX_PIDS 0x2000

typedef void (* psi_callback_t)(uint8_t *data, void *arg);

struct pidinfo_s {
	int             cc;
	uint8_t        *psi_section;
	uint16_t        psi_section_used;
	psi_callback_t  cb;
	void           *arg;
//	int             argi;
//	int             done;
};

struct demux_s {
	struct pidinfo_s pid_table[MAX_PIDS];
	int pkt_counter;
};

void demux_init();
void demux_input_ts(struct demux_s *dmx, uint8_t *ts);

void demux_add_psi_pid(struct demux_s *dmx, uint16_t pid, psi_callback_t cb, void *arg);
void demux_del_psi_pid(struct demux_s *dmx, uint16_t pid);

