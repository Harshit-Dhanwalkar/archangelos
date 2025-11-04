#include "interrupts.h"
#include "gdt.h"
#include "port.h"


InterruptHandler::InterruptHandler(uint8_t interruptNumber, InterruptManager* interruptManager){
    this->interruptNumber = interruptNumber;
    this->interruptManager = interruptManager;
    interruptManager->handler[interruptNumber] = this;
}

InterruptHandler::~InterruptHandler(){
    if (interruptManager->handler[interruptNumber] == this) {
        interruptManager->handler[interruptNumber] = 0;
    }
}

uint32_t InterruptHandler::HandleInterrupt(uint32_t esp){
    return esp;
}

InterruptManager::GateDescriptor InterruptManager::InterruptDescriptorTable[256];

InterruptManager* InterruptManager::ActiveInterruptManager = 0;

void InterruptManager::SetInterruptDescriptorTableEntry(
        uint8_t interruptNumber,
        uint16_t codeSegmentSelectorOffset,
        void (*handler)(),
        uint8_t DescriptorPrivilegelLevel,
        uint8_t DescriptorType
    ){
        const uint8_t IDT_DESC_PRESENT = 0x80;

        InterruptDescriptorTable[interruptNumber].handleAddressLowBits = ((uint64_t)handler) & 0xFFFF;
        InterruptDescriptorTable[interruptNumber].handleAddressHighBits = (((uint64_t)handler) >> 16) & 0xFFFF;
        InterruptDescriptorTable[interruptNumber].gdt_codeSegmentSelector = codeSegmentSelectorOffset;
        InterruptDescriptorTable[interruptNumber].access = IDT_DESC_PRESENT | DescriptorType | ((DescriptorPrivilegelLevel&3) <<5);
        InterruptDescriptorTable[interruptNumber].reserved = 0;
}

InterruptManager::InterruptManager(GlobalDescriptorTable* globalDescriptorTable)
: programmableInterruptControllerMasterCommandPort(0x20),
  programmableInterruptControllerMasterDataPort(0x21),
  programmableInterruptControllerSlaveCommandPort(0xA0),
  programmableInterruptControllerSlaveDataPort(0xA1)
{
     uint32_t CodeSegment = globalDescriptorTable->CodeSegmentSelector();

     const uint8_t IDT_INTERRUPT_GATE = 0xE;
     for (uint8_t i = 255; i > 0; --i) {
         handler[i] = 0;
         SetInterruptDescriptorTableEntry(i, CodeSegment, &IgnoreInterruptRequest, 0, IDT_INTERRUPT_GATE);
     }

     SetInterruptDescriptorTableEntry(0x20, CodeSegment, &HandleInterruptRequest0x00, 0, IDT_INTERRUPT_GATE);
     SetInterruptDescriptorTableEntry(0x21, CodeSegment, &HandleInterruptRequest0x01, 0, IDT_INTERRUPT_GATE);
     SetInterruptDescriptorTableEntry(0x2C, CodeSegment, &HandleInterruptRequest0x0C, 0, IDT_INTERRUPT_GATE);

     programmableInterruptControllerMasterCommandPort.Write(0x11);
     programmableInterruptControllerSlaveCommandPort.Write(0x11);

     programmableInterruptControllerMasterDataPort.Write(0x20);
     programmableInterruptControllerSlaveDataPort.Write(0x28);

     programmableInterruptControllerMasterDataPort.Write(0x04);
     programmableInterruptControllerSlaveDataPort.Write(0x02);

     programmableInterruptControllerMasterDataPort.Write(0x01); // ICW4: 8086 mode
     programmableInterruptControllerSlaveDataPort.Write(0x01); // ICW4: 8086 mode

     programmableInterruptControllerMasterDataPort.Write(0x00);
     programmableInterruptControllerSlaveDataPort.Write(0x00);

     InterruptDescriptorTablePointer idt;
     idt.size = 256 * sizeof(GateDescriptor) - 1;
     idt.base = (uint64_t)InterruptDescriptorTable;

     asm volatile("lidt %0" : : "m" (idt));
}

InterruptManager::~InterruptManager(){
}

void InterruptManager::Activate(){
    if (ActiveInterruptManager != 0) {
        ActiveInterruptManager->Deactivate();
    }
    ActiveInterruptManager = this;
    asm("sti");
}

void InterruptManager::Deactivate(){
    if (ActiveInterruptManager == this) {
        ActiveInterruptManager = 0;
        asm("cli");
    }
}

extern "C" uint32_t InterruptManager::HandleInterrupt(uint8_t interruptNumber, uint32_t esp) {
    if (ActiveInterruptManager != 0) {
        return ActiveInterruptManager->DoHandleInterrupt(interruptNumber, esp);
    }
    return esp;
}

extern "C" uint32_t handleInterrupt(uint8_t interruptNumber, uint32_t esp) {
    if (InterruptManager::ActiveInterruptManager != 0) {
        return InterruptManager::ActiveInterruptManager->DoHandleInterrupt(interruptNumber, esp);
    }
    return esp;
}

extern "C" void printf(const char* str);

uint32_t InterruptManager::DoHandleInterrupt(uint8_t interruptNumber, uint32_t esp) {
    if (handler[interruptNumber] != 0) {
        esp = handler[interruptNumber] -> HandleInterrupt(esp);
    }
    else if (interruptNumber != 0x20) {
        char foo[] = "UNHANDLED INTERRUPT 0x00";
        const char* hex = "0123456789ABCDEF";
        foo[22] = hex[(interruptNumber >> 4) & 0x0F];
        foo[23] = hex[interruptNumber & 0x0F];
        printf(foo);
    }

    if (0x20 <= interruptNumber && interruptNumber < 0x30) {
        programmableInterruptControllerMasterCommandPort.Write(0x20);
        if (0x28 <= interruptNumber) {
            programmableInterruptControllerSlaveCommandPort.Write(0x20);
        }
    }

    return esp;
}
