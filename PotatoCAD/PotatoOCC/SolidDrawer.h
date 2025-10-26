#pragma once
#include <vcclr.h>
using namespace System;

namespace PotatoOCC
{
    public ref class SolidDrawer
    {
    public:
        // Draw a single 2D quadrilateral solid in XY plane
        static void DrawSolid(IntPtr ctxPtr,
            double x1, double y1, double x2, double y2,
            double x3, double y3, double x4, double y4,
            int r, int g, int b, double transparency);

        // Draw multiple SOLID entities in batch
        static array<int>^ DrawSolidBatch(
            IntPtr ctxPtr,
            array<double>^ x1, array<double>^ y1, array<double>^ z1,
            array<double>^ x2, array<double>^ y2, array<double>^ z2,
            array<double>^ x3, array<double>^ y3, array<double>^ z3,
            array<double>^ x4, array<double>^ y4, array<double>^ z4,
            array<int>^ r, array<int>^ g, array<int>^ b,
            array<double>^ transparency);
    };
}