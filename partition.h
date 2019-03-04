#ifndef PARITION_H
#define PARITION_H

#include <linux/types.h>

#define DISK_SIZE 0x200000

extern void write_headers_to_disk(u8 *disk);



#endif
