#pragma once

namespace RBX {
    // Returns the new entry point
    void* hotpatchHook(void* origFunction, void* hookFunction);
    void* hotpatchUnhook(void* pfn);
    bool hookingApiHooked();

}