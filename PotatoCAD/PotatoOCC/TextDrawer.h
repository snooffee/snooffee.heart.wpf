#pragma once
#include <vcclr.h>
using namespace System;

namespace PotatoOCC
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

    private:
        // C++/CLI helper to convert full-width Latin letters to ASCII
        static System::String^ NormalizeText(System::String^ input);
    };
}