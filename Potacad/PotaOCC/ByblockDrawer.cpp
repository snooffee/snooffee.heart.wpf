#include "pch.h"
#include "NativeViewerHandle.h"
#include "ByblockDrawer.h"
#include <AIS_InteractiveContext.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Wire.hxx>
#include <TopoDS_Edge.hxx>
#include <gp_Pnt.hxx>
#include <gp_Ax2.hxx>
#include <gp_Circ.hxx>
#include <Geom_TrimmedCurve.hxx>
#include <GC_MakeArcOfCircle.hxx>
#include <V3d_Viewer.hxx>
#include <V3d_View.hxx>
#include <AIS_Shape.hxx>

using namespace PotaOCC;

array<int>^ ByblockDrawer::DrawByblockBatch(
    IntPtr ctxPtr,
    array<double>^ x1, array<double>^ y1, array<double>^ z1,  // Start point coordinates
    array<double>^ x2, array<double>^ y2, array<double>^ z2,  // End point coordinates
    array<int>^ r, array<int>^ g, array<int>^ b,              // Color
    array<double>^ transparency)
{
    if (ctxPtr == IntPtr::Zero) return gcnew array<int>(0);
    AIS_InteractiveContext* rawCtx = static_cast<AIS_InteractiveContext*>(ctxPtr.ToPointer());
    if (!rawCtx) return gcnew array<int>(0);

    int n = x1->Length;
    auto ids = gcnew array<int>(n);

    Handle(AIS_InteractiveContext) ctx(rawCtx);

    // Loop through the input data (lines)
    for (int i = 0; i < n; i++)
    {
        try
        {
            // Define the start and end points of the line
            gp_Pnt start(x1[i], y1[i], z1[i]);  // Start point
            gp_Pnt end(x2[i], y2[i], z2[i]);    // End point

            // Create an edge (line) between the two points
            TopoDS_Edge lineEdge = BRepBuilderAPI_MakeEdge(start, end);

            // Create a shape for displaying
            Handle(AIS_Shape) aisShape = new AIS_Shape(lineEdge);

            // Set color based on the input data
            Quantity_Color col(r[i] / 255.0, g[i] / 255.0, b[i] / 255.0, Quantity_TOC_RGB);
            ctx->SetColor(aisShape, col, Standard_False);
            ctx->SetTransparency(aisShape, transparency[i], Standard_False);

            // Display the shape in the viewer
            ctx->Display(aisShape, Standard_False);

            ids[i] = (int)aisShape.get();
        }
        catch (...)
        {
            ids[i] = 0; // Mark failed line creation
        }
    }

    // Update the viewer to display all the newly created shapes
    ctx->UpdateCurrentViewer();
    return ids;
}