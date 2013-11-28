#pragma once

#include <bitstream/mpeg/psi/psi.h>
#include <bitstream/dvb/si/eit.h>

struct pnr_s {
	uint16_t  pnr;
	uint16_t  ts_id;
	int       network_id;
	uint8_t  *section[EIT_TABLE_ID_SCHED_OTHER_LAST - EIT_TABLE_ID_SCHED_ACTUAL_FIRST+1][PSI_TABLE_MAX_SECTIONS];
//	int       estimate[EIT_TABLE_ID_SCHED_OTHER_LAST - EIT_TABLE_ID_SCHED_ACTUAL_FIRST][PSI_TABLE_MAX_SECTIONS];
};

struct eit_s {
	struct pnr_s *program;
	int           program_n;
	int           duplicate_counter;
};

#include "epgdump.h"

void eit_pid_init(struct epgdump_s *epgd);
void eit_pid_fini(struct epgdump_s *epgd);

