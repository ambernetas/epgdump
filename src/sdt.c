#include "sdt.h"
#include "demux.h"

#include <bitstream/dvb/si/sdt.h>

static void sdt_pid_parse(uint8_t *psi, void *arg)
{
	uint8_t *section;
	int      section_len;
	struct epgdump_s *epgd = arg;

	if (!sdt_validate(psi))
		return;

	if (!psi_get_current(psi))
		return;

	if (psi_get_tableid(psi) != SDT_TABLE_ID_ACTUAL)
		return;

	section_len = psi_get_length(psi) + PSI_HEADER_SIZE;
	section = malloc(section_len);
	if (!section)
		critical_error("Memory allocation failed\n");
	memcpy(section, psi, section_len);

	if (!psi_table_section(epgd->sdt.sdt_current, section))
	        return;

	epgd->sdt.complete = 1;
	epgd->sdt.original_network_id = sdt_get_onid(epgd->sdt.sdt_current[0]);
	epgd->sdt.current_ts_id = sdt_get_tsid(epgd->sdt.sdt_current[0]);

	demux_del_psi_pid(&epgd->demux, SDT_PID);
}

void sdt_pid_init(struct epgdump_s *epgd)
{
	epgd->sdt.complete = 0;
	psi_table_init(epgd->sdt.sdt_current);
        demux_add_psi_pid(&epgd->demux, SDT_PID, sdt_pid_parse, epgd);
}

void sdt_pid_fini(struct epgdump_s *epgd)
{
	psi_table_free(epgd->sdt.sdt_current);
}

