#ifndef D_A_OBJ_VDS_H
#define D_A_OBJ_VDS_H

#include "f_op/f_op_actor.h"
#include "f_op/f_op_actor_mng.h"
#include "m_Do/m_Do_ext.h"
#include "d/d_a_obj.h"
#include "d/d_kankyo.h"

class J3DAnmTransformKey;
class J3DAnmTevRegKey;
class dBgW;

namespace daObjVds {
    static void* ds_search_switchCB(void*, void*);

    class Act_c : public fopAc_ac_c {
    public:
        struct Attr_c {
            /* 0x00 */ f32 m00;
            /* 0x04 */ f32 m04;
            /* 0x08 */ f32 m08;
            /* 0x0C */ s16 m0C;
            /* 0x0E */ s16 m0E;
            /* 0x10 */ s16 m10;
            /* 0x14 */ f32 m14;
            /* 0x18 */ f32 m18;
            /* 0x1C */ f32 m1C;
            /* 0x20 */ f32 m20;
            /* 0x24 */ f32 m24;
        };  // Size: 0x28

        enum Prm_e {
            PRM_SWSAVE_W = 0x08,
            PRM_SWSAVE_S = 0x00,
        };

        virtual ~Act_c() {}

        s32 prm_get_swSave() const { return daObj::PrmAbstract<int>(this, PRM_SWSAVE_W, PRM_SWSAVE_S); }
        BOOL is_switch() const { return fopAcM_isSwitch(const_cast<Act_c*>(this), prm_get_swSave()); }

        BOOL SetLoopJointAnimation(J3DAnmTransformKey*, J3DAnmTransformKey*, f32, f32);
        BOOL PlayLoopJointAnimation();
        void set_first_process();
        void* search_switchCB(fopAc_ac_c*);
        void stripped_eye_pos(cXyz*, int);
        BOOL process_off_init();
        void process_off_main();
        BOOL process_on_init();
        void process_on_main();
        BOOL process_init(int);
        void process_main();
        void process_common();
        void create_point_light(int, cXyz*);
        void execute_point_light();
        void delete_point_light();
        void Event_init();
        void Event_exe();
        static BOOL solidHeapCB(fopAc_ac_c*);
        bool create_heap();
        cPhs_State _create();
        bool _delete();
        void set_mtx();
        bool _execute();
        void stripped_debug_color(GXColor*);
        bool _draw();

        static const char M_arcname[];

    public:
        /* 0x290 */ // vtbl
        /* 0x294 */ request_of_phase_process_class mPhs;
        /* 0x29C */ Mtx mMtx;
        /* 0x2CC */ mDoExt_McaMorf* M_anm0;
        /* 0x2D0 */ J3DAnmTransformKey* M_bck_data0;
        /* 0x2D4 */ mDoExt_brkAnm mBrk0;
        /* 0x2EC */ J3DAnmTevRegKey* M_brk_data0;
        /* 0x2F0 */ mDoExt_McaMorf* M_anm1;
        /* 0x2F4 */ J3DAnmTransformKey* M_bck_data1;
        /* 0x2F8 */ mDoExt_brkAnm mBrk1;
        /* 0x310 */ J3DAnmTevRegKey* M_brk_data1;
        /* 0x314 */ dBgW* mpBgW;
        /* 0x318 */ s32 field_0x318;
        /* 0x31C */ s32 field_0x31C;
        /* 0x320 */ s32 field_0x320;
        /* 0x324 */ fpc_ProcID mSwitchId[2];
        /* 0x32C */ f32 mPower[2];
        /* 0x334 */ s16 field_0x334;
        /* 0x336 */ s16 field_0x336;
        /* 0x338 */ s16 field_0x338;
        /* 0x33A */ u8 field_0x33A[0x33C - 0x33A];
        /* 0x33C */ LIGHT_INFLUENCE mLight[2];
        /* 0x37C */ cXyz mLightPos[2];
    };  // Size: 0x394
};

STATIC_ASSERT(sizeof(daObjVds::Act_c) == 0x394);

#endif /* D_A_OBJ_VDS_H */
