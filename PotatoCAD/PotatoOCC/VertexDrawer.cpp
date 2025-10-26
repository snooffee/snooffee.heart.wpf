#include "pch.h"
#include "NativeViewerHandle.h"
#include "VertexDrawer.h"
#include <AIS_InteractiveContext.hxx>
#include <V3d_Viewer.hxx>
#include <V3d_View.hxx>
#include <AIS_Shape.hxx>
#include <TopoDS_Vertex.hxx>
#include <gp_Pnt.hxx>
#include <Quantity_Color.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>

using namespace PotatoOCC;

array<int>^ VertexDrawer::DrawVertexBatch(
    IntPtr ctxPtr,
    array<double>^ x, array<double>^ y, array<double>^ z,    // Coordinates of the vertex
    array<int>^ r, array<int>^ g, array<int>^ b,              // Color
    array<double>^ transparency)                               // Transparency
{
    if (ctxPtr == IntPtr::Zero) return gcnew array<int>(0);
    AIS_InteractiveContext* rawCtx = static_cast<AIS_InteractiveContext*>(ctxPtr.ToPointer());
    if (!rawCtx) return gcnew array<int>(0);

    int n = x->Length;
    auto ids = gcnew array<int>(n);

    Handle(AIS_InteractiveContext) ctx(rawCtx);

    // Loop through the input data (vertices)
    for (int i = 0; i < n; i++)
    {
        try
        {
            // Define the position of the vertex (point)
            gp_Pnt point(x[i], y[i], z[i]);

            // Create the vertex
            TopoDS_Vertex vertex = BRepBuilderAPI_MakeVertex(point);

            // Create a shape for displaying
            Handle(AIS_Shape) aisShape = new AIS_Shape(vertex);

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
            ids[i] = 0; // Mark failed vertex creation
        }
    }

    // Update the viewer to display all the newly created shapes
    ctx->UpdateCurrentViewer();
    return ids;
}