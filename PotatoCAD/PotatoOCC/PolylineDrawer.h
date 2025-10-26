#pragma once
#include <vcclr.h>
using namespace System;

namespace PotatoOCC
{
    public ref class PolylineDrawer
    {
    public:
        // Function to draw a batch of POLYLINE entities (multiple polylines)
        static array<int>^ DrawPolylineBatch(
            IntPtr ctxPtr,
            array<double>^ x, array<double>^ y, array<double>^ z,        // Coordinates of the polyline vertices
            array<int>^ r, array<int>^ g, array<int>^ b,                  // Color
            array<double>^ transparency,                                  // Transparency
            array<bool>^ closed);                                         // If polyline is closed (closed = true)
    };
}