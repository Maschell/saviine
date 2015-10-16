#include "../../libwiiu/src/coreinit.h"
#include "../../libwiiu/src/socket.h"
#include "../../libwiiu/src/vpad.h"

#define assert(x) \
    do { \
        if (!(x)) \
            OSFatal("Assertion failed " #x ".\n"); \
    } while (0)

#if VER == 532
    #include "saviine532.h"

    #define socket_lib_finish ((void (*)(void))0x10c0404)

    /* Where to install the saviine handler. */
    #define INSTALL_ADDR        ((void *)0x011de000)
    #define KERN_ADDRESS_TBL    0xFFEAAA10
#endif

void kern_write(void *addr, uint32_t value);
#define DEFAULT_SERVER_IP   0xC0A8010A
#define PRINT_TEXT1(x, y, str) { OSScreenPutFontEx(1, x, y, str); }
#define PRINT_TEXT2(x, y, _fmt, ...) { __os_snprintf(msg, 80, _fmt, __VA_ARGS__); OSScreenPutFontEx(1, x, y, msg); }
#define BTN_PRESSED (BUTTON_LEFT | BUTTON_RIGHT | BUTTON_UP | BUTTON_DOWN)

typedef union u_serv_ip
{
    uint8_t  digit[4];
    uint32_t full;
} u_serv_ip;

void start() {
    /* Load a good stack */
    asm(
        "lis %r1, 0x1ab5 ;"
        "ori %r1, %r1, 0xd138 ;"
    );

    /* Get a handle to coreinit.rpl. */
    unsigned int coreinit_handle;
    OSDynLoad_Acquire("coreinit.rpl", &coreinit_handle);

    /* Load a few useful symbols. */
    void (*_Exit)(void) __attribute__ ((noreturn));
    void (*DCFlushRange)(const void *, int);
    void *(*OSEffectiveToPhysical)(const void *);
    
    OSDynLoad_FindExport(coreinit_handle, 0, "_Exit", &_Exit);
    OSDynLoad_FindExport(coreinit_handle, 0, "DCFlushRange", &DCFlushRange);
    OSDynLoad_FindExport(coreinit_handle, 0, "OSEffectiveToPhysical", &OSEffectiveToPhysical);
    
    assert(_Exit);
    assert(DCFlushRange);
    assert(OSEffectiveToPhysical);
    
    /* Exploit proper begins here. */
    if (OSEffectiveToPhysical((void *)0xa0000000) != (void *)0x31000000) {
        OSFatal("no ksploit");
    } else {
        /* Second run. */
        
        /* ****************************************************************** */
        /*                             IP Settings                            */
        /* ****************************************************************** */

        // Set server ip
        u_serv_ip ip;
        ip.full = DEFAULT_SERVER_IP;

        // Prepare screen
        void (*OSScreenInit)();
        unsigned int (*OSScreenGetBufferSizeEx)(unsigned int bufferNum);
        unsigned int (*OSScreenSetBufferEx)(unsigned int bufferNum, void * addr);
        unsigned int (*OSScreenClearBufferEx)(unsigned int bufferNum, unsigned int temp);
        unsigned int (*OSScreenFlipBuffersEx)(unsigned int bufferNum);
        unsigned int (*OSScreenPutFontEx)(unsigned int bufferNum, unsigned int posX, unsigned int posY, void * buffer);

        OSDynLoad_FindExport(coreinit_handle, 0, "OSScreenInit", &OSScreenInit);
        OSDynLoad_FindExport(coreinit_handle, 0, "OSScreenGetBufferSizeEx", &OSScreenGetBufferSizeEx);
        OSDynLoad_FindExport(coreinit_handle, 0, "OSScreenSetBufferEx", &OSScreenSetBufferEx);
        OSDynLoad_FindExport(coreinit_handle, 0, "OSScreenClearBufferEx", &OSScreenClearBufferEx);
        OSDynLoad_FindExport(coreinit_handle, 0, "OSScreenFlipBuffersEx", &OSScreenFlipBuffersEx);
        OSDynLoad_FindExport(coreinit_handle, 0, "OSScreenPutFontEx", &OSScreenPutFontEx);

        int screen_buf0_size = 0;
        int screen_buf1_size = 0;
        uint32_t screen_color = 0; // (r << 24) | (g << 16) | (b << 8) | a;
        char msg[80];

         // Init screen
        OSScreenInit();

        // Set screens buffers
        screen_buf0_size = OSScreenGetBufferSizeEx(0);
        screen_buf1_size = OSScreenGetBufferSizeEx(1);

        OSScreenSetBufferEx(0, (void *)0xF4000000);
        OSScreenSetBufferEx(1, (void *)0xF4000000 + screen_buf0_size);

        // Clear screens
        OSScreenClearBufferEx(0, screen_color);
        OSScreenClearBufferEx(1, screen_color);

        // Flush the cache
        DCFlushRange((void *)0xF4000000, screen_buf0_size);
        DCFlushRange((void *)0xF4000000 + screen_buf0_size, screen_buf1_size);

        // Flip buffers
        OSScreenFlipBuffersEx(0);
        OSScreenFlipBuffersEx(1);
        
        // Prepare vpad
        unsigned int vpad_handle;
        int (*VPADRead)(int controller, VPADData *buffer, unsigned int num, int *error);
        OSDynLoad_Acquire("vpad.rpl", &vpad_handle);
        OSDynLoad_FindExport(vpad_handle, 0, "VPADRead", &VPADRead);

        // Set server ip with buttons
        uint8_t sel_ip = 3;
        int error;
        uint8_t button_pressed = 1;
        VPADData vpad_data;
        VPADRead(0, &vpad_data, 1, &error); //Read initial vpad status
        while (1)
        {
            // Refresh screen if needed
            if (button_pressed) { OSScreenFlipBuffersEx(1); OSScreenClearBufferEx(1, 0); }

            // Print message
            PRINT_TEXT1(18, 1, "-- SAVIINE (a cafiine mod) --");
            PRINT_TEXT2(0, 5, "%s : %3d.%3d.%3d.%3d", "1. Server IP", ip.digit[0], ip.digit[1], ip.digit[2], ip.digit[3]);
            PRINT_TEXT1(0, 6, "2. Press A to install saviine");
            PRINT_TEXT1(42, 17, "home button to exit ...");
            
            // Print ip digit selector
            uint8_t x_shift = 15 + 4 * sel_ip;
            PRINT_TEXT1(x_shift, 4, "vvv");

            // Read vpad
            VPADRead(0, &vpad_data, 1, &error);

            // Update screen
            if (button_pressed)
            {
                OSScreenFlipBuffersEx(1);
                OSScreenClearBufferEx(1, 0);
            }
            // Check for buttons
            else
            {
                // Home Button
                if (vpad_data.btn_hold & BUTTON_HOME)
                    goto quit;

                // A Button
                if (vpad_data.btn_hold & BUTTON_A)
                    break;

                // Left/Right Buttons
                if (vpad_data.btn_hold & BUTTON_LEFT ) sel_ip = !sel_ip ? sel_ip = 3 : --sel_ip;
                if (vpad_data.btn_hold & BUTTON_RIGHT) sel_ip = ++sel_ip % 4;

                // Up/Down Buttons
                if (vpad_data.btn_hold & BUTTON_UP  ) ip.digit[sel_ip] = ++ip.digit[sel_ip];
                if (vpad_data.btn_hold & BUTTON_DOWN) ip.digit[sel_ip] = --ip.digit[sel_ip];
            }

            // Button pressed ?
            button_pressed = (vpad_data.btn_hold & BTN_PRESSED) ? 1 : 0;
        }

        /* ****************************************************************** */
        /*                         Saviine installation                       */
        /* ****************************************************************** */
        
        /* Copy in our resident cafiine client. */
        unsigned int len = sizeof(cafiine_text_bin);
        unsigned char *loc = (unsigned char *)((char *)INSTALL_ADDR + 0xa0000000);
        
        while (len--) {
            loc[len] = cafiine_text_bin[len];
        }

        /* server IP address */
        ((unsigned int *)loc)[0] = ip.full; //PC_IP;

        DCFlushRange(loc, sizeof(cafiine_text_bin));

        struct magic_t {
            void *real;
            void *replacement;
            void *call;
        } *magic = (struct magic_t *)cafiine_magic_bin;
        len = sizeof(cafiine_magic_bin) / sizeof(struct magic_t);
        
        int *space = (int *)(loc + sizeof(cafiine_text_bin));
        /* Patch branches to it. */
        while (len--) {
            *(int *)(0xa0000000 | (int)magic[len].call) = (int)space - 0xa0000000;

            *space = *(int *)(0xa0000000 | (int)magic[len].real);
            space++;

            *space = 0x48000002 | (((int)magic[len].real + 4) & 0x03fffffc);
            space++;
            DCFlushRange(space - 2, 8);
            
            *(int *)(0xa0000000 | (int)magic[len].real) = 0x48000002 | ((int)magic[len].replacement & 0x03fffffc);
            DCFlushRange((int *)(0xa0000000 | (int)magic[len].real), 4);
        }
        
        unsigned int *socket_finish = (unsigned int *)((char *)socket_lib_finish + 0xa0000000);
        socket_finish[0] = 0x38600000;
        socket_finish[1] = 0x4e800020;

        /* The fix for Splatoon and such */
        kern_write(KERN_ADDRESS_TBL + (0x12 * 4), 0x00000000);
        kern_write(KERN_ADDRESS_TBL + (0x13 * 4), 0x14000000);     
    }

quit:
    _Exit();
}

/* Write a 32-bit word with kernel permissions */
void kern_write(void *addr, uint32_t value)
{
  asm(
      "li 3,1\n"
      "li 4,0\n"
      "mr 5,%1\n"
      "li 6,0\n"
      "li 7,0\n"
      "lis 8,1\n"
      "mr 9,%0\n"
      "mr %1,1\n"
      "li 0,0x3500\n"
      "sc\n"
      "nop\n"
      "mr 1,%1\n"
      :
      :"r"(addr), "r"(value)
      :"memory", "ctr", "lr", "0", "3", "4", "5", "6", "7", "8", "9", "10",
       "11", "12"
      );
}
