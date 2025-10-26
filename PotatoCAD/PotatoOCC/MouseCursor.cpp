#include "pch.h"
#include "NativeViewerHandle.h"
#include "MouseCursor.h"
#include <Graphic3d_ArrayOfSegments.hxx>
#include <Graphic3d_Structure.hxx>
#include <Graphic3d_StructureManager.hxx>
#include <Graphic3d_Group.hxx>

#include <V3d_Viewer.hxx>
#include <V3d_View.hxx>

#using <System.Windows.Forms.dll>
using namespace System::Windows::Forms;

using namespace PotatoOCC;

void MouseCursor::SetCustomCursor(NativeViewerHandle* native, CursorType type)
{
    if (!native) return;

    Control^ winFormsControl = native->hostControl;

    if (winFormsControl == nullptr || !winFormsControl->Visible)
    {
        System::Diagnostics::Debug::WriteLine("hostControl is null or not visible");
        return;
    }

    switch (type)
    {
    case CursorType::Crosshair:
        winFormsControl->Cursor = Cursors::Cross;
        break;
    case CursorType::Wait:
        winFormsControl->Cursor = Cursors::WaitCursor;
        break;
    case CursorType::Default:
        winFormsControl->Cursor = Cursors::Default;
        break;
    }
}

void MouseCursor::DrawRubberBandRectangle(NativeViewerHandle* native, int xMin, int yMin, int xMax, int yMax)
{
    if (!native || native->view.IsNull()) return;

    Handle(V3d_View) view = native->view;

    // Convert screen points to view coordinates
    Standard_Real x1, y1, z1;
    view->Convert(xMin, yMin, x1, y1, z1);

    Standard_Real x2, y2, z2;
    view->Convert(xMax, yMin, x2, y2, z2);

    Standard_Real x3, y3, z3;
    view->Convert(xMax, yMax, x3, y3, z3);

    Standard_Real x4, y4, z4;
    view->Convert(xMin, yMax, x4, y4, z4);

    gp_Pnt p1(x1, y1, z1);
    gp_Pnt p2(x2, y2, z2);
    gp_Pnt p3(x3, y3, z3);
    gp_Pnt p4(x4, y4, z4);

    // Clear previous overlay
    if (!native->rubberBandOverlay.IsNull()) {
        native->rubberBandOverlay->Erase();
        native->rubberBandOverlay.Nullify();
    }

    // Create new structure on top overlay
    Handle(Graphic3d_StructureManager) structMgr = view->Viewer()->StructureManager();
    native->rubberBandOverlay = new Graphic3d_Structure(structMgr);
    native->rubberBandOverlay->SetZLayer(Graphic3d_ZLayerId_Top);
    native->rubberBandOverlay->SetDisplayPriority(10); // optional

    native->rubberBandGroup = native->rubberBandOverlay->NewGroup();

    // Build rectangle using segments
    Handle(Graphic3d_ArrayOfSegments) segments = new Graphic3d_ArrayOfSegments(8);

    segments->AddVertex(p1);
    segments->AddVertex(p2);

    segments->AddVertex(p2);
    segments->AddVertex(p3);

    segments->AddVertex(p3);
    segments->AddVertex(p4);

    segments->AddVertex(p4);
    segments->AddVertex(p1); // Close the loop

    Handle(Prs3d_LineAspect) lineAspect = new Prs3d_LineAspect(
        Quantity_Color(1.0, 0.0, 0.0, Quantity_TOC_RGB), Aspect_TOL_SOLID, 1.0);

    native->rubberBandGroup->SetPrimitivesAspect(lineAspect->Aspect());
    native->rubberBandGroup->AddPrimitiveArray(segments);

    native->rubberBandOverlay->Display();
}