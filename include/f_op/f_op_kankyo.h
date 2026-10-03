#ifndef F_OP_KANKYO_H
#define F_OP_KANKYO_H

#include "SSystem/SComponent/c_xyz.h"
#include "f_pc/f_pc_leaf.h"

struct kankyo_method_class {
    /* 0x00 */ leafdraw_method_class base;
};

#if TARGET_PC
struct kankyo_class : public leafdraw_class {
    PC_BASE_MEMBER(leafdraw_class, base)
#else
struct kankyo_class {
    /* 0x00 */ leafdraw_class base;
#endif
    /* 0xC0 */ int mBsType;
    /* 0xC4 */ create_tag_class draw_tag;
    /* 0xD8 */ kankyo_method_class* sub_method;
    /* 0xDC */ cXyz mPos;
    /* 0xE8 */ cXyz mScale;
    /* 0xF4 */ u32 mParam;
};

struct kankyo_process_profile_definition {
    /* 0x00 */ leaf_process_profile_definition base;
    /* 0x24 */ kankyo_method_class* sub_method;
};  // Size: 0x28

#if TARGET_PC
struct sub_kankyo__class : public kankyo_class {
    PC_BASE_MEMBER(kankyo_class, base)
#else
struct sub_kankyo__class {
    /* 0x00 */ kankyo_class base;
#endif
};

extern leafdraw_method_class g_fopKy_Method;

#endif /* F_OP_KANKYO_H */
