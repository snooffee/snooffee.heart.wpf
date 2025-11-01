#pragma once
#include "NativeViewerHandle.h"
namespace PotaOCC {
    public ref class ViewHelper
    {
    public:
        static void ResetView(System::IntPtr viewPtr);

        // Clears any entities like lines, circles, and resets flags
        static void ClearCreateEntity(NativeViewerHandle* native);

        // Draw a center marker at a given point (x, y, z) in the 3D view
        static void DrawCenterMarker(NativeViewerHandle* native, double x, double y, double z);

        // Clears the center marker
        static void ClearCenterMarker(NativeViewerHandle* native);
    };
}