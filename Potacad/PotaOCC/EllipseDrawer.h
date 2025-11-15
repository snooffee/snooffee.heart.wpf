#pragma once
#include <vcclr.h>
#include <TopoDS_Wire.hxx>
#include <gp_Elips.hxx>
#include <gp_Pnt.hxx>
using namespace System;

namespace PotaOCC
{
    public ref class EllipseDrawer
    {
    public:
        static array<int>^ DrawEllipseBatch(System::IntPtr ctxPtr, array<double>^ x, array<double>^ y, array<double>^ z, array<double>^ semiMajor, array<double>^ semiMinor, array<double>^ rotationAngle, array<System::String^>^ lineTypes, array<int>^ r, array<int>^ g, array<int>^ b, array<double>^ transparency);
        static TopoDS_Wire DrawEllipse(NativeViewerHandle* native, Handle(V3d_View) view, IntPtr viewerHandlePtr, double dragStartX, double dragStartY, double dragEndX, double dragEndY, int h, int w, int x, int y);
    };
}