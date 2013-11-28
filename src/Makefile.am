bin_PROGRAMS = epgdump

epgdump_SOURCES = epgdump.c dev.c demux.c eit.c sdt.c util.c content_nibble_table.c iso_639_conversion.c output_xmltv.c
epgdump_LDADD = @LIBEVENT_LIBS@

AM_CFLAGS = @LIBEVENT_CFLAGS@ -I../include -Wall

ISO-639-2_utf-8.txt:
	wget http://www.loc.gov/standards/iso639-2/ISO-639-2_utf-8.txt

iso_639.map.h: ISO-639-2_utf-8.txt iso_639.awk
	$(AWK) -f iso_639.awk $< > $@
