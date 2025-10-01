#ifndef _STC_A_H_
#define _STC_A_H_

#include <stdint.h>

/* ----- STC_A DRIVE STRUCTURE ----- */
typedef struct {
    uint16_t base;        /* Base I/O port */
    uint16_t control;     /* Control port */
    uint8_t slave;        /* 0 = master, 1 = slave */
    char model[41];       /* Drive model string */
    int present;          /* Drive presence flag */
} stc_a_drive_t;

/* ----- FUNCTION PROTOTYPES ----- */

/* Initialize STC_A controller and detect drives */
void stc_a_init();

/* Identify a given drive */
int stc_a_identify(stc_a_drive_t *drive);

/* Read one 512-byte sector into buffer */
int stc_a_read_sector(stc_a_drive_t *drive, uint32_t lba, uint8_t *buffer);

/* Write one 512-byte sector from buffer */
int stc_a_write_sector(stc_a_drive_t *drive, uint32_t lba, const uint8_t *buffer);

#endif /* _STC_A_H_ */
