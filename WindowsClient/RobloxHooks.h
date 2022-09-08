#pragma once

namespace RBX
{
    typedef BOOLEAN (NTAPI *RtlDispatchExceptionPfn)(PEXCEPTION_RECORD exRec, PCONTEXT ctx);
    // extern RtlDispatchExceptionPfn vehHookContinue; // moved to App because this needs to be checked.
    extern DWORD* vehHookLocation;
    BOOLEAN RtlDispatchExceptionHook(PEXCEPTION_RECORD exRec, PCONTEXT ctx);

    void hookApi();
    void unhookApi();
    bool hookPreVeh();
}