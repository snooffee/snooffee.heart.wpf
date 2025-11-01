#include "pch.h"
#include "MateHelper.h"
#include <cmath>
#include <iostream>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <GeomLProp_SLProps.hxx>
#include <TopExp_Explorer.hxx>
#include <gp_Trsf.hxx>
#include <gp_Vec.hxx>
#include <gp_Pnt.hxx>
#include <AIS_Shape.hxx>
#include <Bnd_Box.hxx>
#include <BRepBndLib.hxx>
#include <Standard_DefineHandle.hxx>

// ✅ Compute normal vector of a face (world coordinates)
gp_Vec MateHelper::ComputeNormal(const TopoDS_Face & face)
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
gp_Trsf MateHelper::ComputeFaceToFaceMate(const gp_Pnt& p1, const gp_Vec& n1,
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
void MateHelper::ApplyTransformationToShape(TopoDS_Shape& shape, const gp_Trsf& transform)
{
    BRepBuilderAPI_Transform transformer(shape, transform, Standard_True);
    if (!transformer.IsDone())
    {
        std::cerr << "❌ Transformation failed!" << std::endl;
        return;
    }
    shape = transformer.Shape();
}

// ✅ Handle face alignment (picking mode)
void MateHelper::HandleFaceAlignment(Handle(AIS_InteractiveContext) context,
    Handle(V3d_View) view,
    int x, int y,
    Handle(AIS_InteractiveObject) movingObject,
    bool isMoveMode,
    bool isRotateMode)
{
    if (isMoveMode || isRotateMode)
        return;

    context->MoveTo(x, y, view, Standard_True);
    if (!context->HasDetected())
    {
        std::cout << "No face detected!" << std::endl;
        return;
    }

    Handle(AIS_Shape) aisShape1 = Handle(AIS_Shape)::DownCast(movingObject);
    if (aisShape1.IsNull())
        return;
    TopoDS_Shape shape1 = aisShape1->Shape();

    Handle(AIS_Shape) aisShape2 = Handle(AIS_Shape)::DownCast(context->DetectedInteractive());
    if (aisShape2.IsNull())
        return;
    TopoDS_Shape shape2 = aisShape2->Shape();

    TopExp_Explorer exp1(shape1, TopAbs_FACE);
    TopExp_Explorer exp2(shape2, TopAbs_FACE);
    if (!exp1.More() || !exp2.More())
        return;

    TopoDS_Face face1 = TopoDS::Face(exp1.Current());
    TopoDS_Face face2 = TopoDS::Face(exp2.Current());

    // ✅ Transform face2 to world coordinates to account for rotation/translation
    gp_Trsf loc2 = aisShape2->LocalTransformation();
    face2 = TopoDS::Face(BRepBuilderAPI_Transform(face2, loc2, true).Shape());

    gp_Vec normal1 = ComputeNormal(face1);
    gp_Vec normal2 = ComputeNormal(face2);

    BRepAdaptor_Surface surf1(face1);
    BRepAdaptor_Surface surf2(face2);

    gp_Pnt p1 = surf1.Value((surf1.FirstUParameter() + surf1.LastUParameter()) / 2.0,
        (surf1.FirstVParameter() + surf1.LastVParameter()) / 2.0);

    gp_Pnt p2 = surf2.Value((surf2.FirstUParameter() + surf2.LastUParameter()) / 2.0,
        (surf2.FirstVParameter() + surf2.LastVParameter()) / 2.0);

    gp_Trsf transformation = ComputeFaceToFaceMate(p1, normal1, p2, normal2);
    ApplyTransformationToShape(shape1, transformation);

    aisShape1->Set(shape1);
    context->Redisplay(aisShape1, Standard_False);
    view->Redraw();

    std::cout << "✅ Face alignment complete.\n";
}

// ✅ Handle face alignment (manual selection mode)
void MateHelper::HandleFaceAlignment(Handle(AIS_InteractiveContext) context,
    Handle(V3d_View) view,
    Handle(AIS_Shape) aisShape1,
    const TopoDS_Face& face1,
    Handle(AIS_Shape) aisShape2,
    const TopoDS_Face& face2)
{
    // Transform face2 to world coordinates
    gp_Trsf loc2 = aisShape2->LocalTransformation();
    TopoDS_Face face2_world = TopoDS::Face(BRepBuilderAPI_Transform(face2, loc2, true).Shape());

    gp_Vec normal1 = ComputeNormal(face1);
    gp_Vec normal2 = ComputeNormal(face2_world);

    BRepAdaptor_Surface surf1(face1);
    BRepAdaptor_Surface surf2(face2_world);

    gp_Pnt p1 = surf1.Value((surf1.FirstUParameter() + surf1.LastUParameter()) / 2.0,
        (surf1.FirstVParameter() + surf1.LastVParameter()) / 2.0);

    gp_Pnt p2 = surf2.Value((surf2.FirstUParameter() + surf2.LastUParameter()) / 2.0,
        (surf2.FirstVParameter() + surf2.LastVParameter()) / 2.0);

    gp_Trsf transformation = ComputeFaceToFaceMate(p1, normal1, p2, normal2);

    TopoDS_Shape shape1 = aisShape1->Shape();
    ApplyTransformationToShape(shape1, transformation);

    aisShape1->Set(shape1);
    context->Redisplay(aisShape1, Standard_False);
    view->Redraw();
}