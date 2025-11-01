#pragma once
#include <vcclr.h>
using namespace System;

namespace PotaOCC
{
    public ref class PointDrawer
    {
    public:
        // Draw a single point
        static void DrawPoint(
            IntPtr ctxPtr,
            double x, double y, double z,
            int r, int g, int b, double transparency);

        // Draw multiple points in batch
        static array<int>^ DrawPointBatch(
            IntPtr ctxPtr,
            array<double>^ x,
            array<double>^ y,
            array<double>^ z,
            array<int>^ r,
            array<int>^ g,
            array<int>^ b,
            array<double>^ transparency);

        // Create a point and return pointer (IntPtr)
        static System::IntPtr MakePoint(double x, double y, double z);
    };
}