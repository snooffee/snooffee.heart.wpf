#include "pch.h"
#include "RectangleDrawer.h"
#include "ShapeDrawer.h"
#include "NativeViewerHandle.h"

using namespace PotaOCC;

Handle(AIS_Shape) RectangleDrawer::DrawRectangle(
    Handle(AIS_InteractiveContext) context,
    NativeViewerHandle* native,
    Handle(V3d_View) view,
    const Quantity_Color& color,
    double transparency)
{
    // 🚫 Safety check: null pointers
    if (context.IsNull() || view.IsNull() || native == nullptr)
    {
        std::cerr << "⚠️ Invalid input: context, view, or native is null." << std::endl;
        return nullptr;
    }

    // 🚫 Ensure drag points are valid
    if (native->dragStartX == native->dragEndX &&
        native->dragStartY == native->dragEndY)
    {
        std::cerr << "⚠️ Drag start and end points are identical — rectangle skipped." << std::endl;
        return nullptr;
    }

    // ✅ Convert screen to world coordinates safely
    gp_Pnt p1, p2;
    try
    {
        p1 = ShapeDrawer::ScreenToWorld(view, native->dragStartX, native->dragStartY);
        p2 = ShapeDrawer::ScreenToWorld(view, native->dragEndX, native->dragEndY);
    }
    catch (Standard_Failure& e)
    {
        std::cerr << "❌ ScreenToWorld failed: " << e.GetMessageString() << std::endl;
        return nullptr;
    }

    // 🚫 Prevent zero-size rectangle
    if (Abs(p1.X() - p2.X()) < Precision::Confusion() ||
        Abs(p1.Y() - p2.Y()) < Precision::Confusion())
    {
        std::cerr << "⚠️ Rectangle has zero width or height — skipped." << std::endl;
        return nullptr;
    }

    // ✅ Create points in order
    gp_Pnt p3(p2.X(), p1.Y(), 0.0);
    gp_Pnt p4(p1.X(), p2.Y(), 0.0);

    TopoDS_Wire wire;
    try
    {
        TopoDS_Edge e1 = BRepBuilderAPI_MakeEdge(p1, p3);
        TopoDS_Edge e2 = BRepBuilderAPI_MakeEdge(p3, p2);
        TopoDS_Edge e3 = BRepBuilderAPI_MakeEdge(p2, p4);
        TopoDS_Edge e4 = BRepBuilderAPI_MakeEdge(p4, p1);
        wire = BRepBuilderAPI_MakeWire(e1, e2, e3, e4);
    }
    catch (Standard_Failure& e)
    {
        std::cerr << "❌ Failed to make wire: " << e.GetMessageString() << std::endl;
        return nullptr;
    }

    if (wire.IsNull())
    {
        std::cerr << "⚠️ Wire is null — rectangle not created." << std::endl;
        return nullptr;
    }

    // ✅ Create face
    TopoDS_Face face;
    try
    {
        face = BRepBuilderAPI_MakeFace(wire);
    }
    catch (Standard_Failure& e)
    {
        std::cerr << "❌ Failed to make face: " << e.GetMessageString() << std::endl;
        return nullptr;
    }

    if (face.IsNull())
    {
        std::cerr << "⚠️ Face is null — cannot create AIS_Shape." << std::endl;
        return nullptr;
    }

    // ✅ Create AIS shape for visualization
    Handle(AIS_Shape) aisRect = new AIS_Shape(face);
    aisRect->SetColor(color);
    aisRect->SetTransparency(transparency);

    // ✅ Remove previous overlay if exists
    if (!native->rectangleOverlay.IsNull())
    {
        try
        {
            context->Remove(native->rectangleOverlay, Standard_False);
        }
        catch (Standard_Failure& e)
        {
            std::cerr << "⚠️ Failed to remove previous overlay: " << e.GetMessageString() << std::endl;
        }
        native->rectangleOverlay.Nullify();
    }

    std::cout << "✅ Rectangle created successfully." << std::endl;
    return aisRect;
}