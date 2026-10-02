#ifndef JSUPPORT_H
#define JSUPPORT_H

template <typename T>
T* JSUConvertOffsetToPtr(const void* ptr, u32 offset) {
    if (offset == NULL) {
        return NULL;
    } else {
#if TARGET_PC
        return (T*)((intptr_t)ptr + offset);  // (s32) truncates 64-bit pointers
#else
        return (T*)((s32)ptr + offset);
#endif
    }
}

template <typename T>
T* JSUConvertOffsetToPtr(const void* ptr, const void* offset) {
    if (offset == NULL) {
        return NULL;
    } else {
#if TARGET_PC
        return (T*)((intptr_t)ptr + (intptr_t)offset);
#else
        return (T*)((s32)ptr + (s32)offset);
#endif
    }
}

inline u8 JSULoNibble(u8 param_0) { return param_0 & 0x0f; }
inline u8 JSUHiNibble(u8 param_0) {return (param_0 & 0xff) >> 4; }

inline u8 JSULoByte(u16 in) {
    return in & 0xff;
}

inline u8 JSUHiByte(u16 in) {
    return in >> 8;
}

inline u16 JSULoHalf(u32 in) {
    return in;
}

inline u16 JSUHiHalf(u32 in) {
    return (in >> 16);
}

#endif
