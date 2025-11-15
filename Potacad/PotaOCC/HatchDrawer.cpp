#include "pch.h"
#include "HatchDrawer.h"
#include <Standard_Type.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Wire.hxx>
#include <TopoDS_Edge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepAlgoAPI_Section.hxx>
#include <AIS_InteractiveContext.hxx>
#include <AIS_Shape.hxx>
#include <Quantity_Color.hxx>
#include <Graphic3d_NameOfMaterial.hxx>
#include <vector>
#include <BRepClass_FaceClassifier.hxx>

using namespace PotaOCC;

array<int>^ HatchDrawer::DrawHatchBatch(
    IntPtr viewerHandlePtr,
    array<array<array<double>^>^>^ allBoundariesX, // outer + inner boundaries
    array<array<array<double>^>^>^ allBoundariesY,
    array<array<array<double>^>^>^ allBoundariesZ,
    array<int>^ r, array<int>^ g, array<int>^ b,
    array<double>^ transparency,
    array<String^>^ pattern,
    array<bool>^ solid)
{
    if (viewerHandlePtr == IntPtr::Zero) return nullptr;

    NativeViewerHandle* native = (NativeViewerHandle*)viewerHandlePtr.ToPointer();
    if (!native || !native->context) return nullptr;

    Handle(AIS_InteractiveContext) context = native->context;
    if (context.IsNull()) return nullptr;

    int count = allBoundariesX->Length;
    array<int>^ ids = gcnew array<int>(count);
    int nextId = 1;

    for (int i = 0; i < count; i++)
    {
        try
        {
            auto boundariesX = allBoundariesX[i];
            auto boundariesY = allBoundariesY[i];
            auto boundariesZ = allBoundariesZ[i];

            if (!boundariesX || !boundariesY || !boundariesZ) continue;
            if (boundariesX->Length == 0) continue;

            // --- 1. Create outer wire (first boundary) ---
            BRepBuilderAPI_MakeWire outerWireMaker;
            int nOuter = boundariesX[0]->Length;
            if (nOuter < 3) continue;

            for (int j = 0; j < nOuter; j++)
            {
                gp_Pnt p1(boundariesX[0][j], boundariesY[0][j], boundariesZ[0][j]);
                gp_Pnt p2(boundariesX[0][(j + 1) % nOuter], boundariesY[0][(j + 1) % nOuter], boundariesZ[0][(j + 1) % nOuter]);
                BRepBuilderAPI_MakeEdge edgeMaker(p1, p2);
                if (edgeMaker.IsDone()) outerWireMaker.Add(edgeMaker.Edge());
            }

            if (!outerWireMaker.IsDone()) continue;
            TopoDS_Wire outerWire = outerWireMaker.Wire();

            // --- 2. Create inner wires (holes) ---
            std::vector<TopoDS_Wire> innerWires;
            for (int k = 1; k < boundariesX->Length; k++) // start from 1: 0 is outer boundary
            {
                int nHole = boundariesX[k]->Length;
                if (nHole < 3) continue;

                BRepBuilderAPI_MakeWire holeWireMaker;
                for (int j = 0; j < nHole; j++)
                {
                    gp_Pnt p1(boundariesX[k][j], boundariesY[k][j], boundariesZ[k][j]);
                    gp_Pnt p2(boundariesX[k][(j + 1) % nHole], boundariesY[k][(j + 1) % nHole], boundariesZ[k][(j + 1) % nHole]);
                    BRepBuilderAPI_MakeEdge edgeMaker(p1, p2);
                    if (edgeMaker.IsDone()) holeWireMaker.Add(edgeMaker.Edge());
                }

                if (holeWireMaker.IsDone())
                    innerWires.push_back(holeWireMaker.Wire());
            }

            // --- 3. Create face with outer + inner wires ---
            BRepBuilderAPI_MakeFace faceMaker(outerWire);
            for (auto& hw : innerWires)
            {
                faceMaker.Add(hw);
            }
            if (!faceMaker.IsDone()) continue;
            TopoDS_Face face = faceMaker.Face();

            // --- 4. Display solid fill ---
            if (solid[i])
            {
                Handle(AIS_Shape) aisFace = new AIS_Shape(face);
                if (!aisFace.IsNull())
                {
                    Quantity_Color col(r[i] / 255.0, g[i] / 255.0, b[i] / 255.0, Quantity_TOC_RGB);
                    aisFace->SetColor(col);
                    aisFace->SetTransparency(transparency[i]);
                    context->Display(aisFace, Standard_True);
                }
            }

            // --- 5. Draw hatch lines clipped to face (respects holes) ---
            double spacing = 1.0;

            // Compute bounding box for outer wire
            Bnd_Box bbox;
            //BRepBndLib::Add(outerWire, bbox);
            Standard_Real xmin, ymin, zmin, xmax, ymax, zmax;
            bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);

            // Loop to draw hatch pattern
            int segments = 20;
            for (double y = ymin; y <= ymax; y += spacing)
            {
                gp_Pnt start(xmin, y, 0);
                gp_Pnt end(xmax, y, 0);

                for (int s = 0; s < segments; s++)
                {
                    double t1 = (double)s / segments;
                    double t2 = (double)(s + 1) / segments;
                    gp_Pnt p1(start.X() + t1 * (end.X() - start.X()), y, 0);
                    gp_Pnt p2(start.X() + t2 * (end.X() - start.X()), y, 0);

                    BRepClass_FaceClassifier classifier;
                    classifier.Perform(face, p1, 1.0e-7);
                    TopAbs_State state = classifier.State();

                    if (state == TopAbs_IN || state == TopAbs_ON)
                    {
                        BRepBuilderAPI_MakeEdge edgeMaker(p1, p2);
                        if (!edgeMaker.IsDone()) continue;

                        Handle(AIS_Shape) aisLine = new AIS_Shape(edgeMaker.Edge());
                        Quantity_Color colLine(r[i] / 255.0, g[i] / 255.0, b[i] / 255.0, Quantity_TOC_RGB);
                        aisLine->SetColor(colLine);
                        aisLine->SetDisplayMode(AIS_WireFrame);
                        context->Display(aisLine, Standard_False);
                    }
                }
            }
            context->UpdateCurrentViewer();


            ids[i] = nextId++;
        }
        catch (...)
        {
            continue;
        }
    }

    return ids;
}