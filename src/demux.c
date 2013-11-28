#include "demux.h"

#include <bitstream/mpeg/ts.h>
#include <bitstream/mpeg/psi.h>
#include <bitstream/mpeg/psi/psi_s.h>

#include "util.h"

static void process_psi(struct pidinfo_s *p, uint8_t *ts)
{
	uint8_t        cc;
	const uint8_t *payload_ptr;
	uint8_t        payload_len;
	uint8_t       *section;

	cc = ts_get_cc(ts);

	if (!ts_has_payload(ts))
		return;

	if (p->cc != -1
			&& ts_check_discontinuity(cc, p->cc)
			&& !psi_assemble_empty_s(p->psi_section, &p->psi_section_used)) {
		psi_assemble_reset_s(p->psi_section, &p->psi_section_used);
	}


	payload_ptr = ts_section(ts);
	payload_len = ts + TS_SIZE - payload_ptr;

	if (!psi_assemble_empty_s(p->psi_section, &p->psi_section_used)) {
		section = psi_assemble_payload_s(p->psi_section, &p->psi_section_used, &payload_ptr, &payload_len);

		if (section)
			p->cb(section, p->arg);
	} else if (!ts_get_unitstart(ts)) {
		return;
	}

	payload_ptr = ts_next_section(ts);
	payload_len = ts + TS_SIZE - payload_ptr;

	while(payload_len) {
		section = psi_assemble_payload_s(p->psi_section, &p->psi_section_used, &payload_ptr, &payload_len);

		if (section)
			p->cb(section, p->arg);
	}
}

void demux_add_psi_pid(struct demux_s *dmx, uint16_t pid, psi_callback_t cb, void *arg)
{
	if (dmx->pid_table[pid].cb)
		return;

	dmx->pid_table[pid].cb = cb;
	dmx->pid_table[pid].cc = -1;
	dmx->pid_table[pid].psi_section = psi_private_allocate();
	dmx->pid_table[pid].arg = arg;
}

void demux_del_psi_pid(struct demux_s *dmx, uint16_t pid)
{
	dmx->pid_table[pid].cb = NULL;
	free(dmx->pid_table[pid].psi_section);

/*	for (i = 0; i < MAX_PIDS; i++)
		if (dmx->pid_table[i].cb)
			break;

	if (i == MAX_PIDS)
		fprintf(stderr, "FIXME break event loop\n");*/
}

void demux_input_ts(struct demux_s *dmx, uint8_t *ts)
{
	uint16_t pid;

	if (!ts_validate(ts)) {
		fprintf(stderr, "Malformed TS packet\n");
		dump_hex("TS", ts, TS_SIZE);
		return;
        }

	pid = ts_get_pid(ts);

	if (!dmx->pid_table[pid].cb)
		return;

	process_psi(&dmx->pid_table[pid], ts);
	dmx->pid_table[pid].cc = ts_get_cc(ts);
}
