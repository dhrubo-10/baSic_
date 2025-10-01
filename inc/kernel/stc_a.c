#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "string.h"
#include "stc_a.h"

/* IRQ mappings */
#define STC_A_IRQ0    (32 + 14)
#define STC_A_IRQ1    (32 + 15)
#define STC_A_IRQ2    (32 + 11)
#define STC_A_IRQ3    (32 + 9)

/* I/O Base Ports */
#define STC_A_BASE0   0x1F0
#define STC_A_BASE1   0x170
#define STC_A_BASE2   0x1E8
#define STC_A_BASE3   0x168

/* Timeouts */
#define STC_A_TIMEOUT          5000
#define STC_A_IDENTIFY_TIMEOUT 1000

/* Register Offsets */
#define STC_A_DATA    0
#define STC_A_ERROR   1
#define STC_A_COUNT   2
#define STC_A_SECTOR  3
#define STC_A_CYL_LO  4
#define STC_A_CYL_HI  5
#define STC_A_FDH     6
#define STC_A_STATUS  7
#define STC_A_COMMAND 7
#define STC_A_CONTROL 0x206

/* Flags */
#define STC_A_FLAGS_ECC 0x80
#define STC_A_FLAGS_LBA 0x40
#define STC_A_FLAGS_SEC 0x20
#define STC_A_FLAGS_SLV 0x10

/* Status bits */
#define STC_A_STATUS_BSY 0x80
#define STC_A_STATUS_RDY 0x40
#define STC_A_STATUS_WF  0x20
#define STC_A_STATUS_SC  0x10
#define STC_A_STATUS_DRQ 0x08
#define STC_A_STATUS_ERR 0x01

/* Commands */
#define STC_A_CMD_READ     0x20
#define STC_A_CMD_WRITE    0x30
#define STC_A_CMD_IDENTIFY 0xEC

/* Example structure for a drive */
typedef struct {
    uint16_t base;
    uint16_t control;
    uint8_t slave;
    char model[41];
    int present;
} stc_a_drive_t;

static stc_a_drive_t stc_a_drives[4];

/* Mock functions simulating port I/O (in real OS you'd use inb/outb) */
uint8_t inb(uint16_t port) {
    /* Simulate read from port */
    return 0;
}

void outb(uint16_t port, uint8_t val) {
    /* Simulate write to port */
    (void)port;
    (void)val;
}

/* Identify a drive */
int stc_a_identify(stc_a_drive_t *drive) {
    printf("Identifying STC_A drive at base 0x%X...\n", drive->base);
    strncpy(drive->model, "STC_A_SIMULATED_DRIVE", sizeof(drive->model) - 1);
    drive->present = 1;
    return 0;
}

/* Initialize all drives */
void stc_a_init() {
    memset(stc_a_drives, 0, sizeof(stc_a_drives));

    stc_a_drives[0].base = STC_A_BASE0;
    stc_a_drives[0].control = STC_A_BASE0 + STC_A_CONTROL;
    stc_a_drives[0].slave = 0;

    stc_a_drives[1].base = STC_A_BASE1;
    stc_a_drives[1].control = STC_A_BASE1 + STC_A_CONTROL;
    stc_a_drives[1].slave = 0;

    for (int i = 0; i < 2; i++) {
        stc_a_identify(&stc_a_drives[i]);
    }
}

/* Read a sector (simulation) */
int stc_a_read_sector(stc_a_drive_t *drive, uint32_t lba, uint8_t *buffer) {
    if (!drive->present) {
        printf("Drive not present!\n");
        return -1;
    }
    printf("Reading sector %u from drive %s...\n", lba, drive->model);
    memset(buffer, 0, 512); // fill with zeros for simulation
    return 0;
}

/* Write a sector (simulation) */
int stc_a_write_sector(stc_a_drive_t *drive, uint32_t lba, const uint8_t *buffer) {
    if (!drive->present) {
        printf("Drive not present!\n");
        return -1;
    }
    printf("Writing sector %u to drive %s...\n", lba, drive->model);
    (void)buffer; // no real write in simulation
    return 0;
}

/* Demo */
int main() {
    stc_a_init();

    uint8_t buffer[512];
    stc_a_read_sector(&stc_a_drives[0], 0, buffer);
    stc_a_write_sector(&stc_a_drives[0], 1, buffer);

    return 0;
}
