#include "pch.h"
#include "MateHelper.h"
#include "GeometryHelper.h"
#include <AIS_Shape.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <gp_Pnt.hxx>
#include <gp_Trsf.hxx>
#include <gp_Vec.hxx>
#include <TopoDS.hxx>
using namespace PotaOCC;
namespace PotaOCC
{
    namespace MateHelper
    {
        void HandleFaceAlignment(Handle(AIS_InteractiveContext) context, Handle(V3d_View) view, Handle(AIS_Shape) aisShape1, const TopoDS_Face& face1, Handle(AIS_Shape) aisShape2, const TopoDS_Face& face2)
        {
            if (aisShape1.IsNull() || aisShape2.IsNull())return;
            gp_Trsf loc1 = aisShape1->LocalTransformation();
            gp_Trsf loc2 = aisShape2->LocalTransformation();
            TopoDS_Face face1_world = TopoDS::Face(BRepBuilderAPI_Transform(face1, loc1, true).Shape());
            TopoDS_Face face2_world = TopoDS::Face(BRepBuilderAPI_Transform(face2, loc2, true).Shape());
            gp_Vec normal1 = GeometryHelper::ComputeNormal(face1_world);
            gp_Vec normal2 = GeometryHelper::ComputeNormal(face2_world);
            BRepAdaptor_Surface surf1(face1_world);
            BRepAdaptor_Surface surf2(face2_world);
            gp_Pnt p1 = surf1.Value((surf1.FirstUParameter() + surf1.LastUParameter()) / 2.0, (surf1.FirstVParameter() + surf1.LastVParameter()) / 2.0);
            gp_Pnt p2 = surf2.Value((surf2.FirstUParameter() + surf2.LastUParameter()) / 2.0, (surf2.FirstVParameter() + surf2.LastVParameter()) / 2.0);
            gp_Trsf mateWorld = GeometryHelper::ComputeFaceToFaceMate(p1, normal1, p2, normal2);
            gp_Trsf mateLocal = loc1.Inverted().Multiplied(mateWorld).Multiplied(loc1);
            gp_Trsf finalTrsf = aisShape1->LocalTransformation().Multiplied(mateLocal);
            TopoDS_Shape oldShape = aisShape1->Shape();
            BRepBuilderAPI_Transform trsfBuilder(oldShape, finalTrsf, true);
            TopoDS_Shape newShape = trsfBuilder.Shape();
            aisShape1->Set(newShape);
            aisShape1->SetLocalTransformation(gp_Trsf());
            context->Redisplay(aisShape1, Standard_False);
            view->Redraw();
        }
    }
}