#pragma once
#include <vcclr.h>
#include <TopTools_HSequenceOfShape.hxx>
#include <NCollection_List.hxx>
#include <NCollection_TListIterator.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRep_Tool.hxx>
#include <TopExp.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Wire.hxx>
#include <AIS_Shape.hxx>

using namespace System;
namespace PotaOCC {
    public enum class MouseAction
    {
        None = 0,
        Rotate = 1,
        Pan = 2,
        Zoom = 3,
        Select = 4
    };
    public ref class MouseControlSettings
    {
    public:
        MouseAction LeftButtonAction;
        MouseAction MiddleButtonAction;
        MouseAction RightButtonAction;
        MouseAction WheelScrollAction;
        MouseControlSettings()
        {
            LeftButtonAction = MouseAction::Select;
            MiddleButtonAction = MouseAction::Pan;
            RightButtonAction = MouseAction::Rotate;
            WheelScrollAction = MouseAction::Zoom;
        }
    };
    public ref class MouseHandler
    {
    public:
        enum class MouseMode
        {
            Default,
            Move,
            Rotate,
            MateAlignment
        };
        static MouseMode GetCurrentMode();
        static void StartRotation(IntPtr viewPtr, int x, int y);
        static void Rotate(IntPtr viewPtr, int x, int y);
        static void Pan(IntPtr viewPtr, int dx, int dy);
        static void Zoom(IntPtr viewPtr, double factor);
        static void ZoomAt(IntPtr viewPtr, int x, int y, double factor);
        static void SetMouseControlSettings(IntPtr viewerHandlePtr, MouseControlSettings^ settings);
        static void OnMouseDown(IntPtr viewerHandlePtr, int x, int y, bool multipleselect);
        static void OnMouseMove(IntPtr viewerHandlePtr, int x, int y, int h);
        static void OnMouseUp(IntPtr viewerHandlePtr, int x, int y, int h, int w);
        static void ShowAxisTriad(IntPtr viewerHandlePtr);
        static void HideAxisTriad(IntPtr viewerHandlePtr);
        static void ResetViewToAxis(Handle(V3d_View) view, const std::string& axis);
        static bool CheckRayIntersectionWithAxis(const gp_Pnt& origin, const gp_Dir& direction, const std::string& axis);
        static void ResetView(IntPtr viewerHandlePtr, int x, int y);
        static void OnKeyDown(IntPtr viewerHandlePtr, char key);
        static void OnKeyUp(IntPtr viewerHandlePtr, char key);
        static void HandleMouseDownAction(Handle(AIS_InteractiveContext) context, Handle(V3d_View) view, int x, int y, bool multipleSelect);
        static void HandleRevolveMode(Handle(AIS_InteractiveContext) context, Handle(V3d_View) view);
        static TopoDS_Wire PickWireAtCursor(const Handle(AIS_InteractiveContext)& context, const Handle(V3d_View)& view, int x, int y);
        static TopoDS_Edge PickEdgeAtCursor(const Handle(AIS_InteractiveContext)& context, const Handle(V3d_View)& view, int x, int y);
        static TopoDS_Shape PickShapeAtCursor(const Handle(AIS_InteractiveContext)& context, const Handle(V3d_View)& view, int x, int y);
    private:
        static bool AreEdgesSame(const TopoDS_Edge& e1, const TopoDS_Edge& e2)
        {
            gp_Pnt p1s = BRep_Tool::Pnt(TopExp::FirstVertex(e1));
            gp_Pnt p1e = BRep_Tool::Pnt(TopExp::LastVertex(e1));
            gp_Pnt p2s = BRep_Tool::Pnt(TopExp::FirstVertex(e2));
            gp_Pnt p2e = BRep_Tool::Pnt(TopExp::LastVertex(e2));
            return (p1s.Distance(p2s) < 1e-6 && p1e.Distance(p2e) < 1e-6) ||
                (p1s.Distance(p2e) < 1e-6 && p1e.Distance(p2s) < 1e-6);
        }
    };
}