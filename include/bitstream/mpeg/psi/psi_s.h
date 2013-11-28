#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

/*****************************************************************************
 * Static PSI section gathering
 *****************************************************************************/
static inline void psi_assemble_init_s(uint8_t *psi_buffer,
                                     uint16_t *psi_buffer_used)
{
    *psi_buffer_used = 0;
}

static inline void psi_assemble_reset_s(uint8_t *pp_psi_buffer,
                                      uint16_t *pi_psi_buffer_used)
{
    psi_assemble_init_s(pp_psi_buffer, pi_psi_buffer_used);
}

static inline bool psi_assemble_empty_s(uint8_t *pp_psi_buffer,
		                               uint16_t *pi_psi_buffer_used)
{
	return *pi_psi_buffer_used == 0;
}

static inline uint8_t *psi_assemble_payload_s(uint8_t *psi_buffer,
                                            uint16_t *psi_buffer_used,
                                            const uint8_t **payload,
                                            uint8_t *payload_len)
{
    uint16_t  i_remaining_size;
    uint16_t  i_copy_size;
    uint16_t  section_len;
    uint8_t  *full_section;

    i_remaining_size = PSI_PRIVATE_MAX_SIZE + PSI_HEADER_SIZE - *psi_buffer_used;
    i_copy_size = *payload_len < i_remaining_size ? *payload_len : i_remaining_size;
	full_section = NULL;

    if (*psi_buffer_used == 0) {
        if (**payload == 0xff) {
            /* padding table to the end of buffer */
            *payload_len = 0;
            return NULL;
        }
    }

    memcpy(psi_buffer + *psi_buffer_used, *payload, i_copy_size);
    *psi_buffer_used += i_copy_size;

    if (*psi_buffer_used >= PSI_HEADER_SIZE) {
        section_len = psi_get_length(psi_buffer) + PSI_HEADER_SIZE;

        if (section_len > PSI_PRIVATE_MAX_SIZE) {
            /* invalid section */
            psi_assemble_reset_s(psi_buffer, psi_buffer_used);
            *payload_len = 0;
            return NULL;
        }
        if (section_len <= *psi_buffer_used) {
            full_section = psi_buffer;
            i_copy_size -= (*psi_buffer_used - section_len);
            *psi_buffer_used = 0;
        }
    }

    *payload += i_copy_size;
    *payload_len -= i_copy_size;
    return full_section;
}

/*****************************************************************************
 * Static PSI table gathering
 *****************************************************************************/

static inline bool psi_table_section_s(uint8_t table[PSI_TABLE_MAX_SECTIONS][PSI_MAX_SIZE + PSI_HEADER_SIZE], uint8_t *section)
{
    uint8_t  section_n = psi_get_section(section);
    uint8_t  last_section = psi_get_lastsection(section);
    uint8_t  version = psi_get_version(section);
    uint16_t ts_id = psi_get_tableidext(section);
    int      section_len;
    int      i;

    section_len = psi_get_length(section) + PSI_HEADER_SIZE;
    if (!memcmp(table[section_n], section, section_len))
    	return false;

    //bzero(table[section_n], PSI_MAX_SIZE + PSI_HEADER_SIZE);
    memcpy(table[section_n], section, section_len);

    for (i = 0; i <= last_section; i++) {
        uint8_t *p = table[i];
        if (psi_get_lastsection(p) != last_section
             || psi_get_version(p) != version
             || psi_get_tableidext(p) != ts_id)
            return false;
    }

    /* a new, full table is available */
    return true;
}

#ifdef __cplusplus
}
#endif

