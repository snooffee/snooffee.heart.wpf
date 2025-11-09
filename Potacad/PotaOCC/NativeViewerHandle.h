#pragma once

#include <AIS_InteractiveObject.hxx>
#include <AIS_InteractiveContext.hxx>
#include <AIS_Shape.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Wire.hxx>
#include <AIS_TextLabel.hxx>
#include <gp_Pnt2d.hxx>
#include <vector>
#include <vcclr.h>
#include "AIS_OverlayLine.h"
#include "AIS_OverlayRectangle.h"
#include "AIS_OverlayCircle.h"
#include "AIS_OverlayEllipse.h"
#include <BRepLib_MakeFace.hxx>
#using <System.Windows.Forms.dll>
using namespace System::Windows::Forms;

namespace PotaOCC {
    struct NativeViewerHandle {
        // ✅ The OCCT Viewer
        Handle(V3d_Viewer) viewer;
        Handle(V3d_View) view;
        Handle(AIS_InteractiveContext) context;

        Handle(AIS_Shape) box3D;
        Handle(AIS_Shape) axisX;
        Handle(AIS_Shape) axisY;
        Handle(AIS_Shape) axisZ;

        Handle(AIS_OverlayLine) lineOverlay;
        Handle(AIS_OverlayCircle) circleOverlay;
        Handle(AIS_OverlayRectangle) rectangleOverlay;
        Handle(AIS_OverlayEllipse) ellipseOverlay;
        Handle(AIS_InteractiveObject) centerMarker;
        Handle(AIS_InteractiveObject) centerOverlay;

        std::vector<double> pixelHeights;
        std::vector<Handle(AIS_Shape)> ais2DShapes;
        std::vector<Handle(AIS_TextLabel)> aisLabels;

        bool isSelecting = false;
        gp_Pnt2d selectStart;
        gp_Pnt2d selectEnd;

        bool textHeightCallbackSet = false;
        gcroot<System::Windows::Forms::Control^> hostControl;

        int leftButtonAction = 0;
        int middleButtonAction = 0;
        int rightButtonAction = 0;
        int wheelScrollAction = 0;

        bool isDragging = false;
        int dragStartX = 0;
        int dragStartY = 0;
        int dragEndX = 0;
        int dragEndY = 0;

        Handle(Graphic3d_Structure) rubberBandOverlay;
        Handle(Graphic3d_Group) rubberBandGroup;

        bool isLineMode = false;
        std::vector<Handle(AIS_Shape)> persistedLines;

        bool isCircleMode = false;
        std::vector<Handle(AIS_Shape)> persistedCircles;

        bool isRectangleMode = false;
        std::vector<Handle(AIS_Shape)> persistedRectangles;

        bool isEllipseMode = false;
        std::vector<Handle(AIS_Shape)> persistedEllipses;

        std::vector<Handle(AIS_Shape)> persistedHatches;

        bool isTrimMode = false;
        bool isRadiusMode = false;
        bool isEnCloseMode = false;
        bool isExtrudeMode = false;

        std::vector<Handle(AIS_Shape)> persistedExtrusions;

        std::vector<Handle(AIS_Shape)> currentLoopEdges;
        std::vector<TopoDS_Edge> selectedEdges;

        bool isExtrudingActive = false;
        gp_Pnt extrudeBasePoint;
        Handle(AIS_Shape) extrudePreviewShape;
        TopoDS_Wire activeWire;
        double currentExtrudeHeight = 0.0;

        std::vector<Handle(AIS_InteractiveObject)> selectedShapes;

        bool isBooleanUnionMode = false;
        bool isBooleanCutMode = false;
        bool isBooleanIntersectMode = false;
        bool isBooleanMode = false;

        bool isMoveMode = false;
        bool isMoving = false;
        gp_Pnt lastMousePoint;
        Handle(AIS_InteractiveObject) movingObject;

        bool isRotateMode = false;
        bool isRotating = false;
        int lastMouseX = 0;
        int lastMouseY = 0;
        Handle(AIS_InteractiveObject) rotatingObject;

        bool isMateAlignmentMode = false;
        bool hasFirstMateSelected;

        Handle(AIS_Shape) firstMateShape;
        TopoDS_Face* firstMateFace;

        bool isRevolveMode = false;
        Handle(AIS_Shape) revolvePreviewShape;
        gp_Ax1 revolveAxis;
        bool isRevolveAxisSet = false;
        TopoDS_Wire revolveWire;
        double currentRevolveAngle = 360.0;
        Handle(AIS_Shape) revolveAxisObj;
        Handle(AIS_Shape) revolveAxisShape;
        bool isCenterLineSet = true;

        bool isDrawDimensionMode = false;
        bool isPickingDimensionFirstPoint = true;
        bool isPlacingDimension = false;
        gp_Pnt dimPoint1;
        gp_Pnt dimPoint2;
        gp_Pnt dimPosition;
        Handle(AIS_Shape) dimExtLine1Temp;
        Handle(AIS_Shape) dimExtLine2Temp;
        Handle(AIS_Shape) dimLineTemp;
        Handle(AIS_Shape) dimLabelTemp;
        Handle(AIS_Shape) dimArrow1Temp;
        Handle(AIS_Shape) dimArrow2Temp;

        bool isZoomWindowMode = false;
        Handle(AIS_Shape) highlightedFace;
        Handle(AIS_Shape) hoverHighlightedFace;


        NativeViewerHandle()
        {
            hasFirstMateSelected = false;
            firstMateFace = nullptr;
            isRevolveMode = false;
            currentRevolveAngle = 360.0;
        }
    };
}