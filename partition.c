#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/crc32.h>
#include <linux/string.h>
#include <linux/module.h>
#include "partition.h"

#define CRC_32_SEED 0xffffffff

#define DISK_SIZE 0x100000
#define LAST_USABLE_BLK DISK_SIZE - (34 * LBA_LEN)
#define NUM_OF_LBA (DISK_SIZE / LBA_LEN)

#define GPT_ENTRY_LEN 0x80
#define GPT_TBL_LEN 92
#define MBR_SIG_LEN 2
#define MBR_TABLE_LEN 512
#define MBR_ENTRY_LEN 16
#define LBA_LEN 512
#define GPT_FIRST_ENTRY_OFF (LBA_LEN * 2)

#define MBR_ZERO_OFF 446
#define MBR_ENTRY_OFF 462
#define MBR_SIG_OFF 510

#define GPT_TBL_OFF LBA_LEN
#define GPT_TBL_ZERO_OFF (GPT_TBL_OFF + GPT_TBL_LEN)
#define GPT_TBL_ZERO_LEN (GPT_FIRST_ENTRY_OFF - GPT_TBL_ZERO_OFF)

#define GPT_ENTRY_ZERO_OFF (GPT_FIRST_ENTRY_OFF + GPT_ENTRY_LEN)
#define GPT_ENTRY_ZERO_LEN (START_PART_OFF - GPT_ENTRY_ZERO_OFF)
#define START_PART_OFF (34 * LBA_LEN)

#define GPT_BACKUP_ENTRY_OFF (DISK_SIZE - (NUM_OF_LBA - 34) * LBA_LEN)
#define GPT_BACKUP_ENTRY_ZERO_OFF (GPT_BACKUP_ENTRY_OFF + GPT_ENTRY_LEN)
#define GPT_BACKUP_HEADER_OFF ((NUM_OF_LBA - 1) * LBA_LEN)
#define GPT_BACKUP_HEADER_ZERO_OFF (GPT_BACKUP_HEADER_OFF + GPT_TBL_LEN)


typedef struct {
	char	BootIndicator;
	char	StartingCHS[3];
	char	OSType;
	char	EndingCHS[3];
	u32	StartingLBA;
	u32	SizeInLBA;
} mbr_table_entry;


typedef struct {
	char	Signature[8];
	u32	Revision;
	u32	HeaderSize;
	u32	HeaderCRC32;
	u32	Reserved;
	u64	PrimaryLBA;
	u64	BackupLBA;
	u64	FirstUsableLBA;
	u64	LastUsableLBA;
	u64	UniqueGUID1;
	u64	UniqueGUID2;
	u64	FirstEntryLBA;
	u32	SizeOfEntry;
	u32	EntriesCRC32;
} gpt_table_header;

typedef struct {
	u64	TypeGUID1;
	u64	TypeGUID2;
	u64	UniqueGUID1;
	u64	UniqueGUID2;
	u64	StartingLBA;
	u64	EndingLBA;
	u64	Attributes;
	char	TypeName[72];
} gpt_table_entry;

static const mbr_table_entry prot_mbr_entry = 
{
	BootIndicator: 0x00,
	StartingCHS: { 0x00, 0x02, 0x00 } ,
	OSType: 0xEE,
	EndingCHS: { 0xFE, 0xFF, 0xFF }, //TODO
	StartingLBA: 0x01, 
	SizeInLBA: 0x07CF //Total LBA size - 1

};

static const gpt_table_entry gpt_entry = {
	TypeGUID1: 0xC12A7328F81F11D2,
	TypeGUID2: 0xBA4B00A0C93EC93B,
	UniqueGUID1: 0x1000000000000000,
	UniqueGUID2: 0x1000000000000000,
	StartingLBA: 0x20,
	EndingLBA: 0x03C6, 
	Attributes: 0x00,
	TypeName: "Test"
};



static gpt_table_header gpt_header = {
	Signature: "EFI PART",
	Revision: 0x00010000,
	HeaderSize: 0x5D,
	HeaderCRC32: 0x00, //TO be calculated second
	Reserved: 0x00,
	PrimaryLBA: 0x01,
	BackupLBA: 0x07CF, //CHECK
	FirstUsableLBA: 0x20,
	LastUsableLBA: 0x03C6, //966 
	UniqueGUID1: 0x1000000000000000,
	UniqueGUID2: 0x1000000000000000,
	FirstEntryLBA: 0x02,
	SizeOfEntry: 0x80,
	EntriesCRC32: 0x00 //TO be calculated first
};


static void write_prot_mbr(u8 *disk) {
	memset(disk, 0x0, MBR_ZERO_OFF); //Set first part of MBR to zero
	memcpy(disk + MBR_ZERO_OFF, &prot_mbr_entry, 
	       MBR_ENTRY_LEN); //Fill one valid entry
	memset(disk + MBR_ENTRY_OFF, 0x00, 
	       MBR_ENTRY_LEN * 3); //Zero fill 3 entries
	
	*(unsigned short *)(disk + MBR_SIG_OFF) = 0xAA55;
	//memset(disk + MBR_TABLE_LEN, 0x00, 
	       //LBA_LEN - MBR_TABLE_LEN); //Zero fill rest of MBR to fit LBA len

	printk(KERN_INFO "MBR header written\n");
}

static void write_gpt(u8 *disk) {
	u32 crc = crc32(CRC_32_SEED , &gpt_entry, sizeof(gpt_table_entry)) 
		^ CRC_32_SEED; // Xor at the end with 0xFF--
	gpt_header.EntriesCRC32 = crc;
	
	crc = crc32(CRC_32_SEED, &gpt_header, sizeof(gpt_table_header))
		^ CRC_32_SEED;
	gpt_header.HeaderCRC32 = crc;
	
	printk(KERN_INFO "The GPT_Table len is %d\n", gpt_header.HeaderSize);
	
	/*WRITE FIRST MAIN GPT HEADER */
	memcpy(disk + GPT_TBL_OFF, &gpt_header, sizeof(gpt_table_header)); //write first gpt header
	memset(disk + GPT_TBL_ZERO_OFF, 0x00, GPT_TBL_ZERO_LEN);

	memcpy(disk + GPT_FIRST_ENTRY_OFF, &gpt_entry, sizeof(gpt_table_entry));
	memset(disk + GPT_ENTRY_ZERO_OFF, 0x00, GPT_ENTRY_ZERO_LEN);

	/*WRITE BACKUP GPT HEADER */
	memcpy(disk + GPT_BACKUP_ENTRY_OFF, &gpt_entry, sizeof(gpt_table_entry));
	memset(disk + GPT_BACKUP_ENTRY_ZERO_OFF, 0x00, GPT_ENTRY_ZERO_LEN);

	memcpy(disk + GPT_BACKUP_HEADER_OFF, &gpt_header, sizeof(gpt_table_header));
	memset(disk + GPT_BACKUP_HEADER_ZERO_OFF, 0x00, GPT_TBL_ZERO_LEN);

	printk(KERN_INFO "GPT headers written\n");
}

void write_headers(u8 *disk){
	write_prot_mbr(disk);
	write_gpt(disk);
}


MODULE_LICENSE("GPL");


