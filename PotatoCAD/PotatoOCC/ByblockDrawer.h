#pragma once
#include <vcclr.h>
using namespace System;

namespace PotatoOCC
{
    public ref class ByblockDrawer
    {
    public:
        // Function to draw a batch of BYBLOCK entities (e.g., multiple arcs)
        static array<int>^ DrawByblockBatch(
            IntPtr ctxPtr,
            array<double>^ x1, array<double>^ y1, array<double>^ z1,  // Start point coordinates
            array<double>^ x2, array<double>^ y2, array<double>^ z2,  // End point coordinates
            array<int>^ r, array<int>^ g, array<int>^ b,              // Color
            array<double>^ transparency);
    };
}