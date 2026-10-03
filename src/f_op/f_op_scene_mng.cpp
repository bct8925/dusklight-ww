/**
 * f_op_scene_mng.cpp
 * Framework - Scene Process Manager
 */

#include "f_op/f_op_scene_mng.h"
#include "f_op/f_op_scene_iter.h"
#include "f_op/f_op_scene_req.h"
#include "f_pc/f_pc_searcher.h"
#include "JSystem/JUtility/JUTAssert.h"
#if TARGET_PC
#include "dusk/trace.h"
#endif

scene_class* fopScnM_SearchByID(fpc_ProcID id) {
    return (scene_class*)fopScnIt_Judge((fop_ScnItFunc)fpcSch_JudgeByID, &id);
}

static uint l_scnRqID = -1;

BOOL fopScnM_ChangeReq(scene_class* i_scene, s16 procName, s16 fadeProcName, u16 fadePeekTime) {
    uint sceneRequestID = fopScnRq_Request(2, i_scene, procName, 0, fadeProcName, fadePeekTime);
#if TARGET_PC
    DUSK_TRACE("scene change -> %s (fade %s): %s", dusk::TraceProcName(procName),
               dusk::TraceProcName(fadeProcName), sceneRequestID == (uint)-1 ? "refused" : "requested");
#endif

    if (sceneRequestID == -1) {
        return FALSE;
    }

    l_scnRqID = sceneRequestID;
    return TRUE;
}

BOOL fopScnM_DeleteReq(scene_class* i_scene) {
    uint sceneRequestID = fopScnRq_Request(1, i_scene, fpcNm_INVALID_e, 0, fpcNm_INVALID_e, 0);
    return sceneRequestID != -1;
}

BOOL fopScnM_CreateReq(s16 procName, s16 fadeProcName, u16 fadePeekTime, uintptr_t user) {  // user may be a pointer
    uint sceneRequestID = fopScnRq_Request(0, 0, procName, (void*)user, fadeProcName, fadePeekTime);
    return sceneRequestID != -1;
}

u32 fopScnM_ReRequest(s16 procName, uintptr_t user) {
    if (l_scnRqID == -1) {
        return 0;
    }

    return fopScnRq_ReRequest(l_scnRqID, procName, (void*)user);
}

void fopScnM_Management() {
    if (!fopScnRq_Handler())
        JUT_ASSERT(DEMO_SELECT(284, 326), FALSE);
}

void fopScnM_Init() {
}
