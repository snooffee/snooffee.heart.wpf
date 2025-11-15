#include "pch.h"
#include "NativeViewerHandle.h"
#include "EllipseDrawer.h"
#include "ShapeDrawer.h"
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
#include "MouseHelper.h"
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
using namespace PotaOCC::MouseHelper;

using namespace PotaOCC;

array<int>^ EllipseDrawer::DrawEllipseBatch(System::IntPtr ctxPtr, array<double>^ x, array<double>^ y, array<double>^ z, array<double>^ semiMajor, array<double>^ semiMinor, array<double>^ rotationAngle, array<System::String^>^ lineTypes, array<int>^ r, array<int>^ g, array<int>^ b, array<double>^ transparency)
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

TopoDS_Wire EllipseDrawer::DrawEllipse(NativeViewerHandle* native, Handle(V3d_View) view, IntPtr viewerHandlePtr, double dragStartX, double dragStartY, double dragEndX, double dragEndY, int h, int w, int x, int y)
{
    // Overlay for preview
    Handle(AIS_OverlayEllipse) persistentEllipse = new AIS_OverlayEllipse();
    ShapeDrawer::SetupOverlay(persistentEllipse, Quantity_NOC_RED, 2.0);

    gp_Pnt center;
    double radiusX = 0.0, radiusY = 0.0;
    gp_Dir normalDir = gp::DZ(); // default normal
    gp_Dir majorDir = gp::DX();  // default X direction
    gp_Dir uDir = gp::DX();
    if (native->isPlaneMode)
    {
        // --- Same coordinate base as RectangleDrawer ---
        gp_Pnt startPnt = native->dragStartPoint;
        gp_Pnt endPnt = Get3DPntOnPlane(view,
            native->planeOrigin,
            native->planeNormal,
            x, y);

        gp_Dir planeNormal = native->planeNormal;
        gp_Ax2 planeAxis(native->planeOrigin, planeNormal);

        // Use local plane coordinate system
        gp_Dir xDir = planeAxis.XDirection();
        gp_Dir yDir = planeAxis.YDirection();

        // Compute drag vector and project to plane axes
        gp_Vec dragVec(startPnt, endPnt);
        double u = dragVec.Dot(gp_Vec(xDir));
        double v = dragVec.Dot(gp_Vec(yDir));

        radiusX = std::abs(u) * 0.5;
        radiusY = std::abs(v) * 0.5;

        // Compute ellipse center same way as rectangle center
        center = startPnt.Translated(gp_Vec(xDir) * (u * 0.5) + gp_Vec(yDir) * (v * 0.5));

        normalDir = planeNormal;
        uDir = xDir;
    }
    else
    {
        // 6️⃣ Screen-space ellipse when not in plane mode
        gp_Pnt pStart = ShapeDrawer::ScreenToWorld(view, dragStartX, dragStartY);
        gp_Pnt pEnd = ShapeDrawer::ScreenToWorld(view, dragEndX, dragEndY);

        center = gp_Pnt(
            (pStart.X() + pEnd.X()) * 0.5,
            (pStart.Y() + pEnd.Y()) * 0.5,
            (pStart.Z() + pEnd.Z()) * 0.5
        );

        radiusX = std::abs(pEnd.X() - pStart.X()) * 0.5;
        radiusY = std::abs(pEnd.Y() - pStart.Y()) * 0.5;

        // 7️⃣ Camera normal
        Standard_Real eyeX, eyeY, eyeZ, atX, atY, atZ;
        view->Eye(eyeX, eyeY, eyeZ);
        view->At(atX, atY, atZ);

        gp_Vec viewVec(gp_Pnt(eyeX, eyeY, eyeZ), gp_Pnt(atX, atY, atZ));
        if (viewVec.Magnitude() < 1e-6)
            viewVec = gp_Vec(0, 0, 1);

        normalDir = gp_Dir(viewVec);
        majorDir = gp::DX();
    }

    // 8️⃣ Ensure positive radii
    if (radiusX <= 1e-6) radiusX = 1e-6;
    if (radiusY <= 1e-6) radiusY = 1e-6;

    // 9️⃣ Build smooth polygonal ellipse wire
    const int numSegments = 72;
    BRepBuilderAPI_MakeWire wireBuilder;

    /*for (int i = 0; i < numSegments; ++i)
    {
        double angle1 = 2.0 * M_PI * i / numSegments;
        double angle2 = 2.0 * M_PI * (i + 1) / numSegments;

        gp_Pnt p1(center.X() + radiusX * std::cos(angle1),
            center.Y() + radiusY * std::sin(angle1),
            center.Z());
        gp_Pnt p2(center.X() + radiusX * std::cos(angle2),
            center.Y() + radiusY * std::sin(angle2),
            center.Z());

        TopoDS_Edge edge = BRepBuilderAPI_MakeEdge(p1, p2);
        wireBuilder.Add(edge);
    }*/
    for (int i = 0; i < numSegments; ++i)
    {
        double angle1 = 2.0 * M_PI * i / numSegments;
        double angle2 = 2.0 * M_PI * (i + 1) / numSegments;

        gp_Pnt p1, p2;

        if (native->isPlaneMode)
        {
            // 🔹 Build ellipse in plane-space (same as overlay)
            gp_Vec v1 = gp_Vec(uDir) * (radiusX * std::cos(angle1)) +
                gp_Vec(normalDir.Crossed(uDir)) * (radiusY * std::sin(angle1));
            gp_Vec v2 = gp_Vec(uDir) * (radiusX * std::cos(angle2)) +
                gp_Vec(normalDir.Crossed(uDir)) * (radiusY * std::sin(angle2));

            p1 = center.Translated(v1);
            p2 = center.Translated(v2);
        }
        else
        {
            // 🔹 Standard XY-plane ellipse
            p1 = gp_Pnt(center.X() + radiusX * std::cos(angle1),
                center.Y() + radiusY * std::sin(angle1),
                center.Z());
            p2 = gp_Pnt(center.X() + radiusX * std::cos(angle2),
                center.Y() + radiusY * std::sin(angle2),
                center.Z());
        }

        TopoDS_Edge edge = BRepBuilderAPI_MakeEdge(p1, p2);
        wireBuilder.Add(edge);
    }


    TopoDS_Wire wire = wireBuilder.Wire();
    return wire;
}