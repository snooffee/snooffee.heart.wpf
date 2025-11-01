#include "pch.h"
#include "NativeViewerHandle.h"
#include "Print.h"
#include "ShapeDrawer.h"
#include <AIS_InteractiveContext.hxx>
#include <GC_MakeSegment.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <V3d_Viewer.hxx>
#include <V3d_View.hxx>
#include <AIS_Shape.hxx>
#include <TopoDS_Edge.hxx>
#include <Font_BRepTextBuilder.hxx>
#include <TopExp_Explorer.hxx>
#include <Geom_Circle.hxx>
#include <Geom_Line.hxx>

using namespace PotaOCC;

void Print::ShapeDetails(Handle(AIS_Shape) aisShape)
{
    TopoDS_Shape detectedShape = aisShape->Shape();
    TopAbs_ShapeEnum shapeType = detectedShape.ShapeType();

    switch (shapeType)
    {
    case TopAbs_EDGE: EdgeDetails(detectedShape); break;
    case TopAbs_VERTEX: VertexDetails(detectedShape); break;
    case TopAbs_WIRE: PrintWireDetails(detectedShape); break;
    case TopAbs_COMPOUND: std::cout << "Compound shape selected." << std::endl; break;
    default: std::cout << "Unsupported shape type selected." << std::endl; break;
    }
}

void Print::EdgeDetails(TopoDS_Shape& detectedShape)
{
    TopoDS_Edge edge = TopoDS::Edge(detectedShape);
    Standard_Real first, last;
    Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, first, last);

    if (curve.IsNull()) {
        std::cout << "Edge detected but no underlying curve found." << std::endl;
        return;
    }

    Handle(Geom_Line) geomLine = Handle(Geom_Line)::DownCast(curve);
    if (!geomLine.IsNull()) {
        gp_Pnt origin = geomLine->Position().Location();
        gp_Dir direction = geomLine->Position().Direction();
        std::cout << "Line Selected: Origin: " << origin.X() << ", " << origin.Y() << ", " << origin.Z() << std::endl;
        std::cout << "Direction: " << direction.X() << ", " << direction.Y() << ", " << direction.Z() << std::endl;
    }
    else {
        std::cout << "Edge with unsupported curve type detected." << std::endl;
    }
}

void Print::VertexDetails(TopoDS_Shape& detectedShape)
{
    TopoDS_Vertex vertex = TopoDS::Vertex(detectedShape);
    gp_Pnt pnt = BRep_Tool::Pnt(vertex);
    std::cout << "Vertex (Point) Selected: (" << pnt.X() << ", " << pnt.Y() << ", " << pnt.Z() << ")" << std::endl;
}

void Print::PrintWireDetails(TopoDS_Shape& detectedShape)
{
    TopoDS_Wire wire = TopoDS::Wire(detectedShape);
    for (TopExp_Explorer exp(wire, TopAbs_EDGE); exp.More(); exp.Next())
    {
        TopoDS_Edge edge = TopoDS::Edge(exp.Current());
        Standard_Real first, last;
        Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, first, last);
        std::cout << "Edge in wire: ";
        if (Handle(Geom_Line)::DownCast(curve)) {
            std::cout << "Line segment detected." << std::endl;
        }
        else if (Handle(Geom_Circle)::DownCast(curve)) {
            std::cout << "Circle segment detected." << std::endl;
        }
        else {
            std::cout << "Other curve detected." << std::endl;
        }
    }
    std::cout << "------------------------------------------------------------" << std::endl;
}