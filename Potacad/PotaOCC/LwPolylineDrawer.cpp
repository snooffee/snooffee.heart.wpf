#include "pch.h"
#include "NativeViewerHandle.h"
#include "LwPolylineDrawer.h"
#include <AIS_InteractiveContext.hxx>
#include <V3d_Viewer.hxx>
#include <V3d_View.hxx>
#include <AIS_Shape.hxx>
#include <TopoDS_Edge.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <gp_Pnt.hxx>
#include <Quantity_Color.hxx>
#include <TopoDS_Compound.hxx>
#include <BRep_Builder.hxx>
#include <GC_MakeArcOfCircle.hxx>

using namespace PotaOCC;

array<int>^ LwPolylineDrawer::DrawLwPolylineBatch(
    IntPtr ctxPtr,
    array<double>^ x,
    array<double>^ y,
    array<double>^ z,
    array<double>^ bulge,
    array<int>^ r,
    array<int>^ g,
    array<int>^ b,
    array<double>^ transparency,
    array<bool>^ closed)
{
    if (ctxPtr == IntPtr::Zero) return gcnew array<int>(0);
    AIS_InteractiveContext* rawCtx = static_cast<AIS_InteractiveContext*>(ctxPtr.ToPointer());
    if (!rawCtx) return gcnew array<int>(0);
    if (x->Length < 2) return gcnew array<int>(0);

    Handle(AIS_InteractiveContext) ctx(rawCtx);

    try
    {
        BRep_Builder builder;
        TopoDS_Compound compound;
        builder.MakeCompound(compound);

        for (int i = 0; i < x->Length - 1; i++)
        {
            gp_Pnt p1(x[i], y[i], z[i]);
            gp_Pnt p2(x[i + 1], y[i + 1], z[i + 1]);

            // Handle bulge: if zero, straight edge; otherwise approximate arc
            if (bulge[i] == 0.0)
            {
                TopoDS_Edge edge = BRepBuilderAPI_MakeEdge(p1, p2);
                builder.Add(compound, edge);
            }
            else
            {
                // Approximate bulge as a circular arc segment
                Standard_Real bulgeValue = bulge[i];
                gp_Vec chordVec(p1, p2);
                Standard_Real chordLength = chordVec.Magnitude();

                // Compute sagitta for arc
                Standard_Real sagitta = bulgeValue * chordLength / 2.0;
                gp_Pnt mid((p1.X() + p2.X()) / 2.0, (p1.Y() + p2.Y()) / 2.0, (p1.Z() + p2.Z()) / 2.0);

                // Perpendicular vector in XY plane
                gp_Vec perpVec(-chordVec.Y(), chordVec.X(), 0);
                perpVec.Normalize();
                perpVec.Multiply(sagitta);

                gp_Pnt arcMid = mid.Translated(perpVec);

                Handle(Geom_TrimmedCurve) arc = GC_MakeArcOfCircle(p1, arcMid, p2);
                TopoDS_Edge edge = BRepBuilderAPI_MakeEdge(arc);
                builder.Add(compound, edge);
            }
        }

        // Close polyline if requested
        if (closed[0])
        {
            gp_Pnt pStart(x[0], y[0], z[0]);
            gp_Pnt pEnd(x[x->Length - 1], y[y->Length - 1], z[z->Length - 1]);
            TopoDS_Edge edge = BRepBuilderAPI_MakeEdge(pEnd, pStart);
            builder.Add(compound, edge);
        }

        Handle(AIS_Shape) aisShape = new AIS_Shape(compound);

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