typedef struct {
	char x00_Signature[8];
	uint32	x08_Revision;
	uint32	x0C_HeaderSize;
	uint32	x10_HeaderCRC32;
	uint32	x14_Reserved;
	uint64	x18_PrimaryLBA;
	uint64	x20_BackupLBA;
	uint64	x28_FirstUsableLBA;
	uint64	x30_LastUsableLBA;
	uint64	x38_UniqueGUID1;
	uint64	x40_UniqueGUID2;
	uint64	x48_FirstEntryLBA;
	uint32	x54_SizeOfEntry;
	uint32	x58_EntriesCRC32;
} table_header;

typedef struct {
	uint64	x00_TypeGUID1;
	uint64	x08_TypeGUID2;
	uint64	x10_UniqueGUID1;
	uint64	x18_UniqueGUID2;
	uint64	x20_StartingLBA;
	uint64	x28_EndingLBA;
	uint64	x30_Attributes;
	uint16	x38_TypeName[36];
} table_entry;


