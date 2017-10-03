#include <coreinit/core.h>
#include <coreinit/debug.h>
#include <coreinit/foreground.h>
#include <coreinit/systeminfo.h>
#include <coreinit/thread.h>
#include <proc_ui/procui.h>
#include <stdint.h>
#include <sysapp/launch.h>
#include <vpad/input.h>

#include "draw.h"
#include "memory.h"

#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 272
#define LOG_BUFFER_SIZE 0x400
#define MEM1_ALIGNMENT 0x40

uint8_t logBuffer[LOG_BUFFER_SIZE] = {0};

bool initialized = false;
bool isAppRunning = true;
bool needToDraw = true;

double xOffset = 0.01f;
double yOffset = 0.01f;
double zoomLevel = 1.0f;

void initializeScreen() {
    // Grab the buffer size for each screen (TV and gamepad)
    const int sizeOfBuffer0 = OSScreenGetBufferSizeEx(0);
    const int sizeOfBuffer1 = OSScreenGetBufferSizeEx(1);
    __os_snprintf(logBuffer, LOG_BUFFER_SIZE, "Screen sizes %x, %x\n",
                  sizeOfBuffer0, sizeOfBuffer1);
    OSReport(logBuffer);

    // Set the buffer area.
    screenBuffer = MEM1_alloc(sizeOfBuffer0 + sizeOfBuffer1, MEM1_ALIGNMENT);
    __os_snprintf(logBuffer, LOG_BUFFER_SIZE,
                  "Allocated screen buffers at %x\n", screenBuffer);
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

void drawMandelbrot() {
    const int maximumIterations = 200;
    const uint8_t alpha = 255;
    uint8_t color[3] = {0};

    for (int row = 0; row < SCREEN_HEIGHT; ++row) {
        for (int column = 0; column < SCREEN_WIDTH; ++column) {
            const double cRe = ((column - SCREEN_WIDTH / 2.0f) * 4.0f *
                                zoomLevel / SCREEN_WIDTH) +
                               xOffset;
            const double cIm = ((row - SCREEN_HEIGHT / 2.0f) * 4.0f *
                                zoomLevel / SCREEN_WIDTH) +
                               yOffset;

            double x = 0.0f;
            double y = 0.0f;

            int iteration = 0;

            while ((x * x + y * y) <= 4.0f && iteration < maximumIterations) {
                double newX = (x * x - y * y + cRe);
                y = (2.0f * x * y + cIm);
                x = newX;
                ++iteration;
            }

            const uint8_t colorValue =
                (iteration >= maximumIterations) ? 0 : 255;

            for (int i = 0; i < 3; ++i) {
                color[i] = colorValue;
            }

            drawPixel(column, row, color[0], color[1], color[2], alpha);
        }
    }
}

int main(int argc, char** argv) {
    OSEnableHomeButtonMenu(false);
    OSScreenInit();
    ProcUIInit(&SaveCallback);

    int vpadError = 0;
    VPADStatus vpadData;

    while (AppRunning()) {
        if (!initialized) {
            continue;
        }

        VPADRead(0, &vpadData, 1, &vpadError);

        if (vpadData.hold & VPAD_BUTTON_RIGHT) {
            xOffset += 0.1f * zoomLevel;
            needToDraw = true;
        }

        if (vpadData.hold & VPAD_BUTTON_LEFT) {
            xOffset -= 0.1f * zoomLevel;
            needToDraw = true;
        }

        if (vpadData.hold & VPAD_BUTTON_UP) {
            zoomLevel *= 0.90f;
            needToDraw = true;
        }

        if (vpadData.hold & VPAD_BUTTON_DOWN) {
            zoomLevel *= 1.10f;
            needToDraw = true;
        }

        if (needToDraw) {
            drawMandelbrot();
            flipBuffers();
            needToDraw = false;
        }
    }

    SYSRelaunchTitle(0, NULL);

    return 0;
}
