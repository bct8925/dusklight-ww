#include "dusk/logging.h"
#include "dusk/main.h"

#include <dolphin/dolphin.h>
#include <dolphin/gx.h>
#include <tracy/Tracy.hpp>

#include <condition_variable>
#include <memory>
#include <mutex>
#include <unordered_map>

#ifndef _WIN32
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#if __APPLE__
#include <mach/mach_time.h>
#endif
#endif

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#undef IN
#undef OUT
#endif

#if __APPLE__
static u64 MachToDolphinNum;
static u64 MachToDolphinDenom;
#elif _WIN32
static LARGE_INTEGER PerfFrequency;
static bool PerfInitialized = false;
#endif


// ==========================================================================
// General OS
// ==========================================================================

u32 OSGetConsoleType() {
    return OS_CONSOLE_RETAIL1;
}

u32 OSGetSoundMode() {
    return 2;
}

// ==========================================================================
// Message Queue (thread-safe implementation)
// ==========================================================================

// Side-table for native synchronization per OSMessageQueue
struct PCMessageQueueData {
    std::mutex mtx;
    std::condition_variable cvSend;     // Notified when space becomes available
    std::condition_variable cvReceive;  // Notified when a message arrives
};

// Lazy-initialized to avoid DLL static init crashes
static std::mutex& GetMsgQueueMapMutex() {
    static std::mutex mtx;
    return mtx;
}
static std::unordered_map<OSMessageQueue*, std::unique_ptr<PCMessageQueueData>>& GetMsgQueueMap() {
    static std::unordered_map<OSMessageQueue*, std::unique_ptr<PCMessageQueueData>> map;
    return map;
}

static PCMessageQueueData& GetMsgQueueData(OSMessageQueue* mq) {
    std::lock_guard<std::mutex> lock(GetMsgQueueMapMutex());
    auto& map = GetMsgQueueMap();
    auto it = map.find(mq);
    if (it == map.end()) {
        auto result = map.emplace(mq, std::make_unique<PCMessageQueueData>());
        return *result.first->second;
    }
    return *it->second;
}

static void ClearMsgQueueMap() {
    std::lock_guard<std::mutex> lock(GetMsgQueueMapMutex());
    auto& map = GetMsgQueueMap();
    for (auto & [_, value] : map) {
        value->cvReceive.notify_all();
        value->cvSend.notify_all();
    }
    map.clear();
}

void OSInitMessageQueue(OSMessageQueue* mq, OSMessage* msgArray, s32 msgCount) {
    if (!mq) return;
    mq->queueSend.head = mq->queueSend.tail = nullptr;
    mq->queueReceive.head = mq->queueReceive.tail = nullptr;
    mq->msgArray   = msgArray;
    mq->msgCount   = msgCount;
    mq->firstIndex = 0;
    mq->usedCount  = 0;
    GetMsgQueueData(mq);  // Ensure side-table entry exists
}

int OSSendMessage(OSMessageQueue* mq, void* msg, s32 flags) {
    if (!mq) return 0;

    PCMessageQueueData& data = GetMsgQueueData(mq);
    std::unique_lock<std::mutex> lock(data.mtx);

    if (mq->usedCount >= mq->msgCount) {
        if (flags == OS_MESSAGE_NOBLOCK) return 0;
        // BLOCK: wait until space is available
        data.cvSend.wait(lock, [mq] { return mq->usedCount < mq->msgCount || dusk::IsShuttingDown; });
    }
    if (dusk::IsShuttingDown) {
        return 0;
    }

    s32 idx = (mq->firstIndex + mq->usedCount) % mq->msgCount;
    ((OSMessage*)mq->msgArray)[idx] = msg;
    mq->usedCount++;

    data.cvReceive.notify_one();
    return 1;
}

BOOL OSReceiveMessage(OSMessageQueue* mq, OSMessage* msg, s32 flags) {
    if (!mq) return 0;

    PCMessageQueueData& data = GetMsgQueueData(mq);
    std::unique_lock<std::mutex> lock(data.mtx);

    if (mq->usedCount == 0) {
        if (flags == OS_MESSAGE_NOBLOCK) return 0;
        // BLOCK: wait until a message arrives
        data.cvReceive.wait(lock, [mq] { return mq->usedCount > 0 || dusk::IsShuttingDown; });
    }
    if (dusk::IsShuttingDown) {
        return 0;
    }

    if (msg) {
        *(OSMessage*)msg = ((OSMessage*)mq->msgArray)[mq->firstIndex];
    }
    mq->firstIndex = (mq->firstIndex + 1) % mq->msgCount;
    mq->usedCount--;

    data.cvSend.notify_one();
    return 1;
}

int OSJamMessage(OSMessageQueue* mq, void* msg, s32 flags) {
    if (!mq) return 0;

    PCMessageQueueData& data = GetMsgQueueData(mq);
    std::unique_lock<std::mutex> lock(data.mtx);

    if (mq->usedCount >= mq->msgCount) {
        if (flags == OS_MESSAGE_NOBLOCK) return 0;
        // BLOCK: wait until space is available
        data.cvSend.wait(lock, [mq] { return mq->usedCount < mq->msgCount || dusk::IsShuttingDown; });
    }
    if (dusk::IsShuttingDown) {
        return 0;
    }

    // Jam inserts at the front of the queue
    mq->firstIndex = (mq->firstIndex - 1 + mq->msgCount) % mq->msgCount;
    ((OSMessage*)mq->msgArray)[mq->firstIndex] = msg;
    mq->usedCount++;

    data.cvReceive.notify_one();
    return 1;
}

// ==========================================================================
// Remaining OS Stubs
// ==========================================================================

void OSSetSoundMode(u32 mode) {}

void OSCreateAlarm(OSAlarm* alarm) {}

void OSCancelAlarm(OSAlarm* alarm) {}

u16 OSGetFontEncode() { return 0; }

char* OSGetFontTexture(char* string, void** image, s32* x, s32* y, s32* width) { return 0; }
char* OSGetFontWidth(char* string, s32* width) { return 0; }

BOOL OSGetResetButtonState() { return FALSE; }
BOOL OSInitFont(OSFontHeader* fontData) { return FALSE; }
BOOL OSLink(OSModuleInfo* newModule, void* bss) { return TRUE; }

void ClearCondMap();
void OSResetSystem(int reset, u32 resetCode, BOOL forceMenu) {
    OSReport("[PC] OSResetSystem called (reset=%d, code=%u)\n", reset, resetCode);
    dusk::IsShuttingDown = true;
    ClearMsgQueueMap();
    ClearCondMap();
}

void OSSetStringTable(void* stringTable) {}
BOOL OSUnlink(OSModuleInfo* oldModule) { return FALSE; }

void OSSwitchFiberEx(__REGISTER u32 param_0, __REGISTER u32 param_1, __REGISTER u32 param_2,
                     __REGISTER u32 param_3, __REGISTER u32 code, __REGISTER u32 stack) {
    // On PC, call the function directly instead of switching stacks.
    // The PPC version switches to 'stack' and calls code(param_0, param_1).
    // Only caller is mDoPrintf_vprintf_Interrupt: OSSwitchFiberEx(fmt, args, 0, 0, vprintf, sp)
    typedef void (*Func2)(u32, u32);
    ((Func2)(uintptr_t)code)(param_0, param_1);
}

u32 __OSGetDIConfig() { return 0; }
u32 OSGetProgressiveMode(void) { return 0; }
u32 OSGetResetCode(void) { return 0; }
BOOL OSGetResetSwitchState() { return FALSE; }
BOOL OSLinkFixed(OSModuleInfo* newModule, void* bss) { return TRUE; }
void OSProtectRange(u32 chan, void* addr, u32 nBytes, u32 control) {}
void OSSetPeriodicAlarm(OSAlarm* alarm, OSTime start, OSTime period, OSAlarmHandler handler) {}
void OSSetProgressiveMode(u32 on) {}
void OSSetSaveRegion(void* start, void* end) {}
OSErrorHandler OSSetErrorHandler(OSError error, OSErrorHandler handler) { return NULL; }
void OSSetAlarm(OSAlarm* alarm, OSTime tick, OSAlarmHandler handler) {}

#pragma mark EXI

BOOL EXIDeselect(int chan) {
    STUB_LOG();
    return FALSE;
}

BOOL EXIDma(int chan, void* buffer, s32 size, int d, int e) {
    STUB_LOG();
    return FALSE;
}

BOOL EXIImm(int chan, u32* b, int c, int d, int e) {
    STUB_LOG();
    return FALSE;
}

BOOL EXILock(int chan, int b, int c) {
    STUB_LOG();
    return FALSE;
}

BOOL EXISelect(int chan, int b, int c) {
    STUB_LOG();
    return FALSE;
}

BOOL EXISync(int chan) {
    STUB_LOG();
    return FALSE;
}

BOOL EXIUnlock(int chan) {
    STUB_LOG();
    return FALSE;
}

// OS-related functions consolidated under "# pragma mark OS" further up

#pragma mark VI

// VI retrace emulation: on GameCube, the VI chip fires a hardware interrupt at
// every VSync (~60Hz). This triggers pre/post retrace callbacks, which in turn
// send messages to JUTVideo's message queue. waitForTick() blocks on that queue.
// On PC, we simulate this by calling VIWaitForRetrace() once per frame in the
// main loop, which increments the retrace counter and fires the callbacks.
static u32 sRetraceCount = 0;
static VIRetraceCallback sVIPreRetraceCallback = NULL;
static VIRetraceCallback sVIPostRetraceCallback = NULL;

extern "C" {

u32 VIGetRetraceCount() {
    return sRetraceCount;
}

u32 VIGetNextField() {
    return 0;
}

void VISetBlack(BOOL black) {
    STUB_LOG();
}

void VISetNextFrameBuffer(void* fb) {
    STUB_LOG();
}

void VIWaitForRetrace() {
    ZoneScoped;
    sRetraceCount++;
    if (sVIPreRetraceCallback) {
        sVIPreRetraceCallback(sRetraceCount);
    }
    if (sVIPostRetraceCallback) {
        sVIPostRetraceCallback(sRetraceCount);
    }
}

void* VIGetCurrentFrameBuffer(void) {
    return NULL;
}

u32 VIGetDTVStatus(void) {
    return 0;
}

void* VIGetNextFrameBuffer(void) {
    return NULL;
}

VIRetraceCallback VISetPostRetraceCallback(VIRetraceCallback callback) {
    VIRetraceCallback old = sVIPostRetraceCallback;
    sVIPostRetraceCallback = callback;
    return old;
}

VIRetraceCallback VISetPreRetraceCallback(VIRetraceCallback cb) {
    VIRetraceCallback old = sVIPreRetraceCallback;
    sVIPreRetraceCallback = cb;
    return old;
}

}  // extern "C"

#pragma mark AI
#include <dolphin/ai.h>
u32 AIGetDSPSampleRate(void) {
    STUB_LOG();
    return 48000;  // Default sample rate?
}

void AIInit(u8* stack) {
    STUB_LOG();
    // This function initializes the AI system, but we don't have any specific implementation here.
    // In a real scenario, it would set up the audio interface and prepare it for use.
}

void AIInitDMA(uintptr_t start_addr, u32 length) {
    STUB_LOG();
}

AIDCallback AIRegisterDMACallback(AIDCallback callback) {
    STUB_LOG();
    return callback;
}

void AISetDSPSampleRate(u32 rate) {
    // Should this link with the getsamplerate? this is very TODO
    STUB_LOG();
}

void AIStartDMA(void) {
    STUB_LOG();
}

void AIStopDMA(void) {
    STUB_LOG();
}

#pragma mark GX
#include <dolphin/gx.h>

void GXSetGPMetric(GXPerf0 perf0, GXPerf1 perf1) {
    STUB_LOG();
}
void GXReadGPMetric(u32* cnt0, u32* cnt1) {
    STUB_LOG();
}
void GXClearGPMetric(void) {
    STUB_LOG();
}
void GXReadMemMetric(u32* cp_req, u32* tc_req, u32* cpu_rd_req, u32* cpu_wr_req, u32* dsp_req,
                     u32* io_req, u32* vi_req, u32* pe_req, u32* rf_req, u32* fi_req) {
    STUB_LOG();
}
void GXClearMemMetric(void) {
    STUB_LOG();
}
void GXClearVCacheMetric(void) {
    STUB_LOG();
}
void GXReadPixMetric(u32* top_pixels_in, u32* top_pixels_out, u32* bot_pixels_in,
                     u32* bot_pixels_out, u32* clr_pixels_in, u32* copy_clks) {
    STUB_LOG();
}
void GXClearPixMetric(void) {
    STUB_LOG();
}
void GXSetVCacheMetric(GXVCachePerf attr) {
    STUB_LOG();
}
void GXReadVCacheMetric(u32* check, u32* miss, u32* stall) {
    STUB_LOG();
}
void GXSetDrawSync(u16 token) {
    STUB_LOG();
}
GXDrawSyncCallback GXSetDrawSyncCallback(GXDrawSyncCallback cb) {
    STUB_LOG();
    return cb;
}
void GXWaitDrawDone(void) {
    STUB_LOG();
}
void GXResetWriteGatherPipe(void) {
    STUB_LOG();
}

void GXAbortFrame(void) {
    STUB_LOG();
}
// GXEnableTexOffsets: now provided by Aurora's GXGeometry.cpp (fifo branch)
OSThread* GXGetCurrentGXThread(void) {
    STUB_LOG();
    return NULL;
}
void* GXGetFifoBase(const GXFifoObj* fifo) {
    STUB_LOG();
    return NULL;
}
u32 GXGetFifoSize(const GXFifoObj* fifo) {
    STUB_LOG();
    return 0;
}
u16 GXGetNumXfbLines(u16 efbHeight, f32 yScale) {
    STUB_LOG();
    return 0;
}

f32 GXGetYScaleFactor(u16 efbHeight, u16 xfbHeight) {
    STUB_LOG();
    return 0.0f;
}

void GXInitTexCacheRegion(GXTexRegion* region, GXBool is_32b_mipmap, u32 tmem_even,
                          GXTexCacheSize size_even, u32 tmem_odd, GXTexCacheSize size_odd) {
    STUB_LOG();
}
// XXX, this should be some struct?
// GXRenderModeObj GXNtsc480IntDf;
//GXRenderModeObj GXNtsc480Int;
void GXReadXfRasMetric(u32* xf_wait_in, u32* xf_wait_out, u32* ras_busy, u32* clocks) {
    STUB_LOG();
    *xf_wait_in = 0;
    *xf_wait_out = 0;
    *ras_busy = 0;
    *clocks = 0;
}

void GXSetCopyClamp(GXFBClamp clamp) {
    STUB_LOG();
}
OSThread* GXSetCurrentGXThread(void) {
    STUB_LOG();
    return NULL;
}

void GXSetMisc(GXMiscToken token, u32 val) {
    STUB_LOG();
}

// Declared by aurora's SDK headers but not implemented there.
u32 OSGetConsoleSimulatedMemSize(void) {
    return 24 * 1024 * 1024;
}
void GXPokeAlphaRead(GXAlphaReadMode mode) {
    STUB_LOG();
}
void GXPeekARGB(u16 x, u16 y, u32* color) {
    STUB_LOG();
    *color = 0;
}

#pragma mark GBA
// The Game Boy Advance link (Tingle Tuner): no GBA is ever connected.
#include <dolphin/gba.h>
void GBAInit(void) {}
s32 GBAGetStatus(s32 chan, u8* status) {
    return GBA_NOT_READY;
}
s32 GBAReset(s32 chan, u8* status) {
    return GBA_NOT_READY;
}
s32 GBAGetProcessStatus(s32 chan, u8* percentp) {
    return GBA_NOT_READY;
}
s32 GBARead(s32 chan, u8* dst, u8* status) {
    return GBA_NOT_READY;
}
s32 GBAWrite(s32 chan, u8* src, u8* status) {
    return GBA_NOT_READY;
}
s32 GBAJoyBoot(s32 chan, s32 palette_color, s32 palette_speed, u8* programp, s32 length, u8* status) {
    return GBA_NOT_READY;
}

#pragma mark PPC Arch
// MSR stuff?
void PPCHalt() {
    abort();
}

extern "C" void PPCSync(void) {
    // Does nothing on PC
}

u32 PPCMfhid2() {
    STUB_LOG();
    return 0;
}

u32 PPCMfmsr() {
    STUB_LOG();
    return 0;
}

void PPCMtmsr(u32 newMSR) {
    STUB_LOG();
}

#pragma mark HIO
#include <dolphin/hio.h>
#include <dolphin/hio2.h>
BOOL HIO2Close(s32 handle) {
    STUB_LOG();
    return FALSE;
}

BOOL HIO2EnumDevices(HIO2EnumCallback callback) {
    STUB_LOG();
    return FALSE;
}

BOOL HIO2Init(void) {
    STUB_LOG();
    return FALSE;
}

s32 HIO2Open(HIO2DeviceType type, HIO2UnkCallback exiCb, HIO2DisconnectCallback disconnectCb) {
    STUB_LOG();
    return 0;
}

BOOL HIO2Read(s32 handle, u32 addr, void* buffer, s32 size) {
    STUB_LOG();
    return FALSE;
}

BOOL HIO2Write(s32 handle, u32 addr, void* buffer, s32 size) {
    STUB_LOG();
    return FALSE;
}

BOOL HIORead(u32 addr, void* buffer, s32 size) {
    STUB_LOG();
    return FALSE;
}

BOOL HIOWrite(u32 addr, void* buffer, s32 size) {
    STUB_LOG();
    return FALSE;
}
