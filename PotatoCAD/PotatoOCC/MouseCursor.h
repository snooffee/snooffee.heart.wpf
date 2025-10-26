#pragma once
#include <vcclr.h>
#using <System.Windows.Forms.dll>
using namespace System::Windows::Forms;

namespace PotatoOCC
{
    enum class CursorType
    {
        Default,        // Standard arrow
        Crosshair,      // Crosshair (for precise selection)
        Hand,           // Hand (commonly used for clickable items)
        Wait,           // Hourglass or spinning wheel
        IBeam,          // Text selection (used for input fields)
        SizeAll,        // Move icon (4 arrows)
        SizeNESW,       // Diagonal resize ??
        SizeNWSE,       // Diagonal resize ??
        SizeNS,         // Vertical resize ?
        SizeWE,         // Horizontal resize ?
        No,             // Circle with a slash (not allowed)
        UpArrow,        // Arrow pointing up (used in selections)
        Help            // Question mark (used for help prompts)
    };

    class MouseCursor
    {
    public:
        static void SetCustomCursor(NativeViewerHandle* native, CursorType type);
        static void DrawRubberBandRectangle(NativeViewerHandle* native, int xMin, int yMin, int xMax, int yMax);
    };
}