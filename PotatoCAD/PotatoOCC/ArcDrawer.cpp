#include "pch.h"
#include "NativeViewerHandle.h"
#include "ArcDrawer.h"
#include <AIS_InteractiveContext.hxx>
#include <GC_MakeArcOfCircle.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <Geom_TrimmedCurve.hxx>
#include <V3d_Viewer.hxx>
#include <V3d_View.hxx>
#include <gp_Circ.hxx>
#include <gp_Ax2.hxx>

using namespace PotatoOCC;

void ArcDrawer::DrawArc(IntPtr ctxPtr,
    double cx, double cy, double cz,
    double radius,
    double startAngle, double endAngle,
    int r, int g, int b, double transparency)
{
    AIS_InteractiveContext* rawCtx = static_cast<AIS_InteractiveContext*>(ctxPtr.ToPointer());
    if (!rawCtx) return;

    Handle(AIS_InteractiveContext) ctx(rawCtx);

    gp_Pnt center(cx, cy, cz);
    gp_Ax2 ax(center, gp::DZ()); // Z-up plane
    gp_Circ circle(ax, radius);

    Handle(Geom_TrimmedCurve) arc = GC_MakeArcOfCircle(circle,
        startAngle * M_PI / 180.0,
        endAngle * M_PI / 180.0,
        true);

    Handle(AIS_Shape) aisArc = new AIS_Shape(BRepBuilderAPI_MakeEdge(arc));

    ctx->Display(aisArc, Standard_False);
    ctx->SetColor(aisArc, Quantity_Color(r / 255.0, g / 255.0, b / 255.0, Quantity_TOC_RGB), Standard_False);
    ctx->SetTransparency(aisArc, transparency, Standard_False);
}

array<int>^ ArcDrawer::DrawArcBatch(
    IntPtr ctxPtr,
    array<double>^ cx, array<double>^ cy, array<double>^ cz,
    array<double>^ radius,
    array<double>^ startAngle, array<double>^ endAngle,
    array<System::String^>^ lineTypes,
    array<int>^ r, array<int>^ g, array<int>^ b,
    array<double>^ transparency)
{
    if (ctxPtr == System::IntPtr::Zero) return gcnew array<int>(0);
    AIS_InteractiveContext* rawCtx = static_cast<AIS_InteractiveContext*>(ctxPtr.ToPointer());
    if (!rawCtx) return gcnew array<int>(0);

    Handle(AIS_InteractiveContext) ctx(rawCtx);

    int n = cx->Length;
    auto ids = gcnew array<int>(n);

    for (int i = 0; i < n; ++i)
    {
        gp_Pnt center(cx[i], cy[i], cz[i]);
        gp_Ax2 ax(center, gp::DZ());
        gp_Circ circle(ax, radius[i]);

        Handle(Geom_TrimmedCurve) arc = GC_MakeArcOfCircle(circle,
            startAngle[i] * M_PI / 180.0,
            endAngle[i] * M_PI / 180.0,
            true);

        Handle(AIS_Shape) aisArc = new AIS_Shape(BRepBuilderAPI_MakeEdge(arc));
        aisArc->SetDisplayMode(AIS_WireFrame);

        // Map DXF linetype string to Aspect_TypeOfLine
        Aspect_TypeOfLine occType = Aspect_TOL_SOLID;
        if (lineTypes != nullptr && i < lineTypes->Length && lineTypes[i] != nullptr)
        {
            System::String^ s = lineTypes[i]->Trim()->ToUpperInvariant();
            if (s->IndexOf("DASH") >= 0)         occType = Aspect_TOL_DASH;
            else if (s->IndexOf("DOT") >= 0)     occType = Aspect_TOL_DOT;
            else if (s->IndexOf("CENTER") >= 0)  occType = Aspect_TOL_DOTDASH;
        }

        // Color
        double dr = (r != nullptr && i < r->Length) ? (double)r[i] / 255.0 : 0.5;
        double dg = (g != nullptr && i < g->Length) ? (double)g[i] / 255.0 : 0.5;
        double db = (b != nullptr && i < b->Length) ? (double)b[i] / 255.0 : 0.5;
        Quantity_Color qcol(dr, dg, db, Quantity_TOC_RGB);

        Handle(Prs3d_Drawer) drawer = aisArc->Attributes();
        if (drawer.IsNull()) drawer = new Prs3d_Drawer();

        Handle(Prs3d_LineAspect) lineAsp = new Prs3d_LineAspect(qcol, occType, 1.0);
        drawer->SetWireAspect(lineAsp);
        aisArc->SetAttributes(drawer);

        ctx->Display(aisArc, Standard_False);
        ctx->SetColor(aisArc, qcol, Standard_False);

        if (transparency != nullptr && i < transparency->Length)
            ctx->SetTransparency(aisArc, transparency[i], Standard_False);

        aisArc->Attributes()->SetLineAspect(lineAsp);
        ctx->Redisplay(aisArc, Standard_False);

        ctx->Activate(aisArc);

        ids[i] = (int)aisArc.get();
    }

    ctx->UpdateCurrentViewer();
    return ids;
}

System::IntPtr ArcDrawer::MakeArc(double cx, double cy, double cz,
    double radius, double startAngle, double endAngle)
{
    gp_Pnt center(cx, cy, cz);
    gp_Ax2 ax(center, gp::DZ());
    gp_Circ circle(ax, radius);

    Handle(Geom_TrimmedCurve) arc = GC_MakeArcOfCircle(circle,
        startAngle * M_PI / 180.0,
        endAngle * M_PI / 180.0,
        true);

    Handle(AIS_Shape) shape = new AIS_Shape(BRepBuilderAPI_MakeEdge(arc));
    return System::IntPtr(shape.get());
}