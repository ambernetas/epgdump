#include "output_xmltv.h"

#include <time.h>
#include <mxml.h>
#include <iconv.h>
#include <bitstream/dvb/si/sdt.h>
#include <bitstream/dvb/si/eit.h>
#include <bitstream/dvb/si/datetime.h>
#include <bitstream/dvb/si/descs_list.h>

#include "content_nibble_table.h"
#include "iso_639_conversion.h"

#define NATIVE_ENC "UTF-8"

static char *iconv_append_null(const char *string, size_t length)
{
	char *new_string = malloc(length + 1);

	memcpy(new_string, string, length);
	new_string[length] = '\0';
	return new_string;
}

static char *iconv_wrapper(void *arg, const char *new_encoding, char *string, size_t length)
{
	char   *new_string;
	char   *ptr;
	size_t  new_length;
	iconv_t *handle;

	if (!strcmp(new_encoding, NATIVE_ENC))
		return iconv_append_null(string, length);

	handle = iconv_open(NATIVE_ENC, new_encoding);
	if (handle == (iconv_t)-1) {
		fprintf(stderr, "iconv_wrapper: iconv does not support %s encoding\n", new_encoding);
		return strdup("charset conversion not supported");
	}
	new_length = length*6;
	new_string = calloc(new_length, sizeof(char));
	ptr = new_string;

	if (!new_string)
		critical_error("iconv_wrapper: error allocating memory\n");

	if (iconv(handle, &string, &length, &ptr, &new_length) == -1) {
		fprintf(stderr, "iconv_wrapper: error converting from %s to " NATIVE_ENC "\n", new_encoding);
		free(new_string);
		return strdup("charset conversion error");
	}
	iconv_close(handle);
	return new_string;
}

static inline char *xmltv_timestamp(time_t t, char *buf)
{
	struct tm tm;

	gmtime_r(&t, &tm);
	sprintf(buf, "%04d%02d%02d%02d%02d%02d", tm.tm_year+1900, tm.tm_mon+1,
	                                         tm.tm_mday, tm.tm_hour,
	                                         tm.tm_min, tm.tm_sec);

	return buf;
}

static void dump_xmltv_descriptors(uint8_t *descs, mxml_node_t *programme)
{
	mxml_node_t *node;
	uint8_t  *desc;
	uint8_t   tag;

	int i, j;

	char *string;
	const uint8_t *str;
	uint8_t  str_len;

	uint8_t *ptr;
	
	char *ext_description = NULL;
	mxml_node_t *ext_description_node = NULL;

	for (i = 0; (desc = descs_get_desc(descs, i)); i++) {
		tag = desc_get_tag(desc);
		switch (tag) {
		case 0x42:	// Stuffing descriptor
			// FIXME
			fprintf(stderr, "\t\ttag: %02x (len %u)\n", tag, desc_get_length(desc));
			break;
		case 0x4a:	// Linkage descriptor
			// FIXME
			fprintf(stderr, "\t\ttag: %02x (len %u)\n", tag, desc_get_length(desc));
			break;
		case 0x4d:	// Short event descriptor
			if (!desc4d_validate(desc))
				break;

			str = desc4d_get_event_name(desc, &str_len);
			string = dvb_string_get(str, str_len, iconv_wrapper, NULL);

//			fprintf(stderr, "\t\t\ttitle (%zu): %s\n", strlen(string), string);
			node = mxmlNewElement(programme, "title");
			mxmlElementSetAttr(node, "lang", iso_639_2to1(desc4d_get_lang(desc)));
			mxmlNewText(node, 0, string);
			free(string);

			str = desc4d_get_text(desc, &str_len);
			string = dvb_string_get(str, str_len, iconv_wrapper, NULL);
			if (strlen(string) > 0) {
//				fprintf(stderr, "\t\t\ttext (%zu): %s\n", strlen(string), string);
				node = mxmlNewElement(programme, "sub-title");
				mxmlElementSetAttr(node, "lang", iso_639_2to1(desc4d_get_lang(desc)));
				mxmlNewText(node, 0, string);
			}
			free(string);

			break;
		case 0x4e:	// Extended event descriptor
			if (!desc4e_validate(desc))
				break;
//			fprintf(stderr, "\t\t\tExtended desc %u/%u (%u items)\n", desc4e_get_desc_number(desc), desc4e_get_last_desc_number(desc), desc4e_get_items_length(desc));
/*			for (j = 0; j < desc4e_get_items_length(desc); j++) {
				ptr = desc4e_get_item(desc, j);
				
				str = desc4en_get_item_description(ptr, &str_len);
				string = dvb_string_get(str, str_len, iconv_wrapper, NULL);
				fprintf(stderr, "\t\t\text-desc (%zu): %s\n", strlen(string), string);
				free(string);

				str = desc4en_get_item_text(ptr, &str_len);
				string = dvb_string_get(str, str_len, iconv_wrapper, NULL);
				fprintf(stderr, "\t\t\text-val (%zu): %s\n", strlen(string), string);
				free(string);
			}*/

			str = desc4e_get_text(desc, &str_len);
			string = dvb_string_get(str, str_len, iconv_wrapper, NULL);
//			fprintf(stderr, "\t\t\text-text (%zu): %s\n", strlen(string), string);
			if (ext_description) {
				j = strlen(ext_description);
				ext_description = realloc(ext_description, j + strlen(string) +1);
				memcpy(ext_description + j, string, strlen(string) +1);
				free(string);
			} else {
				ext_description_node = mxmlNewElement(programme, "desc");
				mxmlElementSetAttr(ext_description_node, "lang", iso_639_2to1(desc4e_get_lang(desc)));
				ext_description = string;
			}
			break;
		case 0x4f:	// Time shifted event descriptor
			// FIXME
			fprintf(stderr, "\t\ttag: %02x (len %u)\n", tag, desc_get_length(desc));
			break;
		case 0x50:	// Component descriptor
			// FIXME
			fprintf(stderr, "\t\ttag: %02x (len %u)\n", tag, desc_get_length(desc));
			break;
		case 0x53:	// CA identifier descriptor
			// FIXME
			fprintf(stderr, "\t\ttag: %02x (len %u)\n", tag, desc_get_length(desc));
			break;
		case 0x54:	// Content descriptor
			if (!desc54_validate(desc))
				break;
			for (j = 0; (ptr = desc54_get_content(desc, j)); j++) {
//				node = mxmlNewElement(programme, "!-- test --");
				if (!content_nibble_table[ptr[0]]) {
					//fprintf(stderr, "\t\t\tContent type reserved for future use: nibble1: %u, nibble2: %u, user: %u\n", desc54n_get_content_l1(ptr), desc54n_get_content_l2(ptr), desc54n_get_user(ptr));
					break;
				} else if (!content_nibble_table[ptr[0]][0]) {
					//fprintf(stderr, "\t\t\tContent type is User defined: nibble1: %u, nibble2: %u, user: %u\n", desc54n_get_content_l1(ptr), desc54n_get_content_l2(ptr), desc54n_get_user(ptr));
					node = mxmlNewElement(programme, "category");
					mxmlNewTextf(node, 0, "User defined %x/%x/%02x", desc54n_get_content_l1(ptr), desc54n_get_content_l2(ptr), desc54n_get_user(ptr));
				} else {
//					fprintf(stderr, "\t\t\tContent: %s\n", content_nibble_table[ptr[0]]);
					node = mxmlNewElement(programme, "category");
					mxmlNewText(node, 0, content_nibble_table[ptr[0]]);
				}
			}
			break;
		case 0x55:	// Parental rating descriptor
			if (!desc55_validate(desc))
				break;
			for (j = 0; (ptr = desc55_get_rating(desc, j)); j++) {
//				fprintf(stderr, "\t\t\tParental rating: CC %.3s, rating: %u\n", desc55n_get_country_code(ptr), desc55n_get_rating(ptr));

				switch (desc55n_get_rating(ptr)) {
				case 0x00:
					break;
				case 0x01 ... 0x0f:
					node = mxmlNewElement(programme, "rating");
					mxmlElementSetAttr(node, "system", "dvb");
					node = mxmlNewElement(node, "value");
					mxmlNewInteger(node, desc55n_get_rating(ptr) + 3);
					break;
				default:
					node = mxmlNewElement(programme, "rating");
					mxmlElementSetAttrf(node, "system", "%.3s", desc55n_get_country_code(ptr));
					node = mxmlNewElement(node, "value");
					mxmlNewInteger(node, desc55n_get_rating(ptr) + 3);
					break;

				}
			}
			break;
		case 0x57:	// Tephone descriptor
			// FIXME
			fprintf(stderr, "\t\ttag: %02x (len %u)\n", tag, desc_get_length(desc));
			break;
		case 0x5e:	// Multilingual component descriptor
			// FIXME
			fprintf(stderr, "\t\ttag: %02x (len %u)\n", tag, desc_get_length(desc));
			break;
		case 0x5f:	// Private data specifier descriptor
			// FIXME
			fprintf(stderr, "\t\ttag: %02x (len %u)\n", tag, desc_get_length(desc));
			break;
		case 0x61:	// Short smoothing buffer descriptor
			// FIXME
			fprintf(stderr, "\t\ttag: %02x (len %u)\n", tag, desc_get_length(desc));
			break;
		case 0x64:	// Data broadcast descriptor
			// FIXME
			fprintf(stderr, "\t\ttag: %02x (len %u)\n", tag, desc_get_length(desc));
			break;
		case 0x69:	// PDC descriptor
			// FIXME
			fprintf(stderr, "\t\ttag: %02x (len %u)\n", tag, desc_get_length(desc));
			break;
		case 0x75:	// TVA_id_descriptor
			// FIXME
			fprintf(stderr, "\t\ttag: %02x (len %u)\n", tag, desc_get_length(desc));
			break;
		case 0x76:	// content_identifier_descriptor
			// FIXME
			fprintf(stderr, "\t\ttag: %02x (len %u)\n", tag, desc_get_length(desc));
			break;
		case 0x7d:	// XAIT location descriptor
			// FIXME
			fprintf(stderr, "\t\ttag: %02x (len %u)\n", tag, desc_get_length(desc));
			break;
		case 0x7e:	// FTA content management descriptor
			// FIXME
			fprintf(stderr, "\t\ttag: %02x (len %u)\n", tag, desc_get_length(desc));
			break;
		case 0x7f:	// Extension descriptor
			// FIXME
			fprintf(stderr, "\t\ttag: %02x (len %u)\n", tag, desc_get_length(desc));
			break;
		default:
			fprintf(stderr, "\t\ttag: %02x (len %u)\n", tag, desc_get_length(desc));
			break;
		}
	}
	if (ext_description && ext_description_node) {
		mxmlNewText(ext_description_node, 0, ext_description);
		free(ext_description);
	}
}

static void dump_xmltv_section(struct pnr_s *program, uint8_t *eit, mxml_node_t *tv)
{
	char time_buf[21];
	mxml_node_t *programme;
	uint8_t *event;
	uint64_t start_ts;
	int      duration, hour, min, sec;
	int i;

	for (i = 0; (event = eit_get_event(eit, i)) != NULL; i++) {
		programme = mxmlNewElement(tv, "programme");
		mxmlElementSetAttrf(programme, "channel", "pnr%u.ts%u.network%d.dvb", program->pnr, program->ts_id, program->network_id);

		start_ts = dvb_time_decode_UTC(eitn_get_start_time(event));
		mxmlElementSetAttrf(programme, "start", "%s", xmltv_timestamp(start_ts, time_buf));

		dvb_time_decode_bcd(eitn_get_duration_bcd(event), &duration, &hour, &min, &sec);
		mxmlElementSetAttrf(programme, "stop", "%s", xmltv_timestamp(start_ts + duration, time_buf));

//		fprintf(stderr, "\tEvent: (%s - ", xmltv_timestamp(start_ts, time_buf));
//		fprintf(stderr, "%s)\n", xmltv_timestamp(start_ts + duration, time_buf));
		dump_xmltv_descriptors(eitn_get_descs(event), programme);
	}
}

static void dump_xmltv_program(struct pnr_s *program, mxml_node_t *tv)
{
	int t, s;
	size_t table_n = sizeof(program->section)/sizeof(program->section[0]);
	size_t section_n = sizeof(program->section[0])/sizeof(program->section[0][0]);

	for (t = 0; t < table_n; t++) {
		for (s = 0; s < section_n; s++) {
			if (program->section[t][s]) {
//				fprintf(stderr, "Section 0x%04x %d/%d/%d\n", program->pnr, t, s/8, s%8);
				dump_xmltv_section(program, program->section[t][s], tv);
			}
		}
	}
}

static void dump_eit(struct epgdump_s *epgd, mxml_node_t *tv)
{
	int i;

	if (!psi_table_validate(epgd->sdt.sdt_current) || !sdt_table_validate(epgd->sdt.sdt_current))
		return;

	for (i = 0; i < epgd->eit.program_n; i++)
		dump_xmltv_program(&epgd->eit.program[i], tv);
}

static void dump_sdt_section(uint8_t *sdt_section, mxml_node_t *tv)
{
	int      i;
	int      j;
	uint8_t *service_ptr;
	uint8_t *descs;
	uint8_t *desc;
	uint16_t service_id;
	uint16_t ts_id;
	uint16_t network_id;

	char *string;
	const uint8_t *str;
	uint8_t  str_len;

	mxml_node_t *node;

	ts_id = sdt_get_tsid(sdt_section);
	network_id = sdt_get_onid(sdt_section);
	
	for (i = 0; (service_ptr = sdt_get_service(sdt_section, i)); i++) {
		service_id = sdtn_get_sid(service_ptr);

		node = mxmlNewElement(tv, "channel");
		mxmlElementSetAttrf(node, "id", "pnr%u.ts%u.network%u.dvb", service_id, ts_id, network_id);

		descs = sdtn_get_descs(service_ptr);
		for (j = 0; (desc = descs_get_desc(descs, j)) != NULL; j++) {
			if (desc_get_tag(desc) == 0x48)	{ // Service descriptor
				str = desc48_get_service(desc, &str_len);
				string = dvb_string_get(str, str_len, iconv_wrapper, NULL);

				node = mxmlNewElement(node, "display-name");
				mxmlNewText(node, 0, string);
				free(string);
				break;
			}
		}
	}

}

static void dump_sdt(struct epgdump_s *epgd, mxml_node_t *tv)
{
	unsigned int sec;
	unsigned int lastsec;

	if (epgd->sdt.sdt_current[0] == NULL)
		return;

	lastsec = psi_get_lastsection(epgd->sdt.sdt_current[0]);
	for (sec = 0 ; sec <= lastsec; sec++)
		dump_sdt_section(epgd->sdt.sdt_current[sec], tv);
}

static const char *whitespace_cb(mxml_node_t *node, int where)
{
	static int   counter = -1;
	static char  buf[32];
	int i;

	// One line elements
	if (node->child && node->child->type != MXML_ELEMENT)
		if (where == MXML_WS_AFTER_OPEN || where == MXML_WS_BEFORE_CLOSE)
			return "";


	if (where == MXML_WS_BEFORE_CLOSE)
		counter--;

	for (i = 0; i < counter; i++)
		buf[i] = '\t';
	buf[i] = '\0';

	if (where == MXML_WS_BEFORE_OPEN)
		return buf;

	// Single tag elements	
	if (node->child == NULL) {
			if (where == MXML_WS_AFTER_OPEN)
				return "\n";
			else
				return "";
	}


	if (where == MXML_WS_AFTER_OPEN)
		counter++;

	if (where == MXML_WS_AFTER_OPEN || where == MXML_WS_AFTER_CLOSE)
		return "\n";

	if (where == MXML_WS_BEFORE_CLOSE) {
		return buf;
	}

	return "";
}

static void attach_network_id(struct epgdump_s *epgd)
{
	int i;

	if (!epgd->sdt.complete)
		return;

	for (i = 0; i < epgd->eit.program_n; i++)
		if (epgd->eit.program[i].ts_id == epgd->sdt.current_ts_id)
			epgd->eit.program[i].network_id = epgd->sdt.original_network_id;
	
}

void output_xmltv(struct epgdump_s *epgd)
{
	mxml_node_t *tv;

	mxmlSetWrapMargin(0);

	epgd->xmltv = mxmlNewXML("1.0");
	if (!epgd->xmltv)
		critical_error("eit_pid_dump: error allocating memory\n");

	tv = mxmlNewElement(epgd->xmltv, "tv");

	attach_network_id(epgd);

	dump_sdt(epgd, tv);
	dump_eit(epgd, tv);

	mxmlSaveFile(epgd->xmltv, stdout, whitespace_cb);
	mxmlDelete(epgd->xmltv);
}

