#include "eit.h"
#include "demux.h"

#include <bitstream/dvb/si/eit.h>

static struct pnr_s *get_program(struct epgdump_s *epgd, uint16_t pnr, uint16_t ts_id)
{
	int i;
	struct pnr_s *p;

	for (i = 0; i < epgd->eit.program_n; i++)
		if (epgd->eit.program[i].pnr == pnr && epgd->eit.program[i].ts_id == ts_id)
			return &epgd->eit.program[i];

	epgd->eit.program_n++;
	epgd->eit.program = realloc(epgd->eit.program, epgd->eit.program_n * sizeof(*epgd->eit.program));
	if (!epgd->eit.program)
		critical_error("get_program: error allocating memory\n");

	p = &epgd->eit.program[epgd->eit.program_n - 1];
	memset(p, 0, sizeof(*p));
	p->pnr = pnr;
	p->ts_id = ts_id;

	return p;
}

static void eit_pid_parse(uint8_t *psi, void *arg)
{
	uint8_t table = psi_get_tableid(psi);
	struct epgdump_s *epgd = arg;
	struct pnr_s     *program;
	uint8_t section;
//	uint8_t  last_segment;
	int table_pos;

	if (table < EIT_TABLE_ID_SCHED_ACTUAL_FIRST || table > EIT_TABLE_ID_SCHED_OTHER_LAST)
		return;

	if (epgd->other_only && table < EIT_TABLE_ID_SCHED_OTHER_FIRST)
		return;

	if (epgd->current_only && table > EIT_TABLE_ID_SCHED_ACTUAL_LAST)
		return;

	if (!eit_validate(psi))
		return;
	
	if (!psi_get_current(psi))
		return;

	program = get_program(epgd, eit_get_sid(psi), eit_get_tsid(psi));

	section = psi_get_section(psi);
	table_pos = table - EIT_TABLE_ID_SCHED_ACTUAL_FIRST;

	if (program->section[table_pos][section] &&
			memcmp(psi, program->section[table_pos][section], psi_get_length(psi) + PSI_HEADER_SIZE) == 0) {
		epgd->eit.duplicate_counter++;
		if (epgd->eit.duplicate_counter == 5) {
			fprintf(stderr, "\n");
			stop();
		}
		return;
	}

	fprintf(stderr, ".");
	fflush(stderr);
	epgd->eit.duplicate_counter = 0;

//	dump_hex("EIT: ", psi, psi_get_length(psi) + PSI_HEADER_SIZE);

//	last_segment = eit_get_segment_last_sec_number(psi);
//	fprintf(stderr, "EIT service id 0x%04x, segment %d/%d/%d, last_segments %d/%d/%d, version %d, ptr %p\n", eit_get_sid(psi), table_pos, section / 8, section % 8, table_pos, last_segment / 8, last_segment % 8, psi_get_version(psi), program->section[table_pos][section]);

	if (!program->section[table_pos][section])
		program->section[table_pos][section] = psi_private_allocate();

	memcpy(program->section[table_pos][section], psi, psi_get_length(psi) + PSI_HEADER_SIZE);
	reset_timeout(epgd);
}

void eit_pid_init(struct epgdump_s *epgd)
{
	demux_add_psi_pid(&epgd->demux, EIT_PID, eit_pid_parse, epgd);
}

void eit_pid_fini(struct epgdump_s *epgd)
{
	demux_del_psi_pid(&epgd->demux, EIT_PID);
}

