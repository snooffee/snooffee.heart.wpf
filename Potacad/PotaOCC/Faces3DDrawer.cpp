#include "pch.h"
#include "NativeViewerHandle.h"
#include "Faces3DDrawer.h"

#include <AIS_InteractiveContext.hxx>
#include <AIS_Shape.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <TopoDS_Compound.hxx>
#include <BRep_Builder.hxx>
#include <gp_Pnt.hxx>
#include <Quantity_Color.hxx>
#include <TopoDS.hxx>

using namespace PotaOCC;

array<int>^ Faces3DDrawer::DrawFaces3DBatch(
    IntPtr ctxPtr,
    array<double>^ x1, array<double>^ y1, array<double>^ z1,
    array<double>^ x2, array<double>^ y2, array<double>^ z2,
    array<double>^ x3, array<double>^ y3, array<double>^ z3,
    array<double>^ x4, array<double>^ y4, array<double>^ z4,
    array<int>^ r, array<int>^ g, array<int>^ b,
    array<double>^ transparency)
{
    // ✅ Safety checks
    if (ctxPtr == IntPtr::Zero) return gcnew array<int>(0);
    AIS_InteractiveContext* rawCtx = static_cast<AIS_InteractiveContext*>(ctxPtr.ToPointer());
    if (!rawCtx) return gcnew array<int>(0);
    if (x1->Length == 0 || x2->Length == 0 || x3->Length == 0) return gcnew array<int>(0);

    Handle(AIS_InteractiveContext) ctx(rawCtx);
    int n = x1->Length;

    array<int>^ ids = gcnew array<int>(n);

    try
    {
        for (int i = 0; i < n; i++)
        {
            // ✅ Create polygon for each face
            BRepBuilderAPI_MakePolygon polygon;
            gp_Pnt p1(x1[i], y1[i], z1[i]);
            gp_Pnt p2(x2[i], y2[i], z2[i]);
            gp_Pnt p3(x3[i], y3[i], z3[i]);
            polygon.Add(p1);
            polygon.Add(p2);
            polygon.Add(p3);

            // Some faces have 4th vertex (solid quad)
            if (i < x4->Length && !std::isnan(x4[i]))
            {
                gp_Pnt p4(x4[i], y4[i], z4[i]);
                polygon.Add(p4);
            }

            polygon.Close();

            // ✅ Create planar face from polygon
            TopoDS_Face face = BRepBuilderAPI_MakeFace(polygon.Wire());
            Handle(AIS_Shape) aisFace = new AIS_Shape(face);

            // ✅ Apply color
            Quantity_Color col(r[i] / 255.0, g[i] / 255.0, b[i] / 255.0, Quantity_TOC_RGB);
            ctx->SetColor(aisFace, col, Standard_False);

            // ✅ Apply transparency
            double tVal = (i < transparency->Length) ? transparency[i] : 0.0;
            ctx->SetTransparency(aisFace, tVal, Standard_False);

            // ✅ Display the face
            ctx->Display(aisFace, Standard_False);

            // ✅ Store ID (for your _dxfShapeDict)
            ids[i] = (int)aisFace.get();
        }

        ctx->UpdateCurrentViewer();
        return ids;
    }
    catch (...)
    {
        return gcnew array<int>(0);
    }
}