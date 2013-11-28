#pragma once

#include <stdint.h>
#include <mxml.h>
#include <event2/event.h>

struct epgdump_s;
#include "demux.h"
#include "sdt.h"
#include "eit.h"

#define MAX_PIDS 0x2000

struct epgdump_s {
	// arg variables
	const char *demux_dev;
	char        demux_dev_buffer[64];
	int         timeout;
	int         current_only;
	int         other_only;

	int input_buffer_size;

	// dev
	int           demux_fd;
	struct event *demux_event;
	uint8_t      *buffer;

	struct event_base *evb;
	struct event      *timeout_ev;

	struct demux_s demux;

	struct sdt_s sdt;
	struct eit_s eit;

	mxml_node_t *xmltv;
};

//extern struct epgdump_s glob;

void critical_error(const char *format, ...);
void stop();
void reset_timeout(struct epgdump_s *epgd);

