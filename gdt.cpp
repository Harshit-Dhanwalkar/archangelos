#include "gdt.h"
#include "types.h"

GlobalDescriptorTable::GlobalDescriptorTable():
    nullSegmentSelector(0, 0 ,0), // Base, Limit, Flags
    unusedSegmentSelector(0, 0, 0), // Base, Limit, Flags
    codeSegmentSelector(0, 64*1024*1024, 0x9A), // Base, Limit, Code Segment Flags (0x9A)
    dataSegmentSelector(0, 64*1024*1024, 0x92) // Base, Limit, Data Segment Flags (0x92)
{
    uint32_t i[2];
    i[0] = (uint32_t)this; // first byte for address of table itself
    i[1] = sizeof(GlobalDescriptorTable) << 16; // fisrt 4 bytes are high byte of second integer

    // GDTR structure: [limit (16 bits) | base (32 bits)] (total 6 bytes)
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

}

GlobalDescriptorTable::~GlobalDescriptorTable() {

}

uint16_t GlobalDescriptorTable::DataSegmentSelector() {
    return (uint8_t*)&dataSegmentSelector - (uint8_t*)this;
}

uint16_t GlobalDescriptorTable::CodeSegmentSelector() {
    return (uint8_t*)&codeSegmentSelector - (uint8_t*)this;
}

GlobalDescriptorTable::SegmentDescriptor::SegmentDescriptor(uint32_t base, uint32_t limit, uint8_t flags) {
    uint8_t* target = (uint8_t*)this;
    target[6] = 0xC0; // Set D/B (32-bit default) and G (4K granularity)

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
        target[6] = 0xC0;
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
    target[5] = flags;
}

uint32_t GlobalDescriptorTable::SegmentDescriptor::Base() {
    uint8_t* target = (uint8_t*)this;
    uint32_t result = target[7];

    result = (result << 8) | target[4];
    result = (result << 8) | target[3];
    result = (result << 8) | target[2];
    return result;
}

uint32_t GlobalDescriptorTable::SegmentDescriptor::Limit() {
    uint8_t* target = (uint8_t*)this;
    uint32_t result = target[0] | ((target[1] << 8) | ((target[6] & 0xF) << 16));

    if ((target[6] & 0xC0) == 0xC0) {
        result = (result << 12) | 0xFFF;
    }

    return result;
}
