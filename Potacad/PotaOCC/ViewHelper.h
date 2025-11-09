#pragma once

#include "NativeViewerHandle.h"

namespace PotaOCC
{
    struct NativeViewerHandle;

    // ✅ This is the C#-visible wrapper
    public ref class ViewHelperPublic
    {
    public:
        static void ResetView(System::IntPtr viewPtr);
    };

    // ✅ This stays as pure C++ native namespace (not C# visible)
    namespace ViewHelper
    {
        void ClearCreateEntity(NativeViewerHandle* native);
        void DrawCenterMarker(NativeViewerHandle* native, double x, double y, double z);
        void ClearCenterMarker(NativeViewerHandle* native);
    }
}