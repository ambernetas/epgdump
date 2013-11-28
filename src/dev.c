#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/dvb/dmx.h>

#include <bitstream/mpeg/psi.h>

#include "epgdump.h"

static void demux_event(evutil_socket_t fd, short ev, void *arg)
{
	struct epgdump_s *epgd;
	ssize_t len;
	ssize_t i;

	epgd = arg;

	while(1) {
		len = read(fd, epgd->buffer, epgd->input_buffer_size);
		if (len < 0)
			if (errno != EAGAIN)
				fprintf(stderr, "demux read error %s (%d)\n", strerror(errno), errno);
	
		if (len < TS_SIZE)
			return;

		for (i = 0; i < len; i += TS_SIZE)
			demux_input_ts(&epgd->demux, epgd->buffer+i);
	}
}

static void demux_open(struct epgdump_s *epgd, const char *path)
{
	epgd->demux_fd = open(path, O_RDWR|O_NONBLOCK);
	if (epgd->demux_fd == -1)
		critical_error("Error %s: %s (%d)\n", path, strerror(errno), errno);
}

static void demux_join(struct epgdump_s *epgd)
{
	struct dmx_pes_filter_params filter;

	bzero(&filter, sizeof(filter));

	filter.pid = 0x2000;
//	filter.pid = 0x12;
	filter.input = DMX_IN_FRONTEND;
	filter.output = DMX_OUT_TSDEMUX_TAP;
	filter.pes_type = DMX_PES_OTHER;
	filter.flags = DMX_IMMEDIATE_START;

	if (ioctl(epgd->demux_fd, DMX_SET_PES_FILTER, &filter) == -1)
		critical_error("ioctl DMX_SET_PES_FILTER failed: %s (%d)\n", strerror(errno), errno);
}

void dev_init(struct epgdump_s *epgd, const char *dev_path)
{
	epgd->buffer = malloc(epgd->input_buffer_size);
	if (!epgd->buffer)
		critical_error("Memory allocation failed\n");

	demux_open(epgd, dev_path);

	if (ioctl(epgd->demux_fd, DMX_SET_BUFFER_SIZE, epgd->input_buffer_size) == -1)
		critical_error("ioctl DMX_SET_BUFFER_SIZE failed: %s (%d)\n", strerror(errno), errno);

	epgd->demux_event = event_new(epgd->evb, epgd->demux_fd, EV_READ|EV_PERSIST, demux_event, epgd);
	event_add(epgd->demux_event, NULL);

	demux_join(epgd);
}

void dev_fini(struct epgdump_s *epgd)
{
	close(epgd->demux_fd);

        if (epgd->demux_event)
                event_free(epgd->demux_event);

        free(epgd->buffer);
}

