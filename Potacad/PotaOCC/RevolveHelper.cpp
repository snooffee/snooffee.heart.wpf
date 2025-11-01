#include "pch.h"
#include "RevolveHelper.h"
#include "ShapeDrawer.h"
#include <TopoDS.hxx>

using namespace PotaOCC;

bool RevolveHelper::TrySelectRevolveAxis(NativeViewerHandle* native, const Handle(AIS_InteractiveContext)& context)
{
    if (!native)
    {
        std::cout << "❌ Native handle is null." << std::endl;
        return false;
    }
    native->persistedLines.clear();
    if (!context->HasDetected())
    {
        std::cout << "⚠️ No shape detected under cursor." << std::endl;
        return false;
    }

    Handle(AIS_InteractiveObject) detectedIO = context->DetectedInteractive();
    if (detectedIO.IsNull())
    {
        std::cout << "⚠️ No interactive object detected." << std::endl;
        return false;
    }

    // ✅ Make sure the detected shape is an AIS_Shape
    Handle(AIS_Shape) aisLine = Handle(AIS_Shape)::DownCast(detectedIO);
    if (aisLine.IsNull())
    {
        std::cout << "⚠️ Detected object is not an AIS_Shape." << std::endl;
        return false;
    }

    // ✅ Extract its TopoDS_Shape
    TopoDS_Shape shape = aisLine->Shape();
    TopoDS_Edge edge;

    if (shape.ShapeType() == TopAbs_EDGE)
    {
        edge = TopoDS::Edge(shape);
    }
    else if (shape.ShapeType() == TopAbs_WIRE)
    {
        TopExp_Explorer exp(shape, TopAbs_EDGE);
        if (exp.More())
            edge = TopoDS::Edge(exp.Current());
    }

    if (!edge.IsNull())
    {
        // ✅ Extract the endpoints of the edge
        TopoDS_Vertex V1, V2;
        TopExp::Vertices(edge, V1, V2);

        gp_Pnt p1 = BRep_Tool::Pnt(V1);
        gp_Pnt p2 = BRep_Tool::Pnt(V2);

        gp_Dir dir(p2.XYZ() - p1.XYZ());

        // ✅ Set revolve axis in native
        native->revolveAxis = gp_Ax1(p1, dir);
        native->isRevolveAxisSet = true;

        // ✅ Highlight the selected line as the center line
        Handle(AIS_Shape) centerLine = ShapeDrawer::DisplayCenterLine(context, edge);

        // ✅ Save to native for later erase
        native->revolveAxisObj = centerLine;

        // ✅ Store it if you still want it in persistedLines
        native->persistedLines.push_back(centerLine);

        //std::cout << "✅ Revolve axis selected (from clicked line)." << std::endl;
    }
    else
    {
        std::cout << "⚠️ Clicked shape is not a valid line." << std::endl;
    }

    return true;
}