#include "pch.h"
#include <msclr/marshal.h>
#include "DimensionDrawer.h"
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <msclr/marshal_cppstd.h>

using namespace PotatoOCC;

array<int>^ DimensionDrawer::DrawDimensionBatch(
    System::IntPtr ctxPtr,
    array<double>^ startX, array<double>^ startY, array<double>^ startZ,
    array<double>^ endX, array<double>^ endY, array<double>^ endZ,
    array<double>^ leaderX, array<double>^ leaderY, array<double>^ leaderZ,
    array<System::String^>^ textValues,
    array<System::String^>^ dimTypes,
    array<int>^ r, array<int>^ g, array<int>^ b)
{
    if (ctxPtr == System::IntPtr::Zero)
        return gcnew array<int>(0);

    AIS_InteractiveContext* rawCtx = static_cast<AIS_InteractiveContext*>(ctxPtr.ToPointer());
    if (!rawCtx) return gcnew array<int>(0);

    Handle(AIS_InteractiveContext) ctx(rawCtx);
    int n = startX->Length;
    auto ids = gcnew array<int>(n);

    for (int i = 0; i < n; ++i)
    {
        // Create dimension line
        gp_Pnt p1(startX[i], startY[i], startZ[i]);
        gp_Pnt p2(endX[i], endY[i], endZ[i]);
        gp_Pnt leader(leaderX[i], leaderY[i], leaderZ[i]);

        Handle(Geom_TrimmedCurve) dimEdge = GC_MakeSegment(p1, p2);
        Handle(AIS_Shape) aisDim = new AIS_Shape(BRepBuilderAPI_MakeEdge(dimEdge));
        aisDim->SetDisplayMode(AIS_WireFrame);

        // Map line type
        Aspect_TypeOfLine occType = Aspect_TOL_SOLID;
        if (dimTypes != nullptr && i < dimTypes->Length && dimTypes[i] != nullptr)
        {
            System::String^ type = dimTypes[i]->Trim()->ToUpperInvariant();
            if (type->Contains("DASH")) occType = Aspect_TOL_DASH;
            else if (type->Contains("DOT")) occType = Aspect_TOL_DOT;
            else if (type->Contains("CENTER")) occType = Aspect_TOL_DOTDASH;
        }

        // Set color
        double dr = (r != nullptr && i < r->Length) ? (double)r[i] / 255.0 : 0.5;
        double dg = (g != nullptr && i < g->Length) ? (double)g[i] / 255.0 : 0.5;
        double db = (b != nullptr && i < b->Length) ? (double)b[i] / 255.0 : 0.5;
        Quantity_Color qcol(dr, dg, db, Quantity_TOC_RGB);

        Handle(Prs3d_Drawer) drawer = aisDim->Attributes();
        if (drawer.IsNull()) drawer = new Prs3d_Drawer();
        Handle(Prs3d_LineAspect) lineAsp = new Prs3d_LineAspect(qcol, occType, 1.0);
        drawer->SetWireAspect(lineAsp);
        aisDim->SetAttributes(drawer);

        // Display dimension line
        ctx->Display(aisDim, false);
        ctx->SetColor(aisDim, qcol, false);

        // Add text label at leader point
        if (textValues != nullptr && i < textValues->Length && textValues[i] != nullptr)
        {
            Handle(AIS_TextLabel) label = new AIS_TextLabel();
            label->SetPosition(leader);

            // Convert managed string to native char* using marshal_context
            msclr::interop::marshal_context context;
            const char* nativeText = context.marshal_as<const char*>(textValues[i]);

            label->SetText(TCollection_ExtendedString(nativeText));

            ctx->Display(label, false);
            ctx->SetColor(label, qcol, false);
        }
        ids[i] = (int)aisDim.get();
    }

    ctx->UpdateCurrentViewer();
    return ids;
}