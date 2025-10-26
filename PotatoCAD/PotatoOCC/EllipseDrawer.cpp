#include "pch.h"
#include "NativeViewerHandle.h"

#include "EllipseDrawer.h"
#include <gp_Ax2.hxx>              // For creating a plane in space (gp_Ax2)
#include <gp_Pnt.hxx>              // For creating points (gp_Pnt)
#include <gp_Trsf.hxx>             // For transformations (gp_Trsf)
#include <Geom_Ellipse.hxx>        // For the ellipse geometry (Geom_Ellipse)
#include <BRepBuilderAPI_MakeEdge.hxx>  // To create edges from geometric objects (BRepBuilderAPI_MakeEdge)
#include <AIS_Shape.hxx>           // For visual representation (AIS_Shape)
#include <Prs3d_Drawer.hxx>        // For drawing attributes (Prs3d_Drawer)
#include <Prs3d_LineAspect.hxx>    // For line attributes (Prs3d_LineAspect)
#include <Aspect_TypeOfLine.hxx>   // For line styles (e.g., solid, dashed)
#include <Quantity_Color.hxx>      // For color handling (Quantity_Color)
#include <AIS_InteractiveContext.hxx>  // For interactive context (AIS_InteractiveContext)
#include <Standard_Integer.hxx>    // For integer types (Standard_Integer)

#include <GC_MakeSegment.hxx>
#include <Geom_TrimmedCurve.hxx> // for trimmed curve
#include <V3d_Viewer.hxx>
#include <V3d_View.hxx>
#include <TopoDS_Shape.hxx>

using namespace PotatoOCC;

array<int>^ EllipseDrawer::DrawEllipseBatch(
    System::IntPtr ctxPtr,
    array<double>^ x, array<double>^ y, array<double>^ z,
    array<double>^ semiMajor, array<double>^ semiMinor, array<double>^ rotationAngle,
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
        // Create ellipse geometry
        gp_Ax2 ax2(gp_Pnt(x[i], y[i], z[i]), gp::DZ());  // Plane (position and orientation)
        Handle(Geom_Ellipse) geomEllipse = new Geom_Ellipse(ax2, semiMajor[i], semiMinor[i]);

        // Apply rotation (if necessary)
        if (rotationAngle != nullptr && i < rotationAngle->Length)
        {
            gp_Pnt center(x[i], y[i], z[i]);
            gp_Ax1 rotationAxis(center, gp_Vec(0, 0, 1));  // Rotation axis passing through ellipse center

            gp_Trsf rotation;
            rotation.SetRotation(rotationAxis, rotationAngle[i]);

            geomEllipse->Transform(rotation);
        }

        // Create an edge for the ellipse
        Handle(AIS_Shape) aisEllipse = new AIS_Shape(BRepBuilderAPI_MakeEdge(geomEllipse));

        // Set display mode (Wireframe or Shaded)
        aisEllipse->SetDisplayMode(AIS_WireFrame);

        // Map line type
        Aspect_TypeOfLine occType = Aspect_TOL_SOLID;  // Default
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

        Handle(Prs3d_Drawer) drawer = aisEllipse->Attributes();
        if (drawer.IsNull()) drawer = new Prs3d_Drawer();

        // Apply color and line type to the ellipse
        Handle(Prs3d_LineAspect) lineAsp = new Prs3d_LineAspect(qcol, occType, 1.0);
        drawer->SetWireAspect(lineAsp);
        aisEllipse->SetAttributes(drawer);

        // Display the ellipse
        ctx->Display(aisEllipse, false);
        ctx->SetColor(aisEllipse, qcol, false);

        // Store ID
        ids[i] = (int)aisEllipse.get();
    }

    // Update viewer to reflect changes
    ctx->UpdateCurrentViewer();

    return ids;
}