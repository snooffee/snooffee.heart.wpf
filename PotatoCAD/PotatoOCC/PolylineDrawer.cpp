#include "pch.h"
#include "NativeViewerHandle.h"
#include "PolylineDrawer.h"
#include <AIS_InteractiveContext.hxx>
#include <V3d_Viewer.hxx>
#include <V3d_View.hxx>
#include <AIS_Shape.hxx>
#include <TopoDS_Edge.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <gp_Pnt.hxx>
#include <Quantity_Color.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <TopoDS_Compound.hxx>

using namespace PotatoOCC;

array<int>^ PolylineDrawer::DrawPolylineBatch(
    IntPtr ctxPtr,
    array<double>^ x, array<double>^ y, array<double>^ z,
    array<int>^ r, array<int>^ g, array<int>^ b,
    array<double>^ transparency,
    array<bool>^ closed)
{
    if (ctxPtr == IntPtr::Zero) return gcnew array<int>(0);
    AIS_InteractiveContext* rawCtx = static_cast<AIS_InteractiveContext*>(ctxPtr.ToPointer());
    if (!rawCtx) return gcnew array<int>(0);
    if (x->Length < 2) return gcnew array<int>(0); // at least 2 points

    Handle(AIS_InteractiveContext) ctx(rawCtx);

    try
    {
        BRepBuilderAPI_MakePolygon polylineBuilder;
        for (int i = 0; i < x->Length; i++)
        {
            gp_Pnt pt(x[i], y[i], z[i]);
            polylineBuilder.Add(pt);
        }

        if (closed[0])
            polylineBuilder.Close();

        polylineBuilder.Build();

        Handle(AIS_Shape) aisShape = new AIS_Shape(polylineBuilder.Shape());

        Quantity_Color col(r[0] / 255.0, g[0] / 255.0, b[0] / 255.0, Quantity_TOC_RGB);
        ctx->SetColor(aisShape, col, Standard_False);
        ctx->SetTransparency(aisShape, transparency[0], Standard_False);
        ctx->Display(aisShape, Standard_False);

        array<int>^ ids = gcnew array<int>(1);

        ids[0] = (int)aisShape.get();

        ctx->UpdateCurrentViewer();
        return ids;
    }
    catch (...)
    {
        return gcnew array<int>(0);
    }
}