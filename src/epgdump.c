#include "epgdump.h"

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <argp.h>

#include <bitstream/mpeg/psi.h>

#if HAVE_CONFIG_H
	#include <config.h>
#endif

#include "dev.h"
#include "sdt.h"
#include "eit.h"
#include "output_xmltv.h"

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define DEFAULT_DEMUX_DEV "/dev/dvb/adapter0/demux0"
#define DEFAULT_TIMEOUT 10

struct epgdump_s glob;

const char *argp_program_version = PACKAGE_STRING;
const char *argp_program_bug_address = "<" PACKAGE_BUGREPORT ">";
static char args_doc[] = "[-a NUM | -d DEV] [-t NUM]";
static char doc[] = "Tool to dump EGP data fom MPEG-TS stream.";

static struct argp_option options[] = {
	{"adapter",      'a', "NUM",   0, "Use /dev/dvb/adapterNUM/demux0 DVB adapter (-a and -d are mutually exclusive)." },
	{"device",       'd', "DEV",   0, "Use DEV as DVB demux device (default: " DEFAULT_DEMUX_DEV ")." },
	{"timeout",      't', "INT",   0, "Timeout after INT seconds if no EPG data availabale (default: " TOSTRING(DEFAULT_TIMEOUT) ")." },
	{"current-only", 'c',     0,   0, "Print only current (actual) TS EPG data (-o and -c are mutually exclusive)." },
	{"other-only",   'o',     0,   0, "Print only other TS EPG data (-o and -c are mutually exclusive)." },
	{ 0 }
};

static error_t parse_opt (int key, char *arg, struct argp_state *state);
static struct argp argp = { options, parse_opt, args_doc, doc };

void critical_error(const char *format, ...)
{
	va_list pvar;

	va_start(pvar, format);
	vfprintf(stderr, format, pvar);
	va_end(pvar);

	exit(EXIT_FAILURE);
}

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
	int adapter_nr;

	switch(key) {
	case 'o':
		glob.current_only = 0;
		glob.other_only = 1;
		break;
	case 'c':
		glob.current_only = 1;
		glob.other_only = 0;
		break;
	case 't':
		glob.timeout = atoi(arg);
		if (glob.timeout < 1)
			critical_error("timeout (%d) must be >= 1 sec\n", glob.timeout);

		break;
	case 'a':
		adapter_nr = atoi(arg);
		if (adapter_nr < 0)
			critical_error("adapter number (%s) must be >= 0\n", arg);
		snprintf(glob.demux_dev_buffer, sizeof(glob.demux_dev_buffer), "/dev/dvb/adapter%d/demux0", adapter_nr);
		glob.demux_dev = glob.demux_dev_buffer;
		break;
	case 'd':
		glob.demux_dev = arg;
		break;
	}

	return 0;
}

void stop()
{
	event_base_loopbreak(glob.evb);
}

static void timeout_cb(evutil_socket_t fd, short event, void *arg)
{
	fprintf(stderr, "Time out\n");
	stop();
}

void reset_timeout(struct epgdump_s *epgd)
{
	struct timeval tv;

	tv.tv_sec = epgd->timeout;
	tv.tv_usec = 0;
	evtimer_add(epgd->timeout_ev, &tv);
}

int main(int argc, char *argv[])
{
	int	optind;

	bzero(&glob, sizeof(glob));

	glob.demux_dev = DEFAULT_DEMUX_DEV;
	glob.timeout = DEFAULT_TIMEOUT;
	glob.input_buffer_size = 1024 * TS_SIZE;

	argp_parse(&argp, argc, argv, 0, &optind, 0);

	glob.evb = event_base_new();
	if (!glob.evb)
		critical_error("%s: error allocating memory\n", argv[0]);

	glob.timeout_ev = evtimer_new(glob.evb, timeout_cb, NULL);
	if (!glob.timeout_ev)
		critical_error("%s: error allocating memory\n", argv[0]);

	reset_timeout(&glob);

	dev_init(&glob, glob.demux_dev);
	sdt_pid_init(&glob);
	eit_pid_init(&glob);

	event_base_dispatch(glob.evb);

	output_xmltv(&glob);

	dev_fini(&glob);
	sdt_pid_fini(&glob);
	eit_pid_fini(&glob);

	evtimer_del(glob.timeout_ev);
	event_base_free(glob.evb);

	return EXIT_SUCCESS;
}
