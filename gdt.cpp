/*
+--------------------------------------------------------------------------------------------------------------------------------+
|                                               Global Descriptor Table Entry (8 Bytes)                                          |
+--------------------------------------------------------------------------------------------------------------------------------+
| Byte 7       | Byte 6             | Byte 5          | Byte 4       | Byte 3       | Byte 2       | Byte 1       | Byte 0       |
+--------------+--------------------+-----------------+--------------+--------------+--------------+--------------+--------------+
| Base [31-24] | Flags [G,D,L,AVL]  | Access          | Base [23-16] | Base [15-08] | Base [07-00] | Limit [15-08]| Limit [07-00]|
|              | Limit [19-16]      | (P,DPL,S,E,RW,A)|              |              |              |              |              |
+--------------+--------------------+-----------------+--------------+--------------+--------------+--------------+--------------+

+---------------------------------------------------------------------------------------------------------------+
|                             Global Descriptor Table (GDT) - 8 Byte Segment Descriptor                         |
+---------------------------------------------------------------------------------------------------------------+
| Byte 7                | Byte 6                | Byte 5                                | Byte 4                |
+-----------------------+-----------------------+---------------------------------------+-----------------------+
| Base Address [31-24]  | G | D | L | AVL | Lim | Type / Access Byte (P|DPL|S|E|R/W|A)  | Base Address [23-16]  |
|                       | (Granularity)         |                                       |                       |
+-----------------------+-----------------------+---------------------------------------+-----------------------+
   7 6 5 4 3 2 1 0         7 6 5 4 3 2 1 0              7 6 5 4 3 2 1 0                     7 6 5 4 3 2 1 0
+-----------------------+-----------------------+---------------------------------------+-----------------------+
| Byte 3                | Byte 2                | Byte 1                                | Byte 0                |
+-----------------------+-----------------------+---------------------------------------+-----------------------+
| Base Address [15-08]  | Base Address [07-00]  | Limit [15-08]                         | Limit [07-00]         |
+-----------------------+-----------------------+---------------------------------------+-----------------------+
   7 6 5 4 3 2 1 0         7 6 5 4 3 2 1 0              7 6 5 4 3 2 1 0                     7 6 5 4 3 2 1 0

- Base Address (32-bit): The starting memory address of the segment. It's split across bytes 2, 3, 4, and 7.
- Segment Limit (20-bit): The maximum offset within the segment. It's split across bytes 0, 1, and 6 (specifically, bits 0-3 of byte 6).
- Flags (Byte 6, bits 4-7):
      + G (Granularity): 0 = byte granularity, 1 = 4KB page granularity.
      + D/B (Default Operation Size / Big): 0 = 16-bit segment, 1 = 32-bit segment (or 16-bit stack segment for data).
      + L (Long Mode): 0 = compatibility mode, 1 = 64-bit mode (for 64-bit code segments).
      + AVL (Available for use by system software): Unused by CPU.
- Access Byte / Type (Byte 5):
      + P (Present): 0 = segment not present, 1 = segment present.
      + DPL (Descriptor Privilege Level): 00 (kernel) - 11 (user).
      + S (System Segment): 0 = system segment, 1 = code/data segment.
      + E (Executable): 0 = data segment, 1 = code segment.
      + C/DC (Conforming / Direction/Conforming): For code segments (C): 0 = non-conforming, 1 = conforming. For data segments (DC): 0 = grows up, 1 = grows down.
      + R/W (Readable/Writable): For code segments (R): 0 = non-readable, 1 = readable. For data segments (W): 0 = non-writable, 1 = writable.
      + A (Accessed): Set by CPU when segment is accessed.
*/

#include "gdt.h"
#include "types.h"
#include "interrupts.h"
#include "port.h"


GlobalDescriptorTable::GlobalDescriptorTable()
    : nullSegmentSelector(0, 0 ,0), // Base, Limit, Flags
      unusedSegmentSelector(0, 0, 0), // Base, Limit, Flags
      codeSegmentSelector(0, 64*1024*1024, 0x9A), // Base, Limit, Code Segment Flags (0x9A)
      dataSegmentSelector(0, 64*1024*1024, 0x92) // Base, Limit, Data Segment Flags (0x92)
{
    uint32_t i[2];
    i[1] = (uint32_t)this; // first byte for address of table itself
    i[0] = sizeof(GlobalDescriptorTable) << 16; // fisrt 4 bytes are high byte of second integer
    asm volatile("lgdt (%0)": :"p" (((uint8_t *) i)+2));

    /*
    GDTR structure: [limit (16 bits) | base (32 bits)] (total 6 bytes)
    uint16_t size = sizeof(GlobalDescriptorTable) - 1;
    uint32_t address = (uint32_t)this;

    // GDTR pointer structure (a temporary stack variable)
    uint8_t gdtr[6];

    // GDTR Limit (size - 1)
    gdtr[0] = size & 0xFF;
    gdtr[1] = (size >> 8) & 0xFF;

    // GDTR Base Address
    gdtr[2] = address & 0xFF;
    gdtr[3] = (address >> 8) & 0xFF;
    gdtr[4] = (address >> 16) & 0xFF;
    gdtr[5] = (address >> 24) & 0xFF;

    // Load the GDTR register
    asm volatile("lgdt (%0)": :"p" (gdtr));
    */

}

GlobalDescriptorTable::~GlobalDescriptorTable() {
}

uint16_t GlobalDescriptorTable::DataSegmentSelector() {
    return (uint8_t*)&dataSegmentSelector - (uint8_t*)this;
}

uint16_t GlobalDescriptorTable::CodeSegmentSelector() {
    return (uint8_t*)&codeSegmentSelector - (uint8_t*)this;
}

GlobalDescriptorTable::SegmentDescriptor::SegmentDescriptor(uint32_t base, uint32_t limit, uint8_t type) {

    uint8_t* target = (uint8_t*)this;

    // Handle 16-bit vs 32-bit size and granularity
    if (limit <= 65536) {
        // 16-bit segment, byte granularity
        target[6] = 0x40;
    }
    else {
        // 32-bit segment, page granularity
        if ((limit & 0xFFF) != 0xFFF) {
            limit = (limit >> 12) - 1;
        }
        else {
            limit = limit >> 12;
        }
    target[6] = 0xC0; // Set D/B (32-bit default) and G (4K granularity)
    }

    // Set limit bytes
    target[0] = limit & 0xFF;
    target[1] = (limit >> 8) & 0xFF;
    target[6] |= (limit >> 16) & 0xF;

    // Set base bytes
    target[2] = base & 0xFF;
    target[3] = (base >> 8) & 0xFF; 
    target[4] = (base >> 16) & 0xFF;
    target[7] = (base >> 24) & 0xFF;

    // Set type/access flags
    target[5] = type;
}

uint32_t GlobalDescriptorTable::SegmentDescriptor::Base() {

    uint8_t* target = (uint8_t*)this;

    uint32_t result = target[7];
    result = (result << 8) + target[4];
    result = (result << 8) + target[3];
    result = (result << 8) + target[2];
    return result;
}

uint32_t GlobalDescriptorTable::SegmentDescriptor::Limit() {

    uint8_t* target = (uint8_t*)this;

    uint32_t result = target[6] & 0xF;
    result = (result << 8) + target[1];
    result = (result << 8) + target[0];

    if((target[6] & 0xC0) == 0xC0)
        result = (result << 12) | 0xFFF;

    return result;
}

