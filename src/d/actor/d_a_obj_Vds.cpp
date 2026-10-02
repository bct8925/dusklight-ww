/**
 * d_a_obj_Vds.cpp
 * Object - Earth Temple - Face statue (lights up when eyes are hit with Mirror Shield)
 */

#include "d/dolzel_rel.h" // IWYU pragma: keep
#include "d/actor/d_a_obj_Vds.h"
#include "d/actor/d_a_obj_swlight.h"
#include "res/Object/Vds.h"
#include "d/d_com_inf_game.h"
#include "d/d_bg_w.h"
#include "m_Do/m_Do_mtx.h"
#include "JSystem/JUtility/JUTAssert.h"

namespace daObjVds {
    namespace {
        static const Act_c::Attr_c L_attr = {
            222.5f, 598.73f, 178.9f,
            0x400, 0x400, 0x400,
            2200.0f,
            0.0f, 0.0f, 0.0f, 0.0f,
        };
    }
}

/* 00000078-00000134       .text SetLoopJointAnimation__Q28daObjVds5Act_cFP18J3DAnmTransformKeyP18J3DAnmTransformKeyff */
BOOL daObjVds::Act_c::SetLoopJointAnimation(J3DAnmTransformKey* i_bck0, J3DAnmTransformKey* i_bck1, f32 i_speed, f32 i_morf) {
    M_anm0->setAnm(i_bck0, J3DFrameCtrl::EMode_LOOP, i_morf, i_speed, 0.0f, -1.0f, NULL);
    M_anm1->setAnm(i_bck1, J3DFrameCtrl::EMode_LOOP, i_morf, i_speed, 0.0f, -1.0f, NULL);
    field_0x318 = 1;
    return TRUE;
}

/* 00000134-00000188       .text PlayLoopJointAnimation__Q28daObjVds5Act_cFv */
BOOL daObjVds::Act_c::PlayLoopJointAnimation() {
    M_anm0->play(NULL, 0, 0);
    M_anm1->play(NULL, 0, 0);
    return TRUE;
}

/* 00000188-000001E8       .text set_first_process__Q28daObjVds5Act_cFv */
void daObjVds::Act_c::set_first_process() {
    process_init(is_switch() ? 1 : 0);
}

/* 000001E8-00000214       .text ds_search_switchCB__8daObjVdsFPvPv */
void* daObjVds::ds_search_switchCB(void* i_actor, void* i_this) {
    return ((Act_c*)i_this)->search_switchCB((fopAc_ac_c*)i_actor);
}

/* 00000214-000002B0       .text search_switchCB__Q28daObjVds5Act_cFP10fopAc_ac_c */
void* daObjVds::Act_c::search_switchCB(fopAc_ac_c* i_actor) {
    if (fopAcM_IsActor(i_actor) && fopAcM_GetName(i_actor) == fpcNm_Obj_Swlight_e) {
        for (int i = 0; i < 2; i++) {
            if (mSwitchId[i] == fpcM_ERROR_PROCESS_ID_e) {
                mSwitchId[i] = fopAcM_GetID(i_actor);
                break;
            }
        }
    }
    return NULL;
}

// The original object had a function here that the linker stripped. Its code is lost, but its
// data (L_attr, the eye position constants and angle_data) survives, so recreate it to match.
void daObjVds::Act_c::stripped_eye_pos(cXyz* o_pos, int i_no) {
    static s16 angle_data[] = {0, 0};
    cXyz left(-222.5f, 598.73f, 178.9f);
    cXyz right(222.5f, 598.73f, 178.9f);
    *o_pos = i_no == 0 ? left : right;
    o_pos->y += L_attr.m14;
    angle_data[i_no]++;
}

/* 000002EC-000003C8       .text process_off_init__Q28daObjVds5Act_cFv */
BOOL daObjVds::Act_c::process_off_init() {
    if (mBrk0.init(M_anm0->getModel()->getModelData(), M_brk_data0, TRUE, J3DFrameCtrl::EMode_NONE, 1.0f, 0, -1, true, FALSE)) {
        mBrk0.setPlaySpeed(0.0f);
        if (mBrk1.init(M_anm1->getModel()->getModelData(), M_brk_data1, TRUE, J3DFrameCtrl::EMode_NONE, 1.0f, 0, -1, true, FALSE)) {
            mBrk1.setPlaySpeed(0.0f);
            return TRUE;
        }
    }
    return FALSE;
}

/* 000003C8-00000474       .text process_off_main__Q28daObjVds5Act_cFv */
void daObjVds::Act_c::process_off_main() {
    for (int i = 0; i < 2; i++) {
        daObjSwlight::Act_c* swlight = (daObjSwlight::Act_c*)fopAcM_SearchByID(mSwitchId[i]);
        mPower[i] = swlight->get_power();
    }

    if (is_switch()) {
        process_init(1);
    }
}

/* 00000474-000004F0       .text process_on_init__Q28daObjVds5Act_cFv */
BOOL daObjVds::Act_c::process_on_init() {
    if (SetLoopJointAnimation(M_bck_data0, M_bck_data1, 1.0f, 0.0f)) {
        for (int i = 0; i < 2; i++) {
            mPower[i] = 1.0f;
        }
        return TRUE;
    }
    return FALSE;
}

/* 000004F0-000004F4       .text process_on_main__Q28daObjVds5Act_cFv */
void daObjVds::Act_c::process_on_main() {
}

/* 000004F4-000005C0       .text process_init__Q28daObjVds5Act_cFi */
BOOL daObjVds::Act_c::process_init(int i_process) {
    typedef BOOL (Act_c::*InitFunc)();
    static InitFunc init_table[] = {
        &Act_c::process_off_init,
        &Act_c::process_on_init,
    };

    if (i_process >= 0 && i_process < 2 && (this->*init_table[i_process])()) {
        field_0x31C = i_process;
        return TRUE;
    }
    return FALSE;
}

/* 000005C0-0000065C       .text process_main__Q28daObjVds5Act_cFv */
void daObjVds::Act_c::process_main() {
    typedef void (Act_c::*MainFunc)();
    static MainFunc main_table[] = {
        &Act_c::process_off_main,
        &Act_c::process_on_main,
    };

    if (field_0x31C >= 0 && field_0x31C < 2) {
        (this->*main_table[field_0x31C])();
    }
}

/* 0000065C-000007EC       .text process_common__Q28daObjVds5Act_cFv */
void daObjVds::Act_c::process_common() {
    switch (field_0x320) {
    case 0: {
        for (int i = 0; i < 2; i++) {
            mSwitchId[i] = fpcM_ERROR_PROCESS_ID_e;
        }
        fopAcIt_Judge(ds_search_switchCB, this);

        if (mSwitchId[0] != fpcM_ERROR_PROCESS_ID_e && mSwitchId[1] != fpcM_ERROR_PROCESS_ID_e) {
            fopAc_ac_c* swlight0 = fopAcM_SearchByID(mSwitchId[0]);
            fopAc_ac_c* swlight1 = fopAcM_SearchByID(mSwitchId[1]);
            if (swlight0 != NULL && swlight1 != NULL) {
                s16 angle_diff = swlight0->shape_angle.y - shape_angle.y;
                if (angle_diff >= 0) {
                    fpc_ProcID temp = mSwitchId[0];
                    mSwitchId[0] = mSwitchId[1];
                    mSwitchId[1] = temp;
                }
                field_0x320 = 1;
            }
        }
        break;
    }
    case 1: {
        fopAc_ac_c* swlight0 = fopAcM_SearchByID(mSwitchId[0]);
        fopAc_ac_c* swlight1 = fopAcM_SearchByID(mSwitchId[1]);
        if (swlight0 != NULL && swlight1 != NULL) {
            create_point_light(0, &swlight0->current.pos);
            create_point_light(1, &swlight1->current.pos);
            field_0x320 = 2;
        }
        break;
    }
    case 2:
        execute_point_light();
        break;
    }
}

/* 000007EC-0000087C       .text create_point_light__Q28daObjVds5Act_cFiP4cXyz */
void daObjVds::Act_c::create_point_light(int i_no, cXyz* i_pos) {
    int idx = i_no & 1;
    mLight[idx].mPos = *i_pos;
    mLightPos[idx] = *i_pos;
    mLight[idx].mColor.r = 0x400;
    mLight[idx].mColor.g = 0x400;
    mLight[idx].mColor.b = 0x400;
    mLight[idx].mPower = 0.0f;
    mLight[idx].mFluctuation = 0.0f;
    dKy_plight_set(&mLight[idx]);
}

/* 0000087C-00000900       .text execute_point_light__Q28daObjVds5Act_cFv */
void daObjVds::Act_c::execute_point_light() {
    for (int i = 0; i < 2; i++) {
        mLight[i].mPower = 2200.0f * mPower[i];
        cXyz pos(mLightPos[i].x, mLightPos[i].y, mLightPos[i].z);
        mLight[i].mPos = pos;
        mLight[i].mColor.r = 0x400;
        mLight[i].mColor.g = 0x400;
        mLight[i].mColor.b = 0x400;
    }
}

/* 00000900-00000954       .text delete_point_light__Q28daObjVds5Act_cFv */
void daObjVds::Act_c::delete_point_light() {
    for (int i = 0; i < 2; i++) {
        dKy_plight_cut(&mLight[i]);
    }
}

/* 00000954-00000968       .text Event_init__Q28daObjVds5Act_cFv */
void daObjVds::Act_c::Event_init() {
    field_0x334 = -1;
    field_0x336 = 0;
}

/* 00000968-00000A28       .text Event_exe__Q28daObjVds5Act_cFv */
void daObjVds::Act_c::Event_exe() {
    switch (field_0x336) {
    case 1:
        if (eventInfo.checkCommandDemoAccrpt()) {
            field_0x336 = 2;
        } else {
            fopAcM_orderOtherEventId(this, field_0x334);
            eventInfo.onCondition(dEvtCnd_UNK2_e);
        }
        break;
    case 2:
        if (dComIfGp_evmng_endCheck(field_0x334)) {
            dComIfGp_event_reset();
            Event_init();
        }
        break;
    }
}

const char daObjVds::Act_c::M_arcname[] = "Vds";

/* 00000A28-00000A4C       .text solidHeapCB__Q28daObjVds5Act_cFP10fopAc_ac_c */
BOOL daObjVds::Act_c::solidHeapCB(fopAc_ac_c* i_this) {
    return ((Act_c*)i_this)->create_heap();
}

/* 00000A4C-00001020       .text create_heap__Q28daObjVds5Act_cFv */
bool daObjVds::Act_c::create_heap() {
    J3DModelData* mdl_data0 = (J3DModelData*)dComIfG_getObjectRes(M_arcname, dRes_INDEX_VDS_BDL_VDSWT0_e);
    JUT_ASSERT(848, mdl_data0 != 0);
    M_bck_data0 = (J3DAnmTransformKey*)dComIfG_getObjectRes(M_arcname, dRes_INDEX_VDS_BCK_VDSWT0_e);
    JUT_ASSERT(852, M_bck_data0 != 0);

    if (mdl_data0 != NULL && M_bck_data0 != NULL) {
        M_anm0 = new mDoExt_McaMorf(
            mdl_data0,
            NULL, NULL,
            M_bck_data0,
            J3DFrameCtrl::EMode_NONE, 1.0f, 0, -1, 1,
            NULL,
            0x00000000,
            0x11020203
        );
    }
    JUT_ASSERT(865, M_anm0 != 0);

    J3DModelData* mdl_data1 = (J3DModelData*)dComIfG_getObjectRes(M_arcname, dRes_INDEX_VDS_BDL_VDSWT1_e);
    JUT_ASSERT(869, mdl_data1 != 0);
    M_bck_data1 = (J3DAnmTransformKey*)dComIfG_getObjectRes(M_arcname, dRes_INDEX_VDS_BCK_VDSWT1_e);
    JUT_ASSERT(873, M_bck_data1 != 0);

    if (mdl_data1 != NULL && M_bck_data1 != NULL) {
        M_anm1 = new mDoExt_McaMorf(
            mdl_data1,
            NULL, NULL,
            M_bck_data1,
            J3DFrameCtrl::EMode_NONE, 1.0f, 0, -1, 1,
            NULL,
            0x00000000,
            0x11020203
        );
    }
    JUT_ASSERT(886, M_anm1 != 0);

    M_brk_data0 = (J3DAnmTevRegKey*)dComIfG_getObjectRes(M_arcname, dRes_INDEX_VDS_BRK_VDSWT0_e);
    JUT_ASSERT(891, M_brk_data0 != 0);
    int brk_ret0 = mBrk0.init(mdl_data0, M_brk_data0, TRUE, J3DFrameCtrl::EMode_NONE);

    M_brk_data1 = (J3DAnmTevRegKey*)dComIfG_getObjectRes(M_arcname, dRes_INDEX_VDS_BRK_VDSWT1_e);
    JUT_ASSERT(904, M_brk_data1 != 0);
    int brk_ret1 = mBrk1.init(mdl_data1, M_brk_data1, TRUE, J3DFrameCtrl::EMode_NONE);

    set_mtx();

    cBgD_t* bgw_data = (cBgD_t*)dComIfG_getObjectRes(M_arcname, dRes_INDEX_VDS_DZB_VDSWT_e);
    JUT_ASSERT(926, bgw_data != 0);
    if (bgw_data != NULL) {
        mpBgW = new dBgW();
        if (mpBgW != NULL) {
            mpBgW->Set(bgw_data, dBgW::MOVE_BG_e, &mMtx);
        }
    }

    return M_bck_data0 != NULL && M_anm0 != NULL && M_anm0->getModel() != NULL &&
        M_bck_data1 != NULL && M_anm1 != NULL && M_anm1->getModel() != NULL &&
        mpBgW != NULL && M_brk_data0 != NULL && M_brk_data1 != NULL &&
        brk_ret0 && brk_ret1;
}

/* 00001020-000011EC       .text _create__Q28daObjVds5Act_cFv */
cPhs_State daObjVds::Act_c::_create() {
    fopAcM_ct(this, Act_c);

    cPhs_State ret = dComIfG_resLoad(&mPhs, M_arcname);

    if (ret == cPhs_COMPLEATE_e) {
        if (fopAcM_entrySolidHeap(this, solidHeapCB, 0)) {
            set_first_process();
            fopAcM_SetMtx(this, M_anm0->getModel()->getBaseTRMtx());
            fopAcM_setCullSizeBox(this, -2000.0f, -2000.0f, -2000.0f, 2000.0f, 2000.0f, 2000.0f);
            dComIfG_Bgsp()->Regist(mpBgW, this);
            mpBgW->SetCrrFunc(NULL);
            Event_init();
            field_0x338 = dComIfGp_evmng_getEventIdx("Vds");
            for (int i = 0; i < 2; i++) {
                mSwitchId[i] = fpcM_ERROR_PROCESS_ID_e;
            }
        } else {
            ret = cPhs_ERROR_e;
        }
    }

    return ret;
}

/* 000012D4-00001368       .text _delete__Q28daObjVds5Act_cFv */
bool daObjVds::Act_c::_delete() {
    if (heap != NULL && mpBgW != NULL && mpBgW->ChkUsed()) {
        dComIfG_Bgsp()->Release(mpBgW);
    }
    delete_point_light();
    dComIfG_resDelete(&mPhs, M_arcname);
    return true;
}

/* 00001368-00001420       .text set_mtx__Q28daObjVds5Act_cFv */
void daObjVds::Act_c::set_mtx() {
    M_anm0->getModel()->setBaseScale(scale);
    mDoMtx_stack_c::transS(current.pos);
    mDoMtx_stack_c::ZXYrotM(shape_angle);
    M_anm0->getModel()->setBaseTRMtx(mDoMtx_stack_c::get());
    M_anm1->getModel()->setBaseTRMtx(mDoMtx_stack_c::get());
    mDoMtx_copy(mDoMtx_stack_c::get(), mMtx);
}

/* 00001420-000014EC       .text _execute__Q28daObjVds5Act_cFv */
bool daObjVds::Act_c::_execute() {
    Event_exe();
    PlayLoopJointAnimation();
    mBrk0.setFrame(mPower[0] * (M_brk_data0->getFrameMax() - 1));
    mBrk1.setFrame(mPower[1] * (M_brk_data1->getFrameMax() - 1));
    process_common();
    process_main();
    set_mtx();
    if (mpBgW != NULL) {
        mpBgW->Move();
    }
    return true;
}

// Another stripped function, which only left behind this color.
void daObjVds::Act_c::stripped_debug_color(GXColor* o_color) {
    GXColor color = {0xFF, 0x00, 0x00, 0x80};
    *o_color = color;
}

/* 000014EC-000015A8       .text _draw__Q28daObjVds5Act_cFv */
bool daObjVds::Act_c::_draw() {
    g_env_light.settingTevStruct(TEV_TYPE_BG0, &current.pos, &tevStr);
    g_env_light.setLightTevColorType(M_anm0->getModel(), &tevStr);
    g_env_light.setLightTevColorType(M_anm1->getModel(), &tevStr);
    mBrk0.entry(M_anm0->getModel()->getModelData());
    mBrk1.entry(M_anm1->getModel()->getModelData());
    M_anm0->updateDL();
    M_anm1->updateDL();
    return true;
}

namespace daObjVds {
namespace {
/* 000015A8-000015C8       .text Mthd_Create__Q28daObjVds25@unnamed@d_a_obj_Vds_cpp@FPv */
cPhs_State Mthd_Create(void* i_this) {
    return ((daObjVds::Act_c*)i_this)->_create();
}

/* 000015C8-000015EC       .text Mthd_Delete__Q28daObjVds25@unnamed@d_a_obj_Vds_cpp@FPv */
BOOL Mthd_Delete(void* i_this) {
    return ((daObjVds::Act_c*)i_this)->_delete();
}

/* 000015EC-00001610       .text Mthd_Execute__Q28daObjVds25@unnamed@d_a_obj_Vds_cpp@FPv */
BOOL Mthd_Execute(void* i_this) {
    return ((daObjVds::Act_c*)i_this)->_execute();
}

/* 00001610-00001634       .text Mthd_Draw__Q28daObjVds25@unnamed@d_a_obj_Vds_cpp@FPv */
BOOL Mthd_Draw(void* i_this) {
    return ((daObjVds::Act_c*)i_this)->_draw();
}

/* 00001634-0000163C       .text Mthd_IsDelete__Q28daObjVds25@unnamed@d_a_obj_Vds_cpp@FPv */
BOOL Mthd_IsDelete(void*) {
    return TRUE;
}

static actor_method_class Mthd_Table = {
    (process_method_func)Mthd_Create,
    (process_method_func)Mthd_Delete,
    (process_method_func)Mthd_Execute,
    (process_method_func)Mthd_IsDelete,
    (process_method_func)Mthd_Draw,
};
}; // namespace
}; // namespace daObjVds

actor_process_profile_definition g_profile_Obj_Vds = {
    /* Layer ID     */ fpcLy_CURRENT_e,
    /* List ID      */ 0x0003,
    /* List Prio    */ fpcPi_CURRENT_e,
    /* Proc Name    */ fpcNm_Obj_Vds_e,
    /* Proc SubMtd  */ &g_fpcLf_Method.base,
    /* Size         */ sizeof(daObjVds::Act_c),
    /* Size Other   */ 0,
    /* Parameters   */ 0,
    /* Leaf SubMtd  */ &g_fopAc_Method.base,
    /* Draw Prio    */ fpcDwPi_Obj_Vds_e,
    /* Actor SubMtd */ &daObjVds::Mthd_Table,
    /* Status       */ fopAcStts_CULL_e | fopAcStts_UNK40000_e,
    /* Group        */ fopAc_ACTOR_e,
    /* Cull Type    */ fopAc_CULLBOX_CUSTOM_e,
};
