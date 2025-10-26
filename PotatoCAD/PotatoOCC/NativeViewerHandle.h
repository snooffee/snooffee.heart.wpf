#pragma once
#include <AIS_InteractiveObject.hxx>
#include <AIS_InteractiveContext.hxx>
#include <AIS_Shape.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Wire.hxx>
#include <AIS_TextLabel.hxx>
#include <gp_Pnt2d.hxx>
#include <vector>
#include <vcclr.h> // for gcroot to hold managed pointers
#include "AIS_OverlayLine.h"
#include "AIS_OverlayCircle.h"
#include <BRepLib_MakeFace.hxx>
#using <System.Windows.Forms.dll>
using namespace System::Windows::Forms;

namespace PotatoOCC {

    // 🔧 NativeViewerHandle holds all state and OpenCascade handles for the 3D viewer
    struct NativeViewerHandle {
        // 👁️ View and context handles for 3D visualization and interaction
        Handle(V3d_View) view;                           // 3D View (camera)
        Handle(AIS_InteractiveContext) context;         // AIS context for managing displayed interactive objects

        // 📦 Common 3D objects used in the viewer
        Handle(AIS_Shape) box3D;                         // 3D bounding box shape

        Handle(AIS_Shape) axisX;                         // X-axis visual indicator
        Handle(AIS_Shape) axisY;                         // Y-axis visual indicator
        Handle(AIS_Shape) axisZ;                         // Z-axis visual indicator

        // 🎨 Overlays for drawing 2D/3D auxiliary graphics (lines, circles, markers)
        Handle(AIS_OverlayLine) lineOverlay;             // Overlay to draw temporary lines
        Handle(AIS_OverlayCircle) circleOverlay;         // Overlay to draw temporary circles
        Handle(AIS_InteractiveObject) centerMarker;      // Marker at center point
        Handle(AIS_InteractiveObject) centerOverlay;     // Additional center overlay (e.g., highlight)

        // 📏 Collections for 2D shapes and labels
        std::vector<double> pixelHeights;                // Stores pixel heights for text or annotations
        std::vector<Handle(AIS_Shape)> ais2DShapes;      // List of AIS shapes representing 2D entities
        std::vector<Handle(AIS_TextLabel)> aisLabels;    // Text labels displayed in the viewer

        // 🔲 Selection state tracking
        bool isSelecting = false;                         // True if user is currently selecting an area
        gp_Pnt2d selectStart;                             // Start coordinate of selection rectangle
        gp_Pnt2d selectEnd;                               // End coordinate of selection rectangle

        bool textHeightCallbackSet = false;               // Flag to indicate if text height update callback is set

        // 🖱️ Host control for mouse cursor updates (Windows Forms)
        gcroot<System::Windows::Forms::Control^> hostControl;

        // 🖱️ Mouse button action mappings (customizable)
        int leftButtonAction = 0;
        int middleButtonAction = 0;
        int rightButtonAction = 0;
        int wheelScrollAction = 0;

        // 🖱️ Dragging state variables for mouse operations like rubber banding
        bool isDragging = false;
        int dragStartX = 0;
        int dragStartY = 0;
        int dragEndX = 0;
        int dragEndY = 0;

        // 🖍️ Rubber band overlay used for drawing selection rectangle or shapes dynamically
        Handle(Graphic3d_Structure) rubberBandOverlay;
        Handle(Graphic3d_Group) rubberBandGroup;

        // ✏️ Modes for drawing geometric primitives
        bool isLineMode = false;                          // Line drawing mode
        std::vector<Handle(AIS_Shape)> persistedLines;   // Stored line shapes that persist in the scene

        bool isCircleMode = false;                        // Circle drawing mode
        std::vector<Handle(AIS_Shape)> persistedCircles; // Stored circle shapes that persist in the scene

        // ✂️ Boolean operation modes & flags
        bool isTrimMode = false;                          // Trim operation active
        bool isRadiusMode = false;                        // Radius modification mode active
        bool isEnCloseMode = false;                        // Enclosure mode active
        bool isExtrudeMode = false;                        // Extrude mode active

        std::vector<Handle(AIS_Shape)> persistedExtrusions;  // Stored extruded shapes

        // 🔄 Loop and edge selections for advanced modeling workflows
        std::vector<Handle(AIS_Shape)> currentLoopEdges;     // Current loop edges selected
        std::vector<TopoDS_Edge> selectedEdges;              // Selected TopoDS edges from model

        // ⬆️ Extrusion operation state
        bool isExtrudingActive = false;                       // True if extruding operation is ongoing
        gp_Pnt extrudeBasePoint;                              // Base point for extrusion start
        Handle(AIS_Shape) extrudePreviewShape;                // Preview shape shown during extrusion
        TopoDS_Wire activeWire;                                // Wire actively being extruded
        double currentExtrudeHeight = 0.0;                     // Current extrusion height

        // ✅ Selection state for interactive objects
        std::vector<Handle(AIS_InteractiveObject)> selectedShapes;  // List of currently selected shapes

        // 🧮 Boolean operations mode flags
        bool isBooleanUnionMode = false;
        bool isBooleanCutMode = false;
        bool isBooleanIntersectMode = false;
        bool isBooleanMode = false;                         // General boolean mode flag

        // 🧲 Moving state - related to moving a selected shape by mouse drag
        bool isMoveMode = false;                            // Move mode activated
        bool isMoving = false;                              // Currently dragging/moving a shape
        gp_Pnt lastMousePoint;                              // Last 3D point under mouse cursor during move
        Handle(AIS_InteractiveObject) movingObject;         // The AIS object currently being moved

        // 🌀 Rotation state - related to rotating a selected shape by mouse drag
        bool isRotateMode = false;                          // Rotation mode activated
        bool isRotating = false;                            // Currently performing rotation drag
        int lastMouseX = 0;                                 // Last X pixel coordinate of mouse
        int lastMouseY = 0;                                 // Last Y pixel coordinate of mouse
        Handle(AIS_InteractiveObject) rotatingObject;       // The AIS object currently being rotated

        bool isMateAlignmentMode = false;
        bool hasFirstMateSelected;

        Handle(AIS_Shape) firstMateShape;
        TopoDS_Face* firstMateFace; // ✅ native pointer

        NativeViewerHandle()
        {
            hasFirstMateSelected = false;
            firstMateFace = nullptr;
        }
    };

} // namespace PotatoOCC