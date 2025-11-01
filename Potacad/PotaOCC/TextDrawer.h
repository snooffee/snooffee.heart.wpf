#pragma once
#include <vcclr.h>
#include <AIS_Shape.hxx>
using namespace System;

namespace PotaOCC
{
    public ref class TextDrawer
    {
    public:
        static void DrawTextBatch(
            IntPtr viewerHandlePtr,
            array<double>^ x,
            array<double>^ y,
            array<double>^ z,
            array<String^>^ texts,
            array<double>^ modelHeights,
            array<double>^ rotations,
            array<double>^ widthFactors,
            array<int>^ r,
            array<int>^ g,
            array<int>^ b,
            array<double>^ transparency,
            double sceneScale);

        static Handle(AIS_Shape) DrawSingleText(
            System::IntPtr viewerHandlePtr,
            double x, double y, double z,
            System::String^ text,
            double height,
            double rotationDeg,
            double widthFactor,
            int r, int g, int b);

    private:
        // C++/CLI helper to convert full-width Latin letters to ASCII
        static System::String^ NormalizeText(System::String^ input);
    };
}