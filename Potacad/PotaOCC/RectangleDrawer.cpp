#include "pch.h"
#include "RectangleDrawer.h"
#include "ShapeDrawer.h"
#include "NativeViewerHandle.h"
#include "MouseHelper.h"
using namespace PotaOCC::MouseHelper;
using namespace PotaOCC;
Handle(AIS_Shape) RectangleDrawer::DrawRectangle(
    Handle(AIS_InteractiveContext) context,
    NativeViewerHandle* native,
    Handle(V3d_View) view)
{
    // Safety check
    if (context.IsNull() || view.IsNull() || native == nullptr)
    {
        std::cerr << "⚠️ Invalid input: context, view, or native is null." << std::endl;
        return nullptr;
    }

    // Ensure drag points are valid
    if (native->dragStartX == native->dragEndX &&
        native->dragStartY == native->dragEndY)
    {
        std::cerr << "⚠️ Drag start and end points are identical — rectangle skipped." << std::endl;
        return nullptr;
    }

    gp_Pnt p1, p2, p3, p4;

    if (native->isPlaneMode)
    {
        // --- Compute rectangle points on the detected plane ---
        gp_Pnt startPnt = native->dragStartPoint; // 3D point on plane at mouse down
        gp_Pnt endPnt = Get3DPntOnPlane(view,
            native->planeOrigin,
            native->planeNormal,
            native->dragEndX,
            native->dragEndY);

        // Use plane coordinate system: we need two axes on the plane
        gp_Dir planeNormal = native->planeNormal;
        gp_Ax2 planeAxis(native->planeOrigin, planeNormal);

        // Compute two orthogonal directions on plane
        gp_Dir uDir = planeAxis.XDirection();
        gp_Dir vDir = planeAxis.YDirection();

        // Project drag vector onto plane axes
        gp_Vec dragVec(startPnt, endPnt);
        double u = dragVec.Dot(gp_Vec(uDir));
        double v = dragVec.Dot(gp_Vec(vDir));

        // Compute rectangle 4 corners in 3D
        p1 = startPnt;
        p2 = startPnt.Translated(gp_Vec(uDir) * u);
        p3 = startPnt.Translated(gp_Vec(uDir) * u + gp_Vec(vDir) * v);
        p4 = startPnt.Translated(gp_Vec(vDir) * v);
    }
    else
    {
        // Screen-plane fallback (XY)
        p1 = ShapeDrawer::ScreenToWorld(view, native->dragStartX, native->dragStartY);
        p3 = ShapeDrawer::ScreenToWorld(view, native->dragEndX, native->dragEndY);
        p2 = gp_Pnt(p3.X(), p1.Y(), 0.0);
        p4 = gp_Pnt(p1.X(), p3.Y(), 0.0);
    }

    // Build wire
    TopoDS_Wire wire;
    try
    {
        TopoDS_Edge e1 = BRepBuilderAPI_MakeEdge(p1, p2);
        TopoDS_Edge e2 = BRepBuilderAPI_MakeEdge(p2, p3);
        TopoDS_Edge e3 = BRepBuilderAPI_MakeEdge(p3, p4);
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

    // Build face
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

    // Create AIS_Shape
    Handle(AIS_Shape) aisRect = new AIS_Shape(face);
    aisRect->SetColor(Quantity_NOC_RED);
    aisRect->SetTransparency(0.2);

    // Remove previous overlay
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