// GF ("fast GX") functions Wind Waker uses that aurora's GF headers do not declare. On the
// GameCube these write raw commands into the GX FIFO; on PC they are implemented with the
// equivalent GX calls (src/dusk/gf_compat.cpp).
#ifndef SDK_COMPAT_PC_GF_COMPAT_H
#define SDK_COMPAT_PC_GF_COMPAT_H

#include <dolphin/gf.h>
#include <dolphin/gx.h>
#ifndef __cplusplus
#include <stdbool.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

void GFSetVtxDescv(GXVtxDescList* list);
void GFSetVtxAttrFmtv(GXVtxFmt fmt, GXVtxAttrFmtList* list);
void GFSetCullMode(GXCullMode mode);
void GFSetChanMatColor(GXChannelID chan, GXColor color);
void GFSetDstAlpha(u8 enable, u8 alpha);
void GFSetTevColor(GXTevRegID reg, GXColor color);
void GFSetAlphaCompare(GXCompare comp0, u8 ref0, GXAlphaOp op, GXCompare comp1, u8 ref1);
void GFLoadPosMtxImm(MtxP mtx, u32 id);
void GFLoadNrmMtxImm(MtxP mtx, u32 id);
void GFSetCurrentMtx(u32 pn, u32 t0, u32 t1, u32 t2, u32 t3, u32 t4, u32 t5, u32 t6, u32 t7);

// Like GXSetArray on PC, the array's size and endianness are needed (see GXSETARRAY).
void GFSetArray(GXAttr attr, const void* data, u32 size, u8 stride, bool le);
#define GFSETARRAY(attr, data, size, stride, le) GFSetArray((attr), (data), (size), (stride), (le))

static inline void GFBegin(GXPrimitive type, GXVtxFmt fmt, u16 nverts) {
    GXBegin(type, fmt, nverts);
}
// GFEnd is empty on the GameCube; aurora's GXEnd submits the draw, so it must be called.
static inline void GFEnd(void) {
    GXEnd();
}
static inline void GFPosition3f32(f32 x, f32 y, f32 z) {
    GXPosition3f32(x, y, z);
}
static inline void GFTexCoord2s16(s16 u, s16 v) {
    GXTexCoord2s16(u, v);
}

#ifdef __cplusplus
}
#endif

#endif
