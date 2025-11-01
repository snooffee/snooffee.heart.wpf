#include "pch.h"
#include <msclr/marshal.h>
#include "DimensionDrawer.h"
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <GC_MakeSegment.hxx>
#include <GC_MakeCircle.hxx>
#include <AIS_Shape.hxx>
#include <AIS_TextLabel.hxx>
#include <AIS_Circle.hxx>
#include <Prs3d_Drawer.hxx>
#include <Prs3d_LineAspect.hxx>
#include <Quantity_Color.hxx>
#include <TCollection_ExtendedString.hxx>
#include <cmath>
#include <vector>
#include <TopoDS_Builder.hxx>
#include <BRep_Builder.hxx>

using namespace PotaOCC;

array<int>^ DimensionDrawer::DrawDimensionBatch(
    System::IntPtr ctxPtr,
    array<double>^ startX, array<double>^ startY, array<double>^ startZ,
    array<double>^ endX, array<double>^ endY, array<double>^ endZ,
    array<double>^ leaderX, array<double>^ leaderY, array<double>^ leaderZ,
    array<System::String^>^ textValues,
    array<System::String^>^ dimTypes,
    array<System::String^>^ lineTypes,
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
        System::String^ type = (dimTypes && i < dimTypes->Length && dimTypes[i] != nullptr)
            ? dimTypes[i]->Trim()->ToUpperInvariant()
            : "LINEAR";

        // --- Geometry points ---
        gp_Pnt p1(startX[i], startY[i], startZ[i]);
        gp_Pnt p2(endX[i], endY[i], 0);
        gp_Pnt pText(leaderX[i], leaderY[i], 0);

        // --- Map DXF linetype to OCCT line type ---
        Aspect_TypeOfLine occType = Aspect_TOL_SOLID; // default
        if (lineTypes != nullptr && i < lineTypes->Length && lineTypes[i] != nullptr)
        {
            System::String^ s = lineTypes[i]->Trim()->ToUpperInvariant();
            if (s->IndexOf("DASH") >= 0)            occType = Aspect_TOL_DASH;
            else if (s->IndexOf("DASHED1") >= 0)    occType = Aspect_TOL_DASH;
            else if (s->IndexOf("DASHED2") >= 0)    occType = Aspect_TOL_DASH;
            else if (s->IndexOf("DOT") >= 0)        occType = Aspect_TOL_DOT;
            else if (s->IndexOf("CENTER") >= 0)     occType = Aspect_TOL_DOTDASH;
            else if (s->IndexOf("CENTER1") >= 0)    occType = Aspect_TOL_DOTDASH;
            else if (s->IndexOf("CENTER2") >= 0)    occType = Aspect_TOL_DOTDASH;
        }

        // --- Build per-dimension color ---
        double dr = (r && i < r->Length) ? r[i] / 255.0 : 0.5;
        double dg = (g && i < g->Length) ? g[i] / 255.0 : 0.5;
        double db = (b && i < b->Length) ? b[i] / 255.0 : 0.5;
        Quantity_Color qcol(dr, dg, db, Quantity_TOC_RGB);

        Handle(AIS_InteractiveObject) dimObj;

        // ---- LINEAR / ALIGNED ----
        if (type->Contains("LINEAR") || type->Contains("ALIGNED"))
        {
            Handle(Geom_TrimmedCurve) line = GC_MakeSegment(p1, p2);
            dimObj = new AIS_Shape(BRepBuilderAPI_MakeEdge(line));
        }
        // ---- RADIUS ----
        else if (type->Contains("RADIUS"))
        {
            double radius = p1.Distance(p2);
            gp_Circ circ(gp_Ax2(p1, gp::DZ()), radius);
            dimObj = new AIS_Circle(new Geom_Circle(circ));
        }
        // ---- DIAMETER ----
        else if (type->Contains("DIAMETER"))
        {
            double radius = p1.Distance(p2);
            gp_Circ circ(gp_Ax2(p1, gp::DZ()), radius);
            dimObj = new AIS_Circle(new Geom_Circle(circ));
        }
        // ---- ANGULAR ----
        else if (type->Contains("ANGULAR"))
        {
            gp_Pnt vertex = p1;
            gp_Pnt arm1 = p2;
            gp_Pnt arm2 = pText;

            Handle(AIS_Shape) line1 = new AIS_Shape(BRepBuilderAPI_MakeEdge(vertex, arm1));
            Handle(AIS_Shape) line2 = new AIS_Shape(BRepBuilderAPI_MakeEdge(vertex, arm2));

            // Angle arc
            double radius = vertex.Distance(arm1) * 0.6;
            gp_Circ circ(gp_Ax2(vertex, gp::DZ()), radius);
            gp_Vec v1(vertex, arm1), v2(vertex, arm2);
            double angle = v1.Angle(v2);

            Handle(Geom_Circle) arc = new Geom_Circle(circ);
            Handle(Geom_TrimmedCurve) arcTrim = new Geom_TrimmedCurve(arc, 0, angle);
            Handle(AIS_Shape) arcShape = new AIS_Shape(BRepBuilderAPI_MakeEdge(arcTrim));

            // Combine all into one AIS_Shape
            BRep_Builder builder;
            TopoDS_Compound comp;
            builder.MakeCompound(comp);
            builder.Add(comp, line1->Shape());
            builder.Add(comp, line2->Shape());
            builder.Add(comp, arcShape->Shape());
            dimObj = new AIS_Shape(comp);
        }

        // ---- Display dimension geometry ----
        if (!dimObj.IsNull())
        {
            Handle(Prs3d_Drawer) drawer = new Prs3d_Drawer();
            Handle(Prs3d_LineAspect) lineAsp = new Prs3d_LineAspect(qcol, occType, 1.5);
            drawer->SetWireAspect(lineAsp);
            dimObj->SetAttributes(drawer);

            ctx->Display(dimObj, Standard_False);
            ctx->SetColor(dimObj, qcol, Standard_False);
        }

        // ---- TEXT LABEL ----
        if (textValues && i < textValues->Length && textValues[i] != nullptr)
        {
            Handle(AIS_TextLabel) label = new AIS_TextLabel();
            label->SetPosition(pText);
            msclr::interop::marshal_context mc;
            TCollection_ExtendedString t(mc.marshal_as<const char*>(textValues[i]));
            label->SetText(t);

            Handle(Prs3d_Drawer) textDrawer = new Prs3d_Drawer();
            Handle(Prs3d_TextAspect) textAsp = new Prs3d_TextAspect();
            textAsp->SetColor(qcol);
            textAsp->SetHeight(3.0);
            textDrawer->SetTextAspect(textAsp);
            label->SetAttributes(textDrawer);

            ctx->Display(label, Standard_False);
        }

        ids[i] = reinterpret_cast<int>(dimObj.get());
    }

    ctx->UpdateCurrentViewer();
    return ids;
}