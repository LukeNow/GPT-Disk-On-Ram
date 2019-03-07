#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/crc32.h>
#include <linux/string.h>
#include <linux/module.h>
#include <linux/uuid.h>
#include <linux/swab.h>
#include <asm/byteorder.h>
#include <linux/slab.h>
#include "partition.h"

#define DEBUG 0

#define CRC_32_SEED 0xffffffff
#define PROT_MBR_SIG 0xAA55
#define GPT_PMBR_OSTYPE  0xEE
#define GPT_HEADER_SIGNATURE 0x5452415020494645LL /* EFI PART */
#define GPT_HEADER_REVISION	0x00010000

#define GPT_DEFAULT_ENTRY_TYPE "0FC63DAF-8483-4772-8E79-3D69D8477DE4"

#define GPT_PART_NAME_LEN (72 / sizeof(u16))
#define PART_NUM 128
#define SECTOR_SIZE 512
#define LBA_LEN SECTOR_SIZE
#define NUM_OF_LBA (DISK_SIZE / LBA_LEN)

#define PART_ARRAY_SIZE (PART_NUM * sizeof(gpt_entry))
#define PART_ARRAY_SZ_LBA (PART_ARRAY_SIZE / SECTOR_SIZE)
#define PART_ENTRY_LBA_SZ (sizeof(gpt_entry) * PART_NUM / SECTOR_SIZE)
#define FIRST_ENTRY_LBA (PART_ENTRY_LBA_SZ + 2)
#define LAST_ENTRY_LBA (NUM_OF_LBA - PART_ENTRY_LBA_SZ - 2)

#define PRIMARY_GPT_HEADER_OFF (1 * SECTOR_SIZE)
#define PRIMARY_PART_ARRAY_OFF (2 * SECTOR_SIZE)
#define SEC_GPT_HEADER_OFF ((NUM_OF_LBA - 1) * SECTOR_SIZE)
#define SEC_GPT_PART_ARRAY_OFF ((NUM_OF_LBA - 1 - PART_ARRAY_SZ_LBA) * SECTOR_SIZE)

/* Globally unique identifier */
struct gpt_guid {
	u32	time_low;
	u16	time_mid;
	u16	time_hi_and_version;
	u8	node[8];
}__packed;

/* The GPT Partition entry array contains an array of GPT entries. */
typedef struct gpt_entry {
	struct gpt_guid	type; /* purpose and type of the partition */
	struct gpt_guid	partition_guid;
	u64		lba_start;
	u64		lba_end;
	u64		attrs;
	u16		name[GPT_PART_NAME_LEN];
} __packed gpt_entry;

/* GPT header */
typedef struct gpt_header {
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
}__packed gpt_header;

typedef struct gpt_legacy_entry {
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
}__packed gpt_legacy_entry;

/* Protected MBR and legacy MBR share same structure */
typedef struct gpt_legacy_mbr {
	u8             boot_code[440];
	u32            unique_mbr_signature;
	u16            unknown;
	gpt_legacy_entry   part_entry[4];
	u16            signature;
}__packed gpt_legacy_mbr;

static inline u32
efi_crc32(const void *buf, unsigned long len)
{
	return (crc32(~0L, buf, len) ^ ~0L);
}

#if DEBUG
static int is_pmbr_valid(gpt_legacy_mbr *mbr)
{
	int i, found = 0, signature = 0;
	if (!mbr)
		return 0;
	signature = (le16_to_cpu(mbr->signature) ==  PROT_MBR_SIG );
	for (i = 0; signature && i < 4; i++) {
		if (mbr->part_entry[i].os_type ==
		    GPT_PMBR_OSTYPE) {
			found = 1;
			break;
		}
	}
	return (signature && found);
}


static int is_gpt_valid(gpt_header *h, gpt_entry *ent)
{
	int rc = 0;
	u32 hdrsz, origcrc, crc;

	if (le64_to_cpu((h)->signature) != GPT_HEADER_SIGNATURE) {
		printk(KERN_ERR "INVALID HEADER SIG\n");
		return rc;
	}

	hdrsz = le32_to_cpu(h->size);
	origcrc = le32_to_cpu(h->crc32);
	h->crc32 = 0;
	crc = efi_crc32((const unsigned char *)(h), le32_to_cpu(h->size));

	printk(KERN_INFO "ORIG=%du, CRC=%du \n", origcrc, crc);
	if (crc != origcrc) {
		
		printk(KERN_ERR "INVALID HEADER CRC\n");
		return rc;
	}

	h->crc32 = origcrc;

	return 1;
}

static void test_headers(gpt_legacy_mbr *mbr, gpt_header *h, 
		gpt_entry *ent)
{
	int rc;
	rc = is_pmbr_valid(mbr);

	if (rc == 0) {
		printk(KERN_ERR "PMBR NOT VALID!!\n");
		return;
	}
	
	rc = is_gpt_valid(h, ent);
	if (rc == 0) {
		printk(KERN_ERR "GPT NOT VALID!!\n");
		return;
	}

	printk(KERN_INFO "TEST SUCCESS\n");
}
#endif

static void write_prot_mbr(gpt_legacy_mbr *pmbr) {
	pmbr->part_entry[0].os_type = GPT_PMBR_OSTYPE;
	pmbr->part_entry[0].start_sector = 1; 
	pmbr->part_entry[0].end_head = 0xFF;
	pmbr->part_entry[0].end_sector = 0xFF;
	pmbr->part_entry[0].end_track = 0xFF;
	pmbr->part_entry[0].starting_lba = cpu_to_le32(1);
	pmbr->part_entry[0].size_in_lba = cpu_to_le32(NUM_OF_LBA - 1ULL);
	pmbr->signature = cpu_to_le32(PROT_MBR_SIG);
}

static void write_primary_gpt_header(gpt_header *h) {
	
	struct gpt_guid guid;

	memset(&guid, 0, sizeof(struct gpt_guid));

	h->signature = cpu_to_le64(GPT_HEADER_SIGNATURE);
	h->revision = cpu_to_le32(GPT_HEADER_REVISION);
	h->size = cpu_to_le32(sizeof(gpt_header) - sizeof(h->reserved2));
	h->my_lba = cpu_to_le32(1);
	h->alternative_lba = cpu_to_le32(NUM_OF_LBA-1);
	h->partition_entry_lba = cpu_to_le64(2ULL);
	h->npartition_entries = cpu_to_le32(PART_NUM); 
	h->sizeof_partition_entry = cpu_to_le32(sizeof(gpt_entry));
	h->first_usable_lba = cpu_to_le64(FIRST_ENTRY_LBA);
	h->last_usable_lba = cpu_to_le64(LAST_ENTRY_LBA);
	/* Set GUID to 0xFF - */
	guid.time_low = ~0;
	memset(&guid.node, ~0, sizeof(guid.node));
	h->disk_guid = guid; 
	
}

static void write_secondary_gpt_header(gpt_header *h) 
{
	write_primary_gpt_header(h); //Everything the same
	h->my_lba = cpu_to_le32(NUM_OF_LBA-1); //Extept for my_lba and alt_lba
	h->alternative_lba = cpu_to_le32(1);
}


static void write_part_entries(gpt_entry *ents)
{
	gpt_entry e;
	uuid_t uuid;
	guid_t guid;
	
	uuid_parse(GPT_DEFAULT_ENTRY_TYPE, &uuid);
	uuid.b[0] = swab64(uuid.b[0]);
	uuid.b[8] = swab64(uuid.b[8]); //TODO
	
	
	memcpy(&e.type, &uuid, sizeof(uuid_t));
	guid_gen(&guid);
	memcpy(&e.partition_guid, &guid, sizeof(guid_t));
	
	e.lba_start = cpu_to_le32(FIRST_ENTRY_LBA);
	e.lba_end = cpu_to_le32(LAST_ENTRY_LBA);
	
	memcpy(ents, &e, sizeof(gpt_entry));
}

static void calculate_crc32(gpt_header *h, gpt_entry *ents)
{
	u32 crc;
	
	h->partition_entry_array_crc32 = 0;
	h->crc32 = 0;

	crc = efi_crc32(ents, PART_ARRAY_SIZE);
	h->partition_entry_array_crc32 = cpu_to_le32(crc);
	
	crc = efi_crc32(h, sizeof(gpt_header) - sizeof(h->reserved2));
	h->crc32 = cpu_to_le32(crc);
}

void write_headers_to_disk(u8 *disk){
	
	gpt_header *h;
	gpt_header *h2;
	gpt_legacy_mbr *pmbr;
	gpt_entry *ents;

	h = (gpt_header *)kmalloc(sizeof(gpt_header), GFP_KERNEL);
	h2 = (gpt_header *)kmalloc(sizeof(gpt_header), GFP_KERNEL);
	pmbr = (gpt_legacy_mbr *)kmalloc(sizeof(gpt_legacy_mbr), GFP_KERNEL);
	ents = (gpt_entry *)kmalloc(PART_ARRAY_SIZE, GFP_KERNEL);
	
	memset(h, 0, sizeof(gpt_header));
	memset(h2, 0, sizeof(gpt_header));
	memset(pmbr, 0, sizeof(gpt_legacy_mbr));
	memset(ents, 0, PART_ARRAY_SIZE);
	
	write_prot_mbr(pmbr);
	write_primary_gpt_header(h);
	write_secondary_gpt_header(h2);
	write_part_entries(ents);

	calculate_crc32(h, ents); 
	calculate_crc32(h2, ents); 
	
	memcpy(disk, pmbr, sizeof(gpt_legacy_mbr));
	memcpy(disk + PRIMARY_GPT_HEADER_OFF, h, sizeof(gpt_header));
	memcpy(disk + PRIMARY_PART_ARRAY_OFF, ents, PART_ARRAY_SIZE);
	memcpy(disk + SEC_GPT_HEADER_OFF, h2, sizeof(gpt_header));
	memcpy(disk + SEC_GPT_PART_ARRAY_OFF, ents, PART_ARRAY_SIZE);
	
#if DEBUG
	test_headers(pmbr, h, ents);
#endif

	kfree(h);
	kfree(h2);
	kfree(pmbr);
	kfree(ents);
}
