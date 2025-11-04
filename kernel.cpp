#include "types.h"
#include "gdt.h"
#include "interrupts.h"
#include "keyboard.h"
#include "mouse.h"

// Global variables for cursor position
static uint16_t* VideoMemory = (uint16_t*)0xb8000;
static int x=0, y=0;

void clearScreen(){
    for(y = 0; y < 25; y++){
        for(x = 0; x < 80; x++){
            // 0x0720 is White-on-Black (0x07) attribute with a space character (0x20)
            VideoMemory[80*y + x] = 0x0720;
        }
    }
    // Reset cursor position to top-left
    x = 0;
    y = 0;
}

extern "C" void printf(const char* str){
    for (int i = 0; str[i] != '\0' ; ++i) {
        char character = str[i];

        switch(character){
            case '\n':
                y++; // Move to next line
                x = 0; // Reset x to column 0
                break;

            default:
                // Print character at current cursor position (x, y)
                VideoMemory[80*y + x] = (VideoMemory[80*y + x] & 0xFF00) | character;
                x++;
                break;
        }

        if(x >= 80){
            x = 0;
            y++;
        }

        // Basic scrolling
        if(y >= 25){
            for(y = 1; y < 25; y++){
                for(x = 0; x < 80; x++){
                    VideoMemory[80*(y-1) + x] = VideoMemory[80*y + x];
                }
            }

            // Clear the last line
            for(x = 0; x < 80; x++){
                VideoMemory[80*24 + x] = 0x0720;
            }
            y = 24;
            x = 0;
        }
    }
}

void printfHex(uint8_t key) {
    const char* hex = "0123456789ABCDEF";
    char foo[] = "00 ";
    foo[0] = hex[(key >> 4) & 0x0F];
    foo[1] = hex[key & 0x0F];
    printf(foo);
}


class PrintfKeyboardEventHandler : public KeyboardEventHandler
{
public:
    void OnKeyDown(const char c)
    {
        char foo[] = " ";
        foo[0] = c;
        printf(foo);
    }
};

typedef void (*constructor)();

extern "C" {
    extern constructor start_ctors;
    extern constructor end_ctors;

    extern void callConstructors(){
    for (constructor* i = &start_ctors; i != &end_ctors ; i++)
            (*i)();
    }

    extern void kernelMain(void* multiboot_structure, uint32_t /*magicnumber */) {
        clearScreen();
        printf("Welcome to ArchAngel_OS!\n");
        printf("Project is on github.com/Harshit-Dhanwalkar/archangelos");

        GlobalDescriptorTable gdt;
        InterruptManager interrupts(&gdt); // Instnaciation of InterruptManager

        PrintfKeyboardEventHandler kbhandler;
        KeyboardDriver keyboard(&interrupts, &kbhandler);
        MouseDriver mouse(&interrupts);

        interrupts.Activate(); // Activation of InterruptManager

        while(1);
    }
}
