#pragma once
#include <vcclr.h>
using namespace System;

namespace PotaOCC
{
    public ref class Faces3DDrawer
    {
    public:
        /// <summary>
        /// Draws a batch of 3D faces (planar polygons with up to 4 vertices).
        /// </summary>
        static array<int>^ DrawFaces3DBatch(
            IntPtr ctxPtr,
            array<double>^ x1, array<double>^ y1, array<double>^ z1,
            array<double>^ x2, array<double>^ y2, array<double>^ z2,
            array<double>^ x3, array<double>^ y3, array<double>^ z3,
            array<double>^ x4, array<double>^ y4, array<double>^ z4,
            array<int>^ r, array<int>^ g, array<int>^ b,
            array<double>^ transparency);
    };
}