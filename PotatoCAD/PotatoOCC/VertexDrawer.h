#pragma once
#include <vcclr.h>
using namespace System;

namespace PotatoOCC
{
    public ref class VertexDrawer
    {
    public:
        // Function to draw a batch of VERTEX entities (e.g., multiple points)
        static array<int>^ DrawVertexBatch(
            IntPtr ctxPtr,
            array<double>^ x, array<double>^ y, array<double>^ z,    // Coordinates of the vertex
            array<int>^ r, array<int>^ g, array<int>^ b,              // Color
            array<double>^ transparency);                             // Transparency
    };
}