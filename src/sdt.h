#pragma once

#include <stdint.h>
#include <bitstream/mpeg/psi/psi.h>

struct sdt_s {
	int complete;
	uint16_t original_network_id;
	uint16_t current_ts_id;
	PSI_TABLE_DECLARE(sdt_current);
};

#include "epgdump.h"


void sdt_pid_init(struct epgdump_s *epgd);
void sdt_pid_fini(struct epgdump_s *epgd);


