﻿
#include "pch.h"
#include "GeometryHelper.h"
#include "NativeViewerHandle.h"
#include <BRepAdaptor_Surface.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <GeomLProp_SLProps.hxx>
#include <iostream>
#include <cmath>
#include "ShapeDrawer.h"
#include "ShapeExtruder.h"

#include "ShapeRevolver.h"
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <AIS_Shape.hxx>
#include <AIS_InteractiveObject.hxx>
#include <TopExp_Explorer.hxx>

using namespace PotaOCC;

// 🔹 Helper to clamp values safely
inline double ClampValue(double val, double minVal, double maxVal)
{
    return (val < minVal) ? minVal : (val > maxVal) ? maxVal : val;
}

// ✅ Compute normal vector of a face (world coordinates)
gp_Vec GeometryHelper::ComputeNormal(const TopoDS_Face& face)
{
    BRepAdaptor_Surface surface(face, Standard_True);

    Standard_Real uMid = (surface.FirstUParameter() + surface.LastUParameter()) / 2.0;
    Standard_Real vMid = (surface.FirstVParameter() + surface.LastVParameter()) / 2.0;

    GeomLProp_SLProps props(surface.Surface().Surface(), uMid, vMid, 1, 1e-6);
    if (!props.IsNormalDefined())
    {
        std::cerr << "⚠️ Normal not defined for this face!" << std::endl;
        return gp_Vec(0, 0, 1);
    }

    gp_Dir normal = props.Normal();
    if (face.Orientation() == TopAbs_REVERSED)
        normal.Reverse();

    return gp_Vec(normal);
}

// ✅ Compute transformation to align two faces
gp_Trsf GeometryHelper::ComputeFaceToFaceMate(const gp_Pnt& p1, const gp_Vec& n1,
    const gp_Pnt& p2, const gp_Vec& n2)
{
    gp_Vec normal1 = n1.Normalized();
    gp_Vec normal2 = n2.Normalized();
    gp_Vec target = -normal2;

    gp_Vec axis = normal1.Crossed(target);
    double dot = normal1.Dot(target);

    gp_Trsf rotation;

    if (axis.Magnitude() < 1e-9)
    {
        // Parallel or anti-parallel
        if (dot < 0.9999)
        {
            gp_Vec ref(1, 0, 0);
            if (Abs(normal1.X()) > 0.9)
                ref = gp_Vec(0, 1, 0);

            axis = normal1.Crossed(ref);
            axis.Normalize();

            gp_Ax1 ax(p1, gp_Dir(axis));
            rotation.SetRotation(ax, M_PI);
        }
        else
        {
            rotation.SetRotation(gp_Ax1(p1, gp::DZ()), 0.0);
        }
    }
    else
    {
        axis.Normalize();
        double angle = acos(ClampValue(dot, -1.0, 1.0));
        gp_Ax1 ax(p1, gp_Dir(axis));
        rotation.SetRotation(ax, angle);
    }

    gp_Pnt p1_rotated = p1.Transformed(rotation);
    gp_Vec translationVec(p1_rotated, p2);

    gp_Trsf translation;
    translation.SetTranslation(translationVec);

    return translation.Multiplied(rotation);
}

// ✅ Apply transformation to shape
void GeometryHelper::ApplyTransformationToShape(TopoDS_Shape& shape, const gp_Trsf& transform)
{
    BRepBuilderAPI_Transform transformer(shape, transform, Standard_True);
    if (!transformer.IsDone())
    {
        std::cerr << "❌ Transformation failed!" << std::endl;
        return;
    }
    shape = transformer.Shape();
}

bool GeometryHelper::PerformRectangleSelection(
    NativeViewerHandle* native,
    const Handle(AIS_InteractiveContext)& context,
    const Handle(V3d_View)& view,
    int h,
    int w,
    int mouseY)
{
    if (native == nullptr || context.IsNull() || view.IsNull())
        return false;

    // compute rectangle coords
    int xMin = std::min(native->dragStartX, native->dragEndX);
    int yMin = std::min(native->dragStartY, native->dragEndY);
    int xMax = std::max(native->dragStartX, native->dragEndX);
    int yMax = std::max(native->dragStartY, native->dragEndY);

    // Perform selection with AIS
    context->Select(xMin, yMin, xMax, yMax, view, Standard_False);

    // Collect selected AIS objects
    bool isAnyObjectSelected = false;
    AIS_ListOfInteractive picked;
    for (context->InitSelected(); context->MoreSelected(); context->NextSelected())
    {
        Handle(AIS_InteractiveObject) obj = context->SelectedInteractive();
        if (!obj.IsNull())
        {
            picked.Append(obj);
            isAnyObjectSelected = true;
        }
    }

    // Highlight selected objects (existing helper)
    ShapeDrawer::HighlightSelectedObjects(context, picked);

    // --- Extrude handling (same logic as original) ---
    if (isAnyObjectSelected && native->isExtrudeMode)
    {
        for (AIS_ListOfInteractive::Iterator it(picked); it.More(); it.Next())
        {
            Handle(AIS_InteractiveObject) obj = it.Value();
            Handle(AIS_Shape) aisShape = Handle(AIS_Shape)::DownCast(obj);
            if (aisShape.IsNull()) continue;

            TopoDS_Shape shape = aisShape->Shape();
            if (shape.IsNull()) continue;

            if (shape.ShapeType() == TopAbs_EDGE)
            {
                TopoDS_Edge detectedEdge = TopoDS::Edge(shape);
                if (detectedEdge.IsNull())
                {
                    std::cout << "Edge is invalid!" << std::endl;
                    return true; // processed selection (caller will reset)
                }

                TopoDS_Wire wire = ShapeDrawer::BuildWireFromSelection(picked);
                if (wire.IsNull())
                {
                    wire = BRepBuilderAPI_MakeWire(detectedEdge);
                }

                native->activeWire = wire;
                native->isExtrudingActive = true;
                native->currentExtrudeHeight = 0.0;
                native->lastMouseY = mouseY;

                // Compute centroid as base
                Bnd_Box bbox;
                BRepBndLib::Add(native->activeWire, bbox);
                Standard_Real xmin, ymin, zmin, xmax, ymax, zmax;
                bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);

                gp_Pnt centroid((xmin + xmax) / 2.0,
                    (ymin + ymax) / 2.0,
                    (zmin + zmax) / 2.0);

                native->extrudeBasePoint = centroid;

                // only handle first edge-case like original (others ignored)
                break;
            }
            else if (shape.ShapeType() == TopAbs_FACE)
            {
                TopoDS_Face detectedFace = TopoDS::Face(shape);

                // Extract the wire from the face
                TopoDS_Wire wire;
                for (TopExp_Explorer exp(detectedFace, TopAbs_WIRE); exp.More(); exp.Next())
                {
                    wire = TopoDS::Wire(exp.Current());
                    break; // only handle first wire
                }

                native->activeWire = wire;
                native->isExtrudingActive = true;
                native->currentExtrudeHeight = 0.0;
                native->lastMouseY = mouseY;

                // Compute centroid as base
                Bnd_Box bbox;
                BRepBndLib::Add(native->activeWire, bbox);
                Standard_Real xmin, ymin, zmin, xmax, ymax, zmax;
                bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);

                gp_Pnt centroid((xmin + xmax) / 2.0,
                    (ymin + ymax) / 2.0,
                    (zmin + zmax) / 2.0);

                native->extrudeBasePoint = centroid;

                //ShapeExtruder::ExtrudeWireAndDisplay(context, wire);
            }
            else if (shape.ShapeType() == TopAbs_WIRE)
            {
                TopoDS_Wire detectedWire = TopoDS::Wire(shape);
                ShapeExtruder::ExtrudeWireAndDisplay(context, detectedWire);
                break;
            }
            else
            {
                std::cout << "Detected shape is not a closed wire!" << std::endl;
            }
        }

        return true; // selection handled -> caller should ResetDragState() and ClearRubberBand()
    }

    // --- Revolve handling (use native->isRevolveMode & axis) ---
    if (isAnyObjectSelected && native->isRevolveMode && native->isRevolveAxisSet)
    {
        // Build profile wire from selection (same intent as original code)
        TopoDS_Wire revolveWire = ShapeDrawer::BuildWireFromSelection(picked);

        // fallback: single edge -> make wire
        if (revolveWire.IsNull())
        {
            for (AIS_ListOfInteractive::Iterator it(picked); it.More(); it.Next())
            {
                Handle(AIS_InteractiveObject) obj = it.Value();
                Handle(AIS_Shape) aisShape = Handle(AIS_Shape)::DownCast(obj);
                if (aisShape.IsNull()) continue;

                TopoDS_Shape shape = aisShape->Shape();
                if (shape.IsNull()) continue;

                if (shape.ShapeType() == TopAbs_EDGE)
                {
                    revolveWire = BRepBuilderAPI_MakeWire(TopoDS::Edge(shape));
                    break;
                }
            }
        }

        if (revolveWire.IsNull())
        {
            std::cout << "⚠️ No valid wire found for revolve." << std::endl;
            return true;
        }

        // Ensure face can be made from the wire before revolve
        BRepBuilderAPI_MakeFace mkFace(revolveWire);
        if (!mkFace.IsDone())
        {
            std::cout << "⚠️ Unable to make face from wire for revolve." << std::endl;
            return true;
        }

        // Perform revolve using the axis stored in native (first selected line)
        ShapeRevolver revolver(native);
        revolver.RevolveWireAndDisplayFinal(context, revolveWire, 360.0);

        // Store and reset revolve state as original code did
        native->revolveWire = revolveWire;
        native->isRevolveMode = false;
        native->isRevolveAxisSet = false;

        std::cout << "🌀 Revolve complete around first mouse-selected line." << std::endl;

        return true; // processed selection
    }

    // nothing to do
    return false;
}