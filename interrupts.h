#ifndef __INTERRUPTS_H
#define __INTERRUPTS_H

#include "types.h"
#include "port.h"
#include "gdt.h"

class InterruptManager;

class InterruptHandler {
    protected:
        uint8_t interruptNumber;
        InterruptManager* interruptManager;

        InterruptHandler(uint8_t interruptNumber, InterruptManager* interruptManager);
        ~InterruptHandler();

    public:
        uint32_t HandleInterrupt(uint32_t esp);
};

class InterruptManager {
    friend class InterruptHandler;
    protected:
        InterruptHandler* handler[256];

        struct GateDescriptor {
            uint16_t handleAddressLowBits;
            uint16_t gdt_codeSegmentSelector;
            uint8_t reserved;
            uint8_t access;
            uint16_t handleAddressHighBits;
        } __attribute__((packed));

        static GateDescriptor InterruptDescriptorTable[256];

        struct InterruptDescriptorTablePointer {
            uint16_t size;
            uint32_t base;
        } __attribute__((packed));

        static void SetInterruptDescriptorTableEntry(
            uint8_t interruptNumber,
            uint16_t codeSegmentSelectorOffset,
            void (*handler)(),
            uint8_t DescriptorPrivilegelLevel,
            uint8_t DescriptorType
        );

        Port8BitSlow programmableInterruptControllerMasterCommandPort;
        Port8BitSlow programmableInterruptControllerMasterDataPort;
        Port8BitSlow programmableInterruptControllerSlaveCommandPort;
        Port8BitSlow programmableInterruptControllerSlaveDataPort;

    public:
        static InterruptManager* ActiveInterruptManager;

        InterruptManager(GlobalDescriptorTable* globalDescriptorTable);
        ~InterruptManager();

        void Activate();
        void Deactivate();

        uint32_t HandleInterrupt(uint8_t interruptNumber, uint32_t esp);
        uint32_t DoHandleInterrupt(uint8_t interruptNumber, uint32_t esp);

        static void IgnoreInterruptRequest();
        static void HandleInterruptRequest0x00(); // Timeout interrupt
        static void HandleInterruptRequest0x01(); // Keyboard interrupt
        static void HandleInterruptRequest0x0C(); // Mouse interrupt
};

#endif // !__INTERRUPTS_H
