#include "pch.h"
#include "NativeViewerHandle.h"
#include "SolidDrawer.h"
#include <AIS_InteractiveContext.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Wire.hxx>
#include <TopoDS_Edge.hxx>
#include <gp_Pnt.hxx>
#include <V3d_Viewer.hxx>
#include <V3d_View.hxx>

#include <BRepBuilderAPI_MakePolygon.hxx>
#include <AIS_Shape.hxx>

using namespace PotatoOCC;

void SolidDrawer::DrawSolid(IntPtr ctxPtr,
    double x1, double y1, double x2, double y2,
    double x3, double y3, double x4, double y4,
    int r, int g, int b, double transparency)
{
    if (ctxPtr == IntPtr::Zero) return;
    AIS_InteractiveContext* rawCtx = static_cast<AIS_InteractiveContext*>(ctxPtr.ToPointer());
    if (!rawCtx) return;

    Handle(AIS_InteractiveContext) ctx(rawCtx);

    // Create 4 corner points in XY plane (Z=0)
    gp_Pnt p1(x1, y1, 0);
    gp_Pnt p2(x2, y2, 0);
    gp_Pnt p3(x3, y3, 0);
    gp_Pnt p4(x4, y4, 0);

    // Build wire for face
    TopoDS_Edge e1 = BRepBuilderAPI_MakeEdge(p1, p2);
    TopoDS_Edge e2 = BRepBuilderAPI_MakeEdge(p2, p3);
    TopoDS_Edge e3 = BRepBuilderAPI_MakeEdge(p3, p4);
    TopoDS_Edge e4 = BRepBuilderAPI_MakeEdge(p4, p1);

    BRepBuilderAPI_MakeWire wireMaker;
    wireMaker.Add(e1); wireMaker.Add(e2); wireMaker.Add(e3); wireMaker.Add(e4);
    TopoDS_Wire wire = wireMaker.Wire();

    TopoDS_Face face = BRepBuilderAPI_MakeFace(wire);

    Handle(AIS_Shape) aisFace = new AIS_Shape(face);
    aisFace->SetDisplayMode(AIS_Shaded);

    ctx->Display(aisFace, Standard_False);
    ctx->SetColor(aisFace, Quantity_Color(r / 255.0, g / 255.0, b / 255.0, Quantity_TOC_RGB), Standard_False);
    ctx->SetTransparency(aisFace, transparency, Standard_False);

    ctx->UpdateCurrentViewer();
}

array<int>^ SolidDrawer::DrawSolidBatch(
    IntPtr ctxPtr,
    array<double>^ x1, array<double>^ y1, array<double>^ z1,
    array<double>^ x2, array<double>^ y2, array<double>^ z2,
    array<double>^ x3, array<double>^ y3, array<double>^ z3,
    array<double>^ x4, array<double>^ y4, array<double>^ z4,
    array<int>^ r, array<int>^ g, array<int>^ b,
    array<double>^ transparency)
{
    if (ctxPtr == IntPtr::Zero) return gcnew array<int>(0);
    AIS_InteractiveContext* rawCtx = static_cast<AIS_InteractiveContext*>(ctxPtr.ToPointer());
    if (!rawCtx) return gcnew array<int>(0);

    int n = x1->Length;
    auto ids = gcnew array<int>(n);

    Handle(AIS_InteractiveContext) ctx(rawCtx);

    for (int i = 0; i < n; i++)
    {
        gp_Pnt p1(x1[i], y1[i], z1[i]);
        gp_Pnt p2(x2[i], y2[i], z2[i]);
        gp_Pnt p3(x3[i], y3[i], z3[i]);
        gp_Pnt p4(x4[i], y4[i], z4[i]);

        try
        {
            BRepBuilderAPI_MakePolygon poly;
            poly.Add(p1);
            poly.Add(p2);
            poly.Add(p4);
            poly.Add(p3);
            poly.Close();

            TopoDS_Face face = BRepBuilderAPI_MakeFace(poly.Wire());
            Handle(AIS_Shape) aisShape = new AIS_Shape(face);

            Quantity_Color col(r[i] / 255.0, g[i] / 255.0, b[i] / 255.0, Quantity_TOC_RGB);
            ctx->SetColor(aisShape, col, Standard_False);
            if (transparency != nullptr && i < transparency->Length)
                ctx->SetTransparency(aisShape, transparency[i], Standard_False);

            ctx->Display(aisShape, Standard_False);
            ids[i] = (int)aisShape.get();
        }
        catch (...)
        {
            ids[i] = 0; // mark failed face
        }
    }


    ctx->UpdateCurrentViewer();
    return ids;
}