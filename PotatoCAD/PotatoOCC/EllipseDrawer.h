#pragma once
#include <vcclr.h>
using namespace System;

namespace PotatoOCC
{
    public ref class EllipseDrawer
    {
    public:
        static array<int>^ DrawEllipseBatch(
            System::IntPtr ctxPtr,
            array<double>^ x, array<double>^ y, array<double>^ z,
            array<double>^ semiMajor, array<double>^ semiMinor, array<double>^ rotationAngle,
            array<System::String^>^ lineTypes,
            array<int>^ r, array<int>^ g, array<int>^ b,
            array<double>^ transparency);
    };
}