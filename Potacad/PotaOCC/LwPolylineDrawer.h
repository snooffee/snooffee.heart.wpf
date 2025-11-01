#pragma once
#include <vcclr.h>
using namespace System;

namespace PotaOCC
{
    public ref class LwPolylineDrawer
    {
    public:
        // Function to draw a batch of POLYLINE entities (multiple polylines)
        static array<int>^ DrawLwPolylineBatch(
            IntPtr ctxPtr,
            array<double>^ x,
            array<double>^ y,
            array<double>^ z,
            array<double>^ bulge,
            array<int>^ r,
            array<int>^ g,
            array<int>^ b,
            array<double>^ transparency,
            array<bool>^ closed);
    };
}