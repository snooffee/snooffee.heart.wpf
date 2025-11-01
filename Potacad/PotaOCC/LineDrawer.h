#pragma once
#include <vcclr.h>
using namespace System;
#include <AIS_Shape.hxx>


namespace PotaOCC
{
    public ref class LineDrawer
    {
    public:
        static Handle(AIS_Shape) DrawLineWithoutSnapping(Handle(V3d_View) view, IntPtr viewerHandlePtr, double dragStartX, double dragStartY, double dragEndX, double dragEndY, int h, int w);
        static Handle(AIS_Shape) DrawLine(Handle(V3d_View) view, IntPtr viewerHandlePtr, double dragStartX, double dragStartY, double dragEndX, double dragEndY, int h, int w);

        static array<int>^ DrawLineBatch(
            IntPtr ctxPtr,
            array<double>^ x1, array<double>^ y1, array<double>^ z1,
            array<double>^ x2, array<double>^ y2, array<double>^ z2,
            array<System::String^>^ lineTypes,
            array<int>^ r, array<int>^ g, array<int>^ b,
            array<double>^ transparency);

        static System::IntPtr MakeLine(double x1, double y1, double z1, double x2, double y2, double z2);
    };
}