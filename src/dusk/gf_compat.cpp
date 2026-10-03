// Wind Waker's GF ("fast GX") functions that aurora does not provide, implemented with the
// equivalent GX calls. See sdk_compat/pc_gf_compat.h.

#include "pc_gf_compat.h"

#include "dusk/logging.h"

extern "C" {

void GFSetVtxDescv(GXVtxDescList* list) {
    // The GF version writes the whole vertex descriptor, so attributes missing from the list are
    // off. GXSetVtxDescv only updates the listed ones; clear first.
    GXClearVtxDesc();
    GXSetVtxDescv(list);
}

void GFSetVtxAttrFmtv(GXVtxFmt fmt, GXVtxAttrFmtList* list) {
    GXSetVtxAttrFmtv(fmt, list);
}

void GFSetCullMode(GXCullMode mode) {
    GXSetCullMode(mode);
}

void GFSetChanMatColor(GXChannelID chan, GXColor color) {
    GXSetChanMatColor(chan, color);
}

void GFSetDstAlpha(u8 enable, u8 alpha) {
    GXSetDstAlpha(enable, alpha);
}

void GFSetTevColor(GXTevRegID reg, GXColor color) {
    GXSetTevColor(reg, color);
}

void GFSetAlphaCompare(GXCompare comp0, u8 ref0, GXAlphaOp op, GXCompare comp1, u8 ref1) {
    GXSetAlphaCompare(comp0, ref0, op, comp1, ref1);
}

void GFLoadPosMtxImm(MtxP mtx, u32 id) {
    GXLoadPosMtxImm(mtx, id);
}

void GFLoadNrmMtxImm(MtxP mtx, u32 id) {
    GXLoadNrmMtxImm(mtx, id);
}

void GFSetCurrentMtx(u32 pn, u32 t0, u32 t1, u32 t2, u32 t3, u32 t4, u32 t5, u32 t6, u32 t7) {
    // The GameCube version also sets the 8 texture matrix indices. Every caller in the game
    // passes GX_IDENTITY for them, which is all this supports.
    const u32 tex[] = {t0, t1, t2, t3, t4, t5, t6, t7};
    for (u32 t : tex) {
        if (t != GX_IDENTITY) {
            DuskLog.error("GFSetCurrentMtx: texture matrix index {} is not supported", t);
            break;
        }
    }
    GXSetCurrentMtx(pn);
}

void GFSetArray(GXAttr attr, const void* data, u32 size, u8 stride, bool le) {
    GXSetArray(attr, data, size, stride, le);
}

}  // extern "C"
