#include "pch.h"
#include "NativeViewerHandle.h"
#include "CircleDrawer.h"
#include "ShapeDrawer.h"
#include <AIS_InteractiveContext.hxx>
#include <GC_MakeSegment.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <Geom_TrimmedCurve.hxx>
#include <Geom_Circle.hxx>
#include <V3d_Viewer.hxx>
#include <V3d_View.hxx>
#include <AIS_Shape.hxx>
#include <Prs3d_Drawer.hxx>
#include <Prs3d_LineAspect.hxx>
#include <Quantity_Color.hxx>
#include <Aspect_TypeOfLine.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Edge.hxx>

using namespace PotaOCC;

TopoDS_Edge CircleDrawer::DrawCircle(Handle(V3d_View) view, IntPtr viewerHandlePtr, double dragStartX, double dragStartY, double dragEndX, double dragEndY, int h, int w)
{
    Handle(AIS_OverlayCircle) persistentCircle = new AIS_OverlayCircle();
    ShapeDrawer::SetupOverlay(persistentCircle, Quantity_NOC_RED, 2.0);

    gp_Pnt center = ShapeDrawer::ScreenToWorld(view, dragStartX, dragStartY);
    gp_Pnt edgePoint = ShapeDrawer::ScreenToWorld(view, dragEndX, dragEndY);

    double radius = center.Distance(edgePoint);

    gp_Circ circle(gp_Ax2(center, gp_Dir(0, 0, 1)), radius);

    return ShapeDrawer::CreateCircleSafe(circle);
}


array<int>^ CircleDrawer::DrawCircleBatch(
    System::IntPtr ctxPtr,
    array<double>^ x, array<double>^ y, array<double>^ z,
    array<double>^ radius,
    array<System::String^>^ lineTypes,
    array<int>^ r, array<int>^ g, array<int>^ b,
    array<double>^ transparency)
{
    if (ctxPtr == System::IntPtr::Zero)
        return gcnew array<int>(0);

    AIS_InteractiveContext* rawCtx = static_cast<AIS_InteractiveContext*>(ctxPtr.ToPointer());
    if (!rawCtx)
        return gcnew array<int>(0);

    Handle(AIS_InteractiveContext) ctx(rawCtx);
    int n = x->Length;
    auto ids = gcnew array<int>(n);

    for (int i = 0; i < n; ++i)
    {
        // Create circle geometry
        gp_Ax2 ax2(gp_Pnt(x[i], y[i], z[i]), gp::DZ());
        Handle(Geom_Circle) geomCircle = new Geom_Circle(ax2, radius[i]);
        Handle(AIS_Shape) aisCircle = new AIS_Shape(BRepBuilderAPI_MakeEdge(geomCircle));

        // Set display mode (Wireframe or Shaded)
        aisCircle->SetDisplayMode(AIS_WireFrame);

        // Map line type
        Aspect_TypeOfLine occType = Aspect_TOL_SOLID; // default
        if (lineTypes != nullptr && i < lineTypes->Length && lineTypes[i] != nullptr)
        {
            System::String^ s = lineTypes[i]->Trim()->ToUpperInvariant();
            if (s->IndexOf("DASH") >= 0) occType = Aspect_TOL_DASH;
            else if (s->IndexOf("DASHED1") >= 0) occType = Aspect_TOL_DASH;
            else if (s->IndexOf("DASHED2") >= 0) occType = Aspect_TOL_DASH;
            else if (s->IndexOf("DOT") >= 0) occType = Aspect_TOL_DOT;
            else if (s->IndexOf("CENTER") >= 0) occType = Aspect_TOL_DOTDASH;
        }

        // Set color (RGB)
        double dr = (r != nullptr && i < r->Length) ? (double)r[i] / 255.0 : 0.5;
        double dg = (g != nullptr && i < g->Length) ? (double)g[i] / 255.0 : 0.5;
        double db = (b != nullptr && i < b->Length) ? (double)b[i] / 255.0 : 0.5;
        Quantity_Color qcol(dr, dg, db, Quantity_TOC_RGB);

        Handle(Prs3d_Drawer) drawer = aisCircle->Attributes();
        if (drawer.IsNull()) drawer = new Prs3d_Drawer();

        // Apply color and line type to the circle
        Handle(Prs3d_LineAspect) lineAsp = new Prs3d_LineAspect(qcol, occType, 1.0);
        drawer->SetWireAspect(lineAsp);
        aisCircle->SetAttributes(drawer);

        // Display the circle
        ctx->Display(aisCircle, Standard_False);
        ctx->SetColor(aisCircle, qcol, Standard_False);

        // Store ID
        ids[i] = (int)aisCircle.get();
    }

    // Update viewer to reflect changes
    ctx->UpdateCurrentViewer();

    return ids;
}



System::IntPtr CircleDrawer::MakeCircle(double cx, double cy, double cz, double radius)
{
    gp_Ax2 ax2(gp_Pnt(cx, cy, cz), gp::DZ());
    Handle(Geom_Circle) circle = new Geom_Circle(ax2, radius);
    Handle(AIS_Shape) shape = new AIS_Shape(BRepBuilderAPI_MakeEdge(circle));
    return System::IntPtr(shape.get());
}