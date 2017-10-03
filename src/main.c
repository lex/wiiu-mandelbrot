#include <coreinit/core.h>
#include <coreinit/debug.h>
#include <coreinit/foreground.h>
#include <coreinit/systeminfo.h>
#include <coreinit/thread.h>
#include <proc_ui/procui.h>
#include <stdint.h>
#include <sysapp/launch.h>

#include "draw.h"
#include "memory.h"

uint8_t logBuffer[0x400];
bool initialized = false;
bool isAppRunning = true;

void initializeScreen() {
    // Grab the buffer size for each screen (TV and gamepad)
    const int sizeOfBuffer0 = OSScreenGetBufferSizeEx(0);
    const int sizeOfBuffer1 = OSScreenGetBufferSizeEx(1);
    __os_snprintf(logBuffer, 0x400, "Screen sizes %x, %x\n", sizeOfBuffer0,
                  sizeOfBuffer1);
    OSReport(logBuffer);

    // Set the buffer area.
    screenBuffer = MEM1_alloc(sizeOfBuffer0 + sizeOfBuffer1, 0x40);
    __os_snprintf(logBuffer, 0x400, "Allocated screen buffers at %x\n",
                  screenBuffer);
    OSReport(logBuffer);

    OSScreenSetBufferEx(0, screenBuffer);
    OSScreenSetBufferEx(1, (screenBuffer + sizeOfBuffer0));
    OSReport("Set screen buffers\n");

    OSScreenEnableEx(0, 1);
    OSScreenEnableEx(1, 1);

    clearFramebuffers();
}

void deInitializeScreen() {
    clearFramebuffers();
    MEM1_free(screenBuffer);
}

static void SaveCallback() {
    OSSavesDone_ReadyToRelease(); // Required
}

static bool AppRunning() {
    if (!OSIsMainCore()) {
        ProcUISubProcessMessages(true);
        return isAppRunning;
    }

    ProcUIStatus status = ProcUIProcessMessages(true);

    switch (status) {
        case PROCUI_STATUS_EXITING:
            // Being closed, deinit, free, and prepare to exit
            isAppRunning = false;

            if (initialized) {
                initialized = false;
                deInitializeScreen();
                memoryRelease();
            }

            ProcUIShutdown();

            break;

        case PROCUI_STATUS_RELEASE_FOREGROUND:
            // Free up MEM1 to next foreground app, deinit screen, etc.
            deInitializeScreen();
            memoryRelease();
            ProcUIDrawDoneRelease();

            break;

        case PROCUI_STATUS_IN_FOREGROUND:
            // Executed while app is in foreground

            if (!initialized) {
                OSReport("Initializing screen...");
                initialized = true;
                memoryInitialize();
                initializeScreen();
                OSReport("Screen initialized.");
            }

            break;
    }

    return isAppRunning;
}

int main(int argc, char** argv) {
    OSEnableHomeButtonMenu(false);
    OSScreenInit();
    ProcUIInit(&SaveCallback);

    while (AppRunning()) {
        if (!initialized) {
            continue;
        }

        drawString(0, 0, "hello world");
    }

    SYSRelaunchTitle(0, NULL);

    return 0;
}
