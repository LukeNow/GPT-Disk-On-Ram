#include <linux/types.h>

#define DISK_SIZE 0x100000
#define LAST_USABLE_BLK DISK_SIZE-LBA_LEN 

#define MBR_ENTRY_LEN 16
#define LBA_LEN 512

#define MBR_ZERO_OFF 446
#define MBR_ENTRY_OFF 462
#define MBR_SIG_OFF 510
#define MBR_SIG_LEN 2

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
	u16	TypeName[36];
} gpt_table_entry;

static const mbr_table_entry prot_mbr_entry = 
{
	BootIndicator: 0x00,
	StartingCHS: 0x000200,
	OSType: 0xEE,
	EndingCHS: LAST_USABLE_BLK,
	StartingLBA: 0x01,
	SizeInLBA: //TODO

};



static void write_prot_mbr(u8 *disk) {
	memset(disk, 0x0, MBR_ZERO_OFF); //Set rest to zero
	memset(disk + MBR_ZERO_OFF, &prot_mbr_entry, 
	       MBR_ENTRY_LEN); //Fill one valid entry
	memset(disk + MBR_ENTRY_OFF, 0x00, 
	       MBR_ENTRY_LEN * 3); //Zero fill 3 entries
	memset(disk + MBR_SIG_OFF, 0x00, 
	       MBR_SIG_LEN); //write signature       
}

static void write_gpt(u8 *disk) {

}

void write_headers(u8 *disk){

}
