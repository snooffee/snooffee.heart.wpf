#pragma once
#include <vcclr.h>
using namespace System;

namespace PotatoOCC
{
    public ref class ArcDrawer
    {
    public:
        // Draw a single arc
        static void DrawArc(IntPtr ctxPtr,
            double cx, double cy, double cz,   // center
            double radius,
            double startAngle, double endAngle, // degrees
            int r, int g, int b, double transparency);

        // Draw multiple arcs in batch
        static array<int>^ DrawArcBatch(
            IntPtr ctxPtr,
            array<double>^ cx, array<double>^ cy, array<double>^ cz,
            array<double>^ radius,
            array<double>^ startAngle, array<double>^ endAngle,
            array<System::String^>^ lineTypes,
            array<int>^ r, array<int>^ g, array<int>^ b,
            array<double>^ transparency);

        // Create an arc shape handle without drawing
        static System::IntPtr MakeArc(double cx, double cy, double cz,
            double radius, double startAngle, double endAngle);
    };
}