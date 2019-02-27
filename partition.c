#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/crc32.h>
#include <linux/string.h>
#include <linux/module.h>
#include <linux/uuid.h>
#include <linux/swab.h>
#include <asm/byteorder.h>

#include "partition.h"

#define CRC_32_SEED 0xffffffff
#define PROT_MBR_SIG 0xAA55
#define GPT_PMBR_OSTYPE  0xEE
#define GPT_HEADER_SIGNATURE 0x5452415020494645LL /* EFI PART */
#define GPT_HEADER_REVISION	0x00010000

#define GPT_DEFAULT_ENTRY_TYPE "0FC63DAF-8483-4772-8E79-3D69D8477DE4"

#define GPT_PART_NAME_LEN (72 / sizeof(u16))
#define PART_NUM 128
#define DISK_SIZE 0x100000
#define SECTOR_SIZE 512
#define LBA_LEN SECTOR_SIZE
#define LAST_USABLE_BLK DISK_SIZE - (34 * LBA_LEN)
#define NUM_OF_LBA (DISK_SIZE / LBA_LEN)

#define PART_ARRAY_SIZE (PART_NUM * SECTOR_SIZE * sizeof(struct gpt_entry))
#define PART_ARRAY_SZ_LBA (sizeof(struct gpt_header) * PART_NUM / SECTOR_SIZE)




#define PRIMARY_GPT_HEADER_OFF (1 * SECTOR_SIZE)
#define PRIMARY_PART_ARRAY_OFF (2 * SECTOR_SIZE)
#define SEC_GPT_HEADER_OFF ((NUM_OF_LBA - 1) * SECTOR_SIZE)
#define SEC_GPT_PART_ARRAY_OFF ((NUM_OF_LBA - 1 - PART_ARRAY_SZ_LBA) * SECTOR_SIZE)


/*
typedef struct {
	char	BootIndicator;
	char	StartingCHS[3];
	char	OSType;
	char	EndingCHS[3];
	u32	StartingLBA;
	u32	SizeInLBA;
} mbr_table_entry;
*/
/*
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
} gpt_table_entry; */
/* Globally unique identifier */
struct gpt_guid {
	u32	time_low;
	u16	time_mid;
	u16	time_hi_and_version;
	u8	node[8];
};
/* The GPT Partition entry array contains an array of GPT entries. */
struct gpt_entry {
	struct gpt_guid	type; /* purpose and type of the partition */
	struct gpt_guid	partition_guid;
	u64		lba_start;
	u64		lba_end;
	u64		attrs;
	u16		name[GPT_PART_NAME_LEN];
};

/* GPT header */
struct gpt_header {
	u64            signature; /* header identification */
	u32            revision; /* header version */
	u32            size; /* in bytes */
	u32            crc32; /* header CRC checksum */
	u32            reserved1; /* must be 0 */
	u64            my_lba; /* LBA of block that contains this struct (LBA 1) */
	u64            alternative_lba; /* backup GPT header */
	u64            first_usable_lba; /* first usable logical block for partitions */
	u64            last_usable_lba; /* last usable logical block for partitions */
	struct gpt_guid     disk_guid; /* unique disk identifier */
	u64            partition_entry_lba; /* LBA of start of partition entries array */
	u32            npartition_entries; /* total partition entries - normally 128 */
	u32            sizeof_partition_entry; /* bytes for each GUID pt */
	u32            partition_entry_array_crc32; /* partition CRC checksum */
	u8             reserved2[512 - 92]; /* must all be 0 */
};

struct gpt_legacy_entry {
	u8             boot_indicator; /* unused by EFI, set to 0x80 for bootable */
	u8             start_head; /* unused by EFI, pt start in CHS */
	u8             start_sector; /* unused by EFI, pt start in CHS */
	u8             start_track;
	u8             os_type; /* EFI and legacy non-EFI OS types */
	u8             end_head; /* unused by EFI, pt end in CHS */
	u8             end_sector; /* unused by EFI, pt end in CHS */
	u8             end_track; /* unused by EFI, pt end in CHS */
	u32            starting_lba; /* used by EFI - start addr of the on disk pt */
	u32            size_in_lba; /* used by EFI - size of pt in LBA */
};

/* Protected MBR and legacy MBR share same structure */
struct gpt_legacy_mbr {
	u8             boot_code[440];
	u32            unique_mbr_signature;
	u16            unknown;
	struct gpt_legacy_entry   part_entry[4];
	u16            signature;
};

/*
static const mbr_table_entry prot_mbr_entry = 
{
	BootIndicator: 0x00,
	StartingCHS: { 0x00, 0x02, 0x00 } ,
	OSType: 0xEE,
	EndingCHS: 0xFFFFFF, 
	StartingLBA: 0x00000001, 
	SizeInLBA: (NUM_OF_LBA - 1)//Total LBA size - 1

};
*/
/*
static const gpt_table_entry gpt_entry = {
	TypeGUID1: 0xC12A7328F81F11D2,
	TypeGUID2: 0xBA4B00A0C93EC93B,
	UniqueGUID1: 0x1000000000000000,
	UniqueGUID2: 0x1000000000000000,
	StartingLBA: 0x20,
	EndingLBA: 0x03C6, 
	Attributes: 0x00,
	TypeName: "Test"
}; */


/*
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
};*/

/* prints UUID in the real byte order! */
static void gpt_debug_uuid(uuid_t *guid)
{
	const unsigned char *uuid = (unsigned char *) guid;

	printk(KERN_INFO
		"%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x\n",
		uuid[0], uuid[1], uuid[2], uuid[3],
		uuid[4], uuid[5],
		uuid[6], uuid[7],
		uuid[8], uuid[9],
		uuid[10], uuid[11], uuid[12], uuid[13], uuid[14],uuid[15]);
}

static void write_prot_mbr(struct gpt_legacy_mbr *pmbr) {
	pmbr->part_entry[0].os_type = GPT_PMBR_OSTYPE;
	pmbr->part_entry[0].start_sector = 2;
	pmbr->part_entry[0].end_head = 0xFF;
	pmbr->part_entry[0].end_sector = 0xFF;
	pmbr->part_entry[0].end_track = 0xFF;
	pmbr->part_entry[0].starting_lba = cpu_to_le32(1);
	pmbr->part_entry[0].size_in_lba = cpu_to_le32(NUM_OF_LBA - 1ULL);
	
}

static void write_gpt_header(struct gpt_header *h) {
	
	struct gpt_guid guid;

	u64 entries_lba_size = sizeof(struct gpt_header) * PART_NUM / SECTOR_SIZE;
	u64 first_entry_lba = entries_lba_size + 2;
	u64 last_entry_lba = NUM_OF_LBA - entries_lba_size - 2;
	
	memset(&guid, 0, sizeof(struct gpt_guid));

	h->signature = cpu_to_le64(GPT_HEADER_SIGNATURE);
	h->revision = cpu_to_le32(GPT_HEADER_REVISION);
	h->size = cpu_to_le32(sizeof(struct gpt_header) - sizeof(h->reserved2));
	h->my_lba = cpu_to_le32(1);
	h->alternative_lba = cpu_to_le32(NUM_OF_LBA-1);
	h->partition_entry_lba = cpu_to_le64(2ULL);
	h->npartition_entries = cpu_to_le32(PART_NUM);
	h->sizeof_partition_entry = cpu_to_le32(sizeof(struct gpt_entry));
	h->first_usable_lba = first_entry_lba;
	h->last_usable_lba = last_entry_lba;

	/* Set GUID to 0xFF - */
	guid.time_low = ~0UL;
	memset(&guid.node, ~0UL, sizeof(guid.node));
	h->disk_guid = guid; //LE?
	
}

static void write_part_entries(struct gpt_entry *e)
{
	
	//struct gpt_guid g;
	uuid_t uuid;
	guid_t guid;

	uuid_parse(GPT_DEFAULT_ENTRY_TYPE, &uuid);
	uuid.b[0] = swab64(uuid.b[0]);
	uuid.b[8] = swab64(uuid.b[8]); //TODO
	
	printk( KERN_INFO "THE STRING UUID IS %s\n" GPT_DEFAULT_ENTRY_TYPE);
	gpt_debug_uuid(&uuid);
	
	/*
	g.time_low = (gpt_guid)uuid.time_low;
	g.time_mid = (gpt_guid)uuid.time_mid;
	g.time_hi_and_version = (gpt_guid)uuid.time_hi_and_version;
	g.node = (gpt_guid)uuid.node;
	*/
	memcpy(&e->type, &uuid, sizeof(uuid_t));
	guid_gen(&guid);
	memcpy(&e->partition_guid, &guid, sizeof(guid_t));
}

static void calculate_crc32(struct gpt_header *h, struct gpt_entry *e)
{
	u32 crc = crc32(CRC_32_SEED , e, sizeof(struct gpt_entry)) 
		^ CRC_32_SEED; // Xor at the end with 0xFF--
	h->partition_entry_array_crc32 = cpu_to_le32(crc);
	
	crc = crc32(CRC_32_SEED, h, sizeof(struct gpt_header))
		^ CRC_32_SEED;
	h->crc32 = cpu_to_le32(crc);
}


void write_headers_to_disk(u8 *disk){
	
	struct gpt_header h;
	struct gpt_legacy_mbr pmbr;
	struct gpt_entry ent;

	memset(&h, 0, sizeof(struct gpt_header));
	memset(&pmbr, 0, sizeof(struct gpt_legacy_mbr));
	memset(&ent, 0, sizeof(struct gpt_entry));

	write_prot_mbr(&pmbr);
	write_gpt_header(&h);
	write_part_entries(&ent);

	calculate_crc32(&h, &ent);

	memcpy(disk, &pmbr, sizeof(struct gpt_legacy_mbr));
	memcpy(disk + PRIMARY_GPT_HEADER_OFF, &h, sizeof(struct gpt_header));
	memcpy(disk + SEC_GPT_HEADER_OFF, &ent, sizeof(struct gpt_entry));
	memcpy(disk + SEC_GPT_PART_ARRAY_OFF, &ent, sizeof(struct gpt_entry));
	memcpy(disk + SEC_GPT_HEADER_OFF, &h, sizeof(struct gpt_header));

	printk(KERN_INFO "PRIMARY HEADER OFF %d\n", PRIMARY_GPT_HEADER_OFF);
	printk(KERN_INFO "PRIMARY PART_ARRAY OFF %llu\n", (unsigned long long)PRIMARY_PART_ARRAY_OFF);
	printk(KERN_INFO "SEC PART_ARRAY OFF %llu\n", (unsigned long long)SEC_GPT_PART_ARRAY_OFF);
	printk(KERN_INFO "SEC HEADER OFF  %llu\n", (unsigned long long)SEC_GPT_HEADER_OFF);
}


MODULE_LICENSE("GPL");

