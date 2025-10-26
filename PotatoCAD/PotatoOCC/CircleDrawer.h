#pragma once
#include <vcclr.h>
#include <TopoDS_Edge.hxx>

using namespace System;

namespace PotatoOCC
{
    public ref class CircleDrawer
    {
    public:
        static TopoDS_Edge CircleDrawer::DrawCircle(Handle(V3d_View) view, IntPtr viewerHandlePtr, double dragStartX, double dragStartY, double dragEndX, double dragEndY, int h, int w);

        static array<int>^ DrawCircleBatch(
            IntPtr ctxPtr,
            array<double>^ x, array<double>^ y, array<double>^ z,
            array<double>^ radius,
            array<System::String^>^ lineTypes,
            array<int>^ r, array<int>^ g, array<int>^ b,
            array<double>^ transparency);

        static System::IntPtr MakeCircle(double cx, double cy, double cz, double radius);
    };
}