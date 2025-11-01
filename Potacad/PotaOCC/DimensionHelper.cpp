#include "pch.h"
#include "DimensionHelper.h"
#include "NativeViewerHandle.h"
#include "TextDrawer.h"
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
using namespace PotaOCC;
void DimensionHelper::ClearPreviousOverlays(NativeViewerHandle* native, Handle(AIS_InteractiveContext) context, bool isFinal)
{
    if (!isFinal) {
        if (!native->dimLineTemp.IsNull()) {
            context->Remove(native->dimLineTemp, Standard_False);
            native->dimLineTemp.Nullify();
        }
        if (!native->dimLabelTemp.IsNull()) {
            context->Remove(native->dimLabelTemp, Standard_False);
            native->dimLabelTemp.Nullify();
        }
        if (!native->dimExtLine1Temp.IsNull()) {
            context->Remove(native->dimExtLine1Temp, Standard_False);
            native->dimExtLine1Temp.Nullify();
        }
        if (!native->dimExtLine2Temp.IsNull()) {
            context->Remove(native->dimExtLine2Temp, Standard_False);
            native->dimExtLine2Temp.Nullify();
        }
        if (!native->dimArrow1Temp.IsNull()) {
            context->Remove(native->dimArrow1Temp, Standard_False);
            native->dimArrow1Temp.Nullify();
        }
        if (!native->dimArrow2Temp.IsNull()) {
            context->Remove(native->dimArrow2Temp, Standard_False);
            native->dimArrow2Temp.Nullify();
        }
    }
}
void DimensionHelper::DrawExtensionLine(NativeViewerHandle* native, Handle(AIS_InteractiveContext) context, const gp_Pnt& start, const gp_Pnt& end, bool isFinal)
{
    gp_Vec extVec(start, end);
    if (extVec.Magnitude() > 1e-6) {
        BRepBuilderAPI_MakeEdge extEdge(start, end);
        Handle(AIS_Shape) extLine = new AIS_Shape(extEdge.Edge());
        extLine->SetColor(Quantity_NOC_YELLOW);
        if (isFinal)
            context->Display(extLine, Standard_True);
        else {
            native->dimExtLine1Temp = extLine;
            context->Display(native->dimExtLine1Temp, Standard_False);
        }
    }
}
void DimensionHelper::DrawDimensionLine(NativeViewerHandle* native, Handle(AIS_InteractiveContext) context, const gp_Pnt& p1, const gp_Pnt& p2, bool isFinal)
{
    BRepBuilderAPI_MakeEdge dimEdge(p1, p2);
    Handle(AIS_Shape) dimLineShape = new AIS_Shape(dimEdge.Edge());
    dimLineShape->SetColor(Quantity_NOC_YELLOW);
    if (isFinal)
        context->Display(dimLineShape, Standard_True);
    else {
        native->dimLineTemp = dimLineShape;
        context->Display(native->dimLineTemp, Standard_False);
    }
}
void DimensionHelper::DrawArrowheads(NativeViewerHandle* native, Handle(AIS_InteractiveContext) context, const gp_Pnt& p1, const gp_Pnt& p2, const gp_Vec& edgeVec, bool isFinal)
{
    double arrowLength = 2.5;
    double arrowAngle = M_PI / 8;

    auto MakeArrow = [&](const gp_Pnt& tip, const gp_Vec& direction)->Handle(AIS_Shape) {
        gp_Vec dir = direction.Normalized() * arrowLength;
        gp_Vec perp1(-dir.Y(), dir.X(), 0.0);
        gp_Vec perp2 = -perp1;

        gp_Pnt base1 = tip.Translated(-dir + perp1 * arrowLength / 2);
        gp_Pnt base2 = tip.Translated(-dir + perp2 * arrowLength / 2);

        BRepBuilderAPI_MakeWire wireMaker;
        wireMaker.Add(BRepBuilderAPI_MakeEdge(tip, base1).Edge());
        wireMaker.Add(BRepBuilderAPI_MakeEdge(tip, base2).Edge());

        Handle(AIS_Shape) arrow = new AIS_Shape(wireMaker.Wire());
        arrow->SetColor(Quantity_NOC_YELLOW);
        return arrow;
    };

    Handle(AIS_Shape) arrow1 = MakeArrow(p1, -edgeVec);
    Handle(AIS_Shape) arrow2 = MakeArrow(p2, edgeVec);

    if (isFinal) {
        context->Display(arrow1, Standard_True);
        context->Display(arrow2, Standard_True);
    }
    else {
        native->dimArrow1Temp = arrow1;
        native->dimArrow2Temp = arrow2;
        context->Display(native->dimArrow1Temp, Standard_False);
        context->Display(native->dimArrow2Temp, Standard_False);
    }
}
void DimensionHelper::DrawLabel(NativeViewerHandle* native, Handle(AIS_InteractiveContext) context, const gp_Pnt& dimLineP1, const gp_Pnt& dimLineP2, const gp_Pnt& p1, const gp_Pnt& p2, const gp_Vec& edgeVec, bool isFinal)
{
    gp_Pnt midPoint((dimLineP1.X() + dimLineP2.X()) / 2.0,
        (dimLineP1.Y() + dimLineP2.Y()) / 2.0,
        (dimLineP1.Z() + dimLineP2.Z()) / 2.0);
    gp_Pnt labelPos = midPoint.Translated(gp_Vec(0, 0, 0.1));

    double dist = p1.Distance(p2);
    // Use ostringstream to control the number of decimals
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << dist;  // Set precision to 2 decimal places
    std::string text = oss.str();  // Convert to string

    double angle = std::atan2(edgeVec.Y(), edgeVec.X()) * 180.0 / M_PI;

    Handle(AIS_Shape) labelShape = TextDrawer::DrawSingleText(
        System::IntPtr(native),
        labelPos.X(), labelPos.Y(), labelPos.Z(),
        gcnew System::String(text.c_str()),
        3.5,
        angle,
        2.0,
        255, 255, 0
    );

    if (!labelShape.IsNull()) {
        if (isFinal) context->Display(labelShape, Standard_True);
        else {
            native->dimLabelTemp = labelShape;
            context->Display(native->dimLabelTemp, Standard_False);
        }
    }
}