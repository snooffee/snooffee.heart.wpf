#include "pch.h"
#include "NativeViewerHandle.h"
#include "PointDrawer.h"

#include <AIS_InteractiveContext.hxx>
#include <AIS_Point.hxx>
#include <Geom_CartesianPoint.hxx> // Use Geom_CartesianPoint instead of Geom_Point
#include <V3d_Viewer.hxx>
#include <V3d_View.hxx>

using namespace PotaOCC;

void PointDrawer::DrawPoint(
    IntPtr ctxPtr,
    double x, double y, double z,
    int r, int g, int b, double transparency)
{
    if (ctxPtr == IntPtr::Zero) return;

    AIS_InteractiveContext* rawCtx = static_cast<AIS_InteractiveContext*>(ctxPtr.ToPointer());
    if (!rawCtx) return;

    Handle(AIS_InteractiveContext) ctx(rawCtx);

    gp_Pnt p(x, y, z);
    Handle(Geom_CartesianPoint) geomPoint = new Geom_CartesianPoint(p);
    Handle(AIS_Point) aisPoint = new AIS_Point(geomPoint);

    aisPoint->SetWidth(4.0); // optional: make point visible

    Quantity_Color qcol(r / 255.0, g / 255.0, b / 255.0, Quantity_TOC_RGB);
    ctx->SetColor(aisPoint, qcol, Standard_False);
    ctx->SetTransparency(aisPoint, transparency, Standard_False);
    ctx->Display(aisPoint, Standard_False);

    ctx->UpdateCurrentViewer();
}

array<int>^ PointDrawer::DrawPointBatch(
    IntPtr ctxPtr,
    array<double>^ x,
    array<double>^ y,
    array<double>^ z,
    array<int>^ r,
    array<int>^ g,
    array<int>^ b,
    array<double>^ transparency)
{
    if (ctxPtr == IntPtr::Zero) return gcnew array<int>(0);

    AIS_InteractiveContext* rawCtx = static_cast<AIS_InteractiveContext*>(ctxPtr.ToPointer());
    if (!rawCtx) return gcnew array<int>(0);

    Handle(AIS_InteractiveContext) ctx(rawCtx);

    int n = x->Length;
    auto ids = gcnew array<int>(n);

    for (int i = 0; i < n; ++i)
    {
        gp_Pnt p(x[i], y[i], z[i]);
        Handle(Geom_CartesianPoint) geomPoint = new Geom_CartesianPoint(p);
        Handle(AIS_Point) aisPoint = new AIS_Point(geomPoint);

        aisPoint->SetWidth(4.0);

        double dr = (r != nullptr && i < r->Length) ? r[i] / 255.0 : 0.5;
        double dg = (g != nullptr && i < g->Length) ? g[i] / 255.0 : 0.5;
        double db = (b != nullptr && i < b->Length) ? b[i] / 255.0 : 0.5;
        Quantity_Color qcol(dr, dg, db, Quantity_TOC_RGB);

        ctx->SetColor(aisPoint, qcol, Standard_False);

        if (transparency != nullptr && i < transparency->Length)
            ctx->SetTransparency(aisPoint, transparency[i], Standard_False);

        ctx->Display(aisPoint, Standard_False);

        ids[i] = (int)aisPoint.get();
    }

    ctx->UpdateCurrentViewer();
    return ids;
}

System::IntPtr PointDrawer::MakePoint(double x, double y, double z)
{
    gp_Pnt p(x, y, z);
    Handle(Geom_CartesianPoint) geomPoint = new Geom_CartesianPoint(p);
    Handle(AIS_Point) aisPoint = new AIS_Point(geomPoint);
    aisPoint->SetWidth(4.0);
    return System::IntPtr(aisPoint.get());
}