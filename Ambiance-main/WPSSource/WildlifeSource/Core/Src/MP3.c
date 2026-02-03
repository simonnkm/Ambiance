/*
 * Name: MP3.c
 * Brief: DFPlayer/MP3 module for event/service framework.
 * Author: Caitlin Bonesio
 * Created: 4/19/25
 * Modified: 4/23/25
 *
 * Notes:
 * - Communicates with the MP3 module over LPUART using DFPlayer-style packets.
 * - Maintains a simple duty-cycle “play then pause” behavior based on FLASH duty cycle.
 * - Randomizes next track on “song complete” (0x3D), with basic debounce.
 * - Includes defensive recovery if “song complete” is never received.
 */

//----------------------------------------Private Includes---------------------------------------
#include "CONFIG.h"
#include "MP3.h"
#include "TIMERS.h"
#include "FLASH.h"
#include "UART.h"
#include "Scheduler.h"
#include "discountIO.h"
#include "FIFO.h"
#include <limits.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//----------------------------------------Private Defines----------------------------------------
#define CYCLELENGTH (10/*minutes*/*60000/*milliseconds/minute*/) // duty cycle window (ms)
#define RESETTIMER 2000
#define FAILEDTRACKRECOVERY (2/*minutes*/*60000/*milliseconds/minute*/)
#define DEFAULT_VOLUME 30 // startup volume (0-100 mapped to DFPlayer 0-30)

//----------------------------------------Private Typedefs---------------------------------------
typedef enum DFPacketStates{
    Start,
    Version,
    Length,
    Command,
    Ack,
    Param1,
    Param2,
    Checksum1,
    Checksum2,
} DFPacketState_t;

typedef struct DFPacket{
    uint8_t command;
    uint8_t ack;
    uint8_t Param1;
    uint8_t Param2;
} DFPacket_t;

typedef enum DFinitSM{
    setup,
    scanning,
} DFinitSM_t;

//----------------------------------------Private Variables--------------------------------------
extern RNG_HandleTypeDef hrng;
FIFO MP3queue;

/*
 * pause meanings:
 *   0x00: actively playing / allowed to play
 *   0x01: duty-cycle pause (resume after computed pause time)
 *   0x02: stopped until next explicit play event
 */
static uint8_t pause;
static uint8_t DC;        // duty cycle percentage (clamped to 1..100)
static uint8_t volume;    // 0..100 (mapped to DFPlayer range)

static uint32_t starttime;    // watchdog / track start reference for recovery
static uint32_t endtime;      // time we paused (or track ended)

static uint32_t inittime;
static uint8_t initialized;
static DFinitSM_t initSM;
static uint8_t df_error_retries = 0;

static DFPacketState_t PacketSM;
static DFPacket_t Packet;

static uint8_t track;
static uint8_t folder;
static uint16_t firsttrack;
static uint8_t nexttrack;
static uint8_t lastplayed;
static uint8_t lasttrack = 0;
static uint8_t* folders;
static uint8_t numfolders;
static uint32_t last_complete_ms = 0;

static char lastsent[4];
static uint32_t track_start_ms = 0;

//----------------------------------------Private Functions--------------------------------------

/**
 * @brief Send a single DFPlayer command packet.
 *
 * Packet format:
 *   0x7E, 0xFF, 0x06, [cmd, ack, p1, p2], [checksum_hi, checksum_lo], 0xEF
 *
 * @param string 4-byte payload: {command, ack, Param1, Param2}
 */
void MP3_SendData(char string[4]){
    uint16_t accumulation = 0;

    /* Optional LED instrumentation. Remove if you want zero side effects. */
    if ((uint8_t)string[0] == 0x03) {          // play absolute
        BSP_LED_Toggle(LED_RED);
    } else if ((uint8_t)string[0] == 0x0D) {   // play
        BSP_LED_Toggle(LED_BLUE);
    }

    LPUART_WriteTx(0x7E);         // start
    accumulation += 0xFF;
    LPUART_WriteTx(0xFF);         // version
    accumulation += 0x06;
    LPUART_WriteTx(0x06);         // length

    for (int i = 0; i < 4; i++) {
        accumulation += (uint8_t)string[i];
        lastsent[i] = string[i];
        LPUART_WriteTx((uint8_t)string[i]);
    }

    accumulation = (uint16_t)(-((int16_t)accumulation)); // checksum
    LPUART_WriteTx((uint8_t)(accumulation >> 8));
    LPUART_WriteTx((uint8_t)(accumulation & 0xFF));
    LPUART_WriteTx(0xEF);         // end
}

/**
 * @brief Play a track by absolute index (1-based indexing expected by module).
 * @param index16 Absolute track index.
 */
static void MP3_PlayAbsolute(uint16_t index16){
    char send[4] = {
        0x03,
        0x00,
        (char)((index16 >> 8) & 0xFF),
        (char)(index16 & 0xFF)
    };
    MP3_SendData(send);
}

/**
 * @brief Parse incoming UART stream into DFPlayer packets.
 *
 * Behavior:
 * - Returns 1 when a full valid packet is received and Packet fields are populated.
 * - Returns 0 otherwise.
 * - Resynchronizes on start flag and checksum mismatch.
 * - Special handling for 0x40 error packets:
 *     - If Param2 == 0x04 (bad track), retry last command up to 3 times.
 *     - After retries, converts the condition into a synthetic "song complete" (0x3D)
 *       so the upper layer can advance.
 *
 * @param rx Next byte from UART.
 * @return 1 if packet completed, else 0.
 */
uint8_t parsePacket(char rx){
    if (rx == UARTFAILED) {
        return 0;
    }

    if ((uint8_t)rx == 0x7E) {
        PacketSM = Start;
    }

    switch (PacketSM) {
    case Start:
        if ((uint8_t)rx == 0xFF) {
            PacketSM = Version;
        }
        break;

    case Version:
        if ((uint8_t)rx == 0x06) {
            PacketSM = Length;
        } else {
            PacketSM = Start;
        }
        break;

    case Length:
        PacketSM = Command;
        Packet.command = (uint8_t)rx;
        break;

    case Command:
        if ((uint8_t)rx == 0x01 || (uint8_t)rx == 0x00) {
            Packet.ack = (uint8_t)rx;
            PacketSM = Ack;
        } else {
            PacketSM = Start;
        }
        break;

    case Ack:
        PacketSM = Param1;
        Packet.Param1 = (uint8_t)rx;
        break;

    case Param1:
        PacketSM = Param2;
        Packet.Param2 = (uint8_t)rx;
        break;

    case Param2: {
        uint16_t checkval = (uint16_t)(-(0x105 + Packet.command + Packet.ack + Packet.Param1 + Packet.Param2));
        if ((uint8_t)rx == (uint8_t)(checkval >> 8)) {
            PacketSM = Checksum1;
        } else {
            PacketSM = Start;
        }
        break;
    }

    case Checksum1: {
        uint16_t checkval = (uint16_t)(-(0x105 + Packet.command + Packet.ack + Packet.Param1 + Packet.Param2));
        if ((uint8_t)rx == (uint8_t)(checkval & 0xFF)) {
            PacketSM = Checksum2;
        } else {
            PacketSM = Start;
        }
        break;
    }

    case Checksum2:
        if ((uint8_t)rx == 0xEF) {
            if (Packet.command == 0x40) {
                BSP_LED_Toggle(LED_RED);

                if (Packet.Param2 == 0x04) {
                    if (df_error_retries < 3) {
                        df_error_retries++;
                        MP3_SendData(lastsent);
                        PacketSM = Start;
                        return 0;
                    } else {
                        df_error_retries = 0;
                        Packet.command = 0x3D;  // synthetic complete
                        Packet.Param2  = track;
                    }
                } else {
                    df_error_retries = 0;
                }
            } else {
                df_error_retries = 0;
            }

            BSP_LED_Toggle(LED_BLUE);
            PacketSM = Start;
            return 1;
        }
        PacketSM = Start;
        break;
    }

    return 0;
}

//----------------------------------------Public Functions---------------------------------------

/**
 * @brief Initialize MP3 event module.
 *
 * - Initializes internal state.
 * - Loads duty-cycle and volume settings.
 * - Sends a module reset command.
 *
 * @param Queue FIFO queue used for posting events.
 * @return INIT_OK on success.
 */
uint8_t MP3_Event_Init(FIFO Queue){
    MP3queue = Queue;

    TIMERS_Init();

    pause = 0x02;
    DC    = FLASH_GetDutyCycle();

    /* Clamp to safe range. DC=0 would cause division-by-zero in pause-time math. */
    if (DC == 0) {
        DC = 1;
    } else if (DC > 100) {
        DC = 100;
    }

    volume = DEFAULT_VOLUME;
    starttime = TIMERS_GetMilliSeconds();
    inittime  = TIMERS_GetMilliSeconds();

    initSM = setup;
    initialized = 0;

    if (volume == 0xFF || volume > DEFAULT_VOLUME) {
        volume = DEFAULT_VOLUME;
    }

    {
        char send[4] = {0x0C, 0x00, 0x00, 0x00}; // reset module
        MP3_SendData(send);
    }

    return INIT_OK;
}

/**
 * @brief Post an event to the MP3 module queue.
 * @param event Event to enqueue.
 */
void MP3_Event_Post(Event_t event){
    FIFO_Enqueue(MP3queue, event);
}

/**
 * @brief Polling updater: reads UART, handles timeouts, and posts resulting events.
 *
 * Events generated:
 * - EVENT_LPUART: a valid DFPlayer packet parsed (command in upper byte, Param2 in lower).
 * - EVENT_INIT: during initialization when either a packet arrives or init timer expires.
 * - EVENT_TIMEOUT: duty-cycle pause has elapsed and playback should resume.
 * - EVENT_SETTINGS: FLASH volume or duty-cycle changed.
 *
 * @return (EVENT_NONE,0) always, since events are posted asynchronously to queue.
 */
Event_t MP3_Event_Updater(void){
    Event_t event = (Event_t){EVENT_NONE, 0};
    uint32_t timer = TIMERS_GetMilliSeconds();
    char rx = LPUART_ReadRx();

    /* UART parsing -> EVENT_LPUART (and EVENT_INIT if not initialized). */
    if (rx != UARTFAILED) {
        if (parsePacket(rx)) {
            event.status = EVENT_LPUART;
            event.data   = ((uint16_t)Packet.command << 8) | (uint16_t)Packet.Param2;

            if (!initialized) {
                event.status = EVENT_INIT;
                event.data   = 1;
            }
            MP3_Event_Post(event);
        }
    }

    /* Duty-cycle pause handling: when paused by DC, resume after computed pause time. */
    if (pause == 1 && initialized) {
        uint32_t curtime     = TIMERS_GetMilliSeconds();
        uint32_t active_time = (endtime > track_start_ms) ? (endtime - track_start_ms) : 0;
        uint32_t dc_percent  = (uint32_t)DC;

        BSP_LED_Toggle(LED_RED);

        if (dc_percent > 0 && dc_percent < 100) {
            uint64_t tmp        = (uint64_t)active_time * (100u - dc_percent);
            uint32_t pause_time = (uint32_t)(tmp / dc_percent);

            if ((uint32_t)(curtime - endtime) >= pause_time) {
                pause = 0;
                event.status = EVENT_TIMEOUT;
                MP3_Event_Post(event);
            }
        }
    }

    /* Recovery if module never reports song complete. */
    if (initialized && pause == 0) {
        uint32_t now = TIMERS_GetMilliSeconds();
        if (now - starttime > FAILEDTRACKRECOVERY) {
            Packet.command = 0x3D;
            Packet.Param2  = track;
            endtime        = now;

            event.status   = EVENT_LPUART;
            MP3_Event_Post(event);

            starttime = now;
        }
    }

    /* Initialization delay: wait for module to become ready. */
    if (((timer - inittime) >= 3000) && !initialized) {
        event.status = EVENT_INIT;
        event.data   = 0;
        MP3_Event_Post(event);
    }

    /* Settings changes while running. */
    if ((volume != FLASH_GetVolume() || DC != FLASH_GetDutyCycle()) && initialized) {
        event.status = EVENT_SETTINGS;
        event.data   = (volume == FLASH_GetVolume()); // 1 if only DC changed, 0 if volume changed too
        MP3_Event_Post(event);
    }

    return event;
}

/**
 * @brief Handle incoming events for MP3 module.
 *
 * Responsibilities:
 * - Initialization scan: determine folders and track counts.
 * - On EVENT_PLAY: start a requested folder/track.
 * - On song complete: pick next randomized track in same folder.
 * - Duty-cycle: if DC < 100, pause after each track then resume later.
 * - On settings update: apply new DC/volume.
 *
 * @param event Incoming event.
 * @return 1 on success, 0 if handler forced a safe stop.
 */
uint8_t MP3_Event_Handler(Event_t event){
    if (event.status == EVENT_INIT) {
        {
            char send[4] = {0x0E, 0x00, 0x00, 0x00}; // pause
            MP3_SendData(send);
        }

        uint8_t scanning = 1;
        numfolders = 1;
        FIFO tempFolders = FIFO_Create();

        while (scanning) {
            {
                char send[4] = {0x4E, 0x00, 0x00, numfolders}; // query files in folder
                MP3_SendData(send);
            }

            uint32_t time = TIMERS_GetMilliSeconds();
            while (!parsePacket(LPUART_ReadRx()) && (time + 1000) > TIMERS_GetMilliSeconds());

            if ((time + 1000) < TIMERS_GetMilliSeconds()) {
                continue;
            }

            if (Packet.command == 0x4E) {
                numfolders++;
                FIFO_Enqueue(tempFolders, (Event_t){EVENT_NONE, Packet.Param2});
            } else if (Packet.command == 0x40) {
                scanning = 0;
            }
        }

        numfolders--;

        folders = malloc(sizeof(uint8_t) * numfolders);
        for (int i = 0; i < numfolders; i++) {
            folders[i] = FIFO_Dequeue(tempFolders).data;
        }
        FIFO_Destroy(tempFolders);

        initialized = 1;

        {
            char send[4] = {0x06, 0x00, 0x00, (uint8_t)(((uint16_t)volume) * 30 / 100)};
            MP3_SendData(send);
        }
    }

    if (event.status == EVENT_TIMEOUT) {
        pause = 0;
        starttime = TIMERS_GetMilliSeconds();

        if (folder >= 1 && folder <= numfolders && folders != NULL) {
            firsttrack = 1;
            for (int i = 0; i < folder - 1; i++) {
                firsttrack += folders[i];
            }

            uint16_t abs_ind = firsttrack + nexttrack - 1;
            uint32_t now = TIMERS_GetMilliSeconds();
            starttime = now;
            track_start_ms = now;
            MP3_PlayAbsolute(abs_ind);
        } else {
            pause = 1;
            char send[4] = {0x0E, 0x00, 0x00, 0x00};
            MP3_SendData(send);
            return 0;
        }
    }

    if (event.status == EVENT_LPUART) {
        uint8_t cmd = (uint8_t)(event.data >> 8);

        if (cmd == 0x3D) {
            uint32_t now = TIMERS_GetMilliSeconds();

            if ((uint32_t)(now - last_complete_ms) > 300) {
                last_complete_ms = now;
                endtime = now;

                if (folder >= 1 && folder <= numfolders && folders != NULL) {
                    uint32_t n = folders[folder - 1];
                    if (n == 0) {
                        return 0;
                    }

                    do {
                        uint32_t r;
                        uint32_t limit = UINT32_MAX - (UINT32_MAX % n);
                        do {
                            HAL_RNG_GenerateRandomNumber(&hrng, &r);
                        } while (r >= limit);

                        nexttrack = (r % n) + 1;

                        if (n > 1 && nexttrack == lasttrack) {
                            nexttrack = (nexttrack % n) + 1;
                        }
                    } while (folder == 4 && nexttrack == 7);

                    lasttrack = nexttrack;
                    track = nexttrack;

                    firsttrack = 1;
                    for (int i = 0; i < folder - 1; i++) {
                        firsttrack += folders[i];
                    }

                    if (DC >= 100) {
                        uint16_t abs_ind = firsttrack + nexttrack - 1;
                        uint32_t tnow = TIMERS_GetMilliSeconds();
                        starttime = tnow;
                        track_start_ms = tnow;
                        MP3_PlayAbsolute(abs_ind);
                    } else {
                        pause = 1;
                        char send[4] = {0x0E, 0x00, 0x00, 0x00};
                        MP3_SendData(send);
                    }

                    return 1;
                } else {
                    return 0;
                }
            }
        }
    }

    if (event.status == EVENT_PLAY) {
        Scheduler_Event_Post(event);
        starttime = TIMERS_GetMilliSeconds();

        if ((event.data >> 8) != 0 && (event.data >> 8) <= numfolders) {
            if ((event.data & 0xFF) != 0 && (event.data & 0xFF) <= folders[(event.data >> 8) - 1]) {
                pause  = 0;
                folder = (uint8_t)(event.data >> 8);
                track  = (uint8_t)(event.data & 0xFF);

                firsttrack = 1;
                for (int i = 0; i < folder - 1; i++) {
                    firsttrack += folders[i];
                }

                {
                    uint16_t absIndex = firsttrack + track - 1;
                    uint32_t now = TIMERS_GetMilliSeconds();
                    starttime = now;
                    track_start_ms = now;
                    MP3_PlayAbsolute(absIndex);
                    HAL_Delay(100);
                }

                {
                    char send[4] = {0x0D, 0x00, 0x00, 0x00};
                    MP3_SendData(send);
                }

                lastplayed = 0;
            }
        } else {
            pause = 0x02;
            {
                char send[4] = {0x0E, 0x00, 0x00, 0x00};
                MP3_SendData(send);
            }
            folder = 0;
            track  = 0;
        }
    }

    if (event.status == EVENT_SETTINGS) {
        DC = FLASH_GetDutyCycle();
        if (DC == 0) {
            DC = 1;
        } else if (DC > 100) {
            DC = 100;
        }

        volume = FLASH_GetVolume();

        if (!event.data) {
            char send[4] = {0x06, 0x00, 0x00, (uint8_t)(((uint16_t)volume) * 30 / 100)};
            MP3_SendData(send);
        }
    }

    return 1;
}

/**
 * @brief Return currently tracked folder/track.
 * @return Upper byte: folder, lower byte: track.
 */
uint16_t MP3_GetCurrentFile(void){
    return ((uint16_t)folder << 8) + track;
}

//----------------------------------------Test Harness-------------------------------------------
//#define MP3TESTHARNESS

#ifdef MP3TESTHARNESS
#include "BOARD.h"
#include "discountIO.h"

int main(void){
    BOARD_Init();
    BSP_LED_Init(LED_BLUE);
    BSP_LED_Init(LED_RED);
    UARTs_Init();
    TIMERS_Init();

    {
        char send[4] = {0x0C, 0x00, 0x00, 0x00};
        MP3_SendData(send);
        HAL_Delay(1000);
    }

    {
        char send[4] = {0x0E, 0x00, 0x00, 0x00};
        MP3_SendData(send);
    }
    HAL_Delay(100);

    {
        char send[4] = {0x06, 0x00, 0x00, 0x10};
        MP3_SendData(send);
        HAL_Delay(100);
    }

    uint8_t scanning = 1;
    static uint8_t numfolders = 1;
    FIFO tempFolders = FIFO_Create();

    while (scanning) {
        {
            char send[4] = {0x4E, 0x00, 0x00, numfolders};
            MP3_SendData(send);
        }
        while (!parsePacket(LPUART_ReadRx()));
        if (Packet.command == 0x4E) {
            numfolders++;
            FIFO_Enqueue(tempFolders, (Event_t){EVENT_NONE, Packet.Param2});
        } else if (Packet.command == 0x40) {
            scanning = 0;
        }
    }

    numfolders--;

    uint8_t folders_local[numfolders];
    for (int i = 0; i < numfolders; i++) {
        folders_local[i] = FIFO_Dequeue(tempFolders).data;
    }
    FIFO_Destroy(tempFolders);

    HAL_Delay(100);

    folder = 1;
    track  = 1;
    firsttrack = 1;

    for (int i = 0; i < folder - 1; i++) {
        firsttrack += folders_local[i];
    }

    {
        char send2[4] = {0x03, 0x00, 0x00, (char)(firsttrack + track - 1)};
        MP3_SendData(send2);
        HAL_Delay(100);
    }

    {
        char send[4] = {0x0D, 0x00, 0x00, 0x00};
        MP3_SendData(send);
    }

    while (1) {
        char rx = LPUART_ReadRx();
        if (rx != UARTFAILED) {
            if (parsePacket(rx)) {
                if (Packet.command == 0x3D) {
                    if (Packet.Param2 != lastplayed) {
                        lastplayed = Packet.Param2;
                        track++;
                        HAL_Delay(100);

                        if (track > folders_local[folder - 1]) {
                            char send2[4] = {0x03, 0x00, 0x00, (char)firsttrack};
                            MP3_SendData(send2);
                            BSP_LED_On(LED_RED);
                            track = 1;
                        } else {
                            char send[4] = {0x01, 0x00, 0x00, 0x00};
                            MP3_SendData(send);
                            BSP_LED_Off(LED_RED);
                        }
                    }
                }
            }
        }
        HAL_Delay(1);
    }
}
#endif
