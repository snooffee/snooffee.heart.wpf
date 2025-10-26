#pragma once
#include <vcclr.h>

using namespace System;

namespace PotatoOCC {

    // Managed handle to store pointers to native OCCT objects
    public ref class ViewerHandle
    {
    public:
        IntPtr ViewPtr;     // V3d_View*
        IntPtr ContextPtr;  // AIS_InteractiveContext*
        IntPtr NativeHandle; // NativeViewerHandle*

        double PreviousPixelsPerUnit;

        // Store base heights for each label
        System::Collections::Generic::List<double>^ PixelHeights;

        ViewerHandle()
        {
            PreviousPixelsPerUnit = 1.0;
            PixelHeights = gcnew System::Collections::Generic::List<double>();
        }
    };

    public ref class ViewerManager
    {
    public:
        // Create a new viewer for a given HWND
        static ViewerHandle^ CreateViewer(IntPtr hwnd, int width, int height, Control^ hostControl);

        // Resize the viewer window
        static void ResizeViewer(IntPtr viewerHandlePtr, int width, int height);

        // Fit all displayed objects in view
        static void FitAll(IntPtr viewPtr);

        // Update/redraw the viewer
        static void UpdateView(IntPtr viewerHandlePtr, bool isDisposing);

        // Set background color
        static void SetBackgroundColor(IntPtr viewPtr, int r, int g, int b);

        // Dispose the viewer and free memory
        static void DisposeViewer(IntPtr viewerHandlePtr);

        static void Display(IntPtr nativeHandlePtr, Handle(AIS_InteractiveObject) shape, bool redraw);
        static void Remove(System::IntPtr contextPtr, System::IntPtr shapePtr);
        static void Redraw(System::IntPtr viewPtr);

        static void Convert(IntPtr viewPtr, int x, int y, double% X, double% Y, double% Z);

        static void WorldToScreen(System::IntPtr viewPtr, double wx, double wy, double wz, double* sx, double* sy);

    };
}