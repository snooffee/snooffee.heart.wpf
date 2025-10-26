#pragma once
#include <vcclr.h>
#include <TopoDS.hxx>
#include <AIS_InteractiveObject.hxx>  // For Handle(AIS_InteractiveObject)
#include <AIS_InteractiveContext.hxx>  // For Handle(AIS_InteractiveContext)
#include <gp_Pnt.hxx>  // For gp_Pnt
#include <Geom_Curve.hxx>  // For Geom_Curve type (as parameter)

using namespace System;

namespace PotatoOCC {

    public ref class ShapeDrawer
    {
    public:
        // ---------------- 3D Box ----------------
        static void DrawBox(
            IntPtr viewerHandlePtr,
            double dx, double dy, double dz,
            int r, int g, int b,
            double transparency,
            double edgeWidth,
            int er, int eg, int eb, System::String^ edgeType);

        static void RotateBoxAroundCenter(IntPtr viewerHandlePtr, double angleX, double angleY);
        static void SetWireframe(IntPtr viewerHandlePtr);
        static void SetShaded(IntPtr viewerHandlePtr);
        // ------------------ Clear ------------------
        static void ClearAllShapes(IntPtr viewerHandlePtr);

        // ------------------ 2D Box & Label ------------------
        static void Draw2DBoxWithLabel(
            IntPtr viewerHandlePtr,
            double x, double y, double width, double height,
            int fr, int fg, int fb, double fillAlpha,   // <-- fixed here
            double edgeWidth, int er, int eg, int eb, double edgeAlpha,
            System::String^ labelText, int lr, int lg, int lb, double textAlpha,
            double fontHeight, System::String^ fontFamily, System::String^ edgeType);

        static void ActivateFaceSelection(System::IntPtr viewerHandlePtr);
        static void AlignViewToSelectedFace(System::IntPtr viewerHandlePtr);

        static void UpdateSelection(IntPtr viewerHandlePtr, int x, int y);
        static void UpdateSelection(IntPtr viewerHandlePtr, int x, int y, bool isClick);

        static void ResetView(IntPtr viewerHandlePtr);

        static gp_Pnt ScreenToWorld(Handle(V3d_View) view, double screenX, double screenY);
        static TopoDS_Edge CreateEdgeSafe(const gp_Pnt& p1, const gp_Pnt& p2);
        static TopoDS_Edge CreateCircleSafe(const gp_Circ& circle);
        static void SetupOverlay(Handle(AIS_InteractiveObject)& obj, const Quantity_NameOfColor color, double width);

        static void eraseLineByDetectedIO(Handle(AIS_InteractiveObject) detectedIO, std::vector<Handle(AIS_Shape)>& persistedLines);

        static bool findIntersectionPoints(const gp_Pnt& intersectionPoint, const Handle(Geom_Curve)& curve,
            Standard_Real f, Standard_Real l, Handle(AIS_InteractiveObject) detectedIO,
            Handle(AIS_InteractiveContext) context, std::vector<Handle(AIS_Shape)>& persistedLines);

        static bool IsClosedWire(const TopoDS_Wire& wire);

        static TopoDS_Wire BuildWireFromSelection(const AIS_ListOfInteractive& picked);
        static Handle(AIS_InteractiveObject) BuildCompoundFromSelection(const AIS_ListOfInteractive& picked);

        static void HighlightSelectedObjects(Handle(AIS_InteractiveContext) context, AIS_ListOfInteractive& picked);

        // If you want to pass persistedLines to checkAndTrimEdge
        static bool checkAndTrimEdge(
            const TopoDS_Edge& selectedEdge,
            const TopoDS_Edge& otherEdge,
            const Handle(Geom_Curve)& curve,
            Standard_Real f, Standard_Real l,
            Handle(AIS_InteractiveObject) detectedIO,
            bool& trimmed,
            Handle(AIS_InteractiveContext) context,
            std::vector<Handle(AIS_Shape)>& persistedLines);
    };
}