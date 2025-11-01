#include "pch.h"
#include "NativeViewerHandle.h"
#include "ViewerManager.h"

#include <WNT_Window.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <V3d_Viewer.hxx>
#include <V3d_View.hxx>
#include <AIS_InteractiveContext.hxx>
#include <Quantity_Color.hxx>
#include <mutex>
#include <map>

#include <AIS_Shape.hxx>
#include <AIS_TextLabel.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Wire.hxx>
#include <TopoDS_Face.hxx>
#include <gp_Pnt.hxx>
#include <gp_Trsf.hxx>
#include <gp_Vec.hxx>
#include <gp_Dir.hxx>
#include <Prs3d_Drawer.hxx>
#include <Prs3d_LineAspect.hxx>
#include <vcclr.h>
#include <vector>
#include <TCollection_ExtendedString.hxx>

#include <TopoDS_Shape.hxx>
#include <BRepTools.hxx>
#include <Standard_Type.hxx>
#include <gcroot.h>

#include <unordered_map>
#include <string>

#include <Graphic3d_Camera.hxx>


using namespace PotaOCC;

// Thread-safe map to keep track of NativeViewerHandle per viewer (optional, for future use)
static std::map<NativeViewerHandle*, Handle(V3d_View)> g_viewMap;
static std::mutex g_viewMapMutex;

ViewerHandle^ ViewerManager::CreateViewer(IntPtr hwnd, int width, int height, Control^ hostControl)
{
    Handle(Aspect_DisplayConnection) displayConnection = new Aspect_DisplayConnection();
    Handle(OpenGl_GraphicDriver) driver = new OpenGl_GraphicDriver(displayConnection);
    Handle(V3d_Viewer) viewer = new V3d_Viewer(driver);
    viewer->SetDefaultLights();
    viewer->SetLightOn();

    Handle(V3d_View) view = viewer->CreateView();
    Handle(WNT_Window) window = new WNT_Window((HWND)hwnd.ToPointer());
    view->SetWindow(window);
    if (!window->IsMapped()) window->Map();

    Handle(AIS_InteractiveContext) context = new AIS_InteractiveContext(viewer);
    view->SetBackgroundColor(Quantity_NOC_BLACK);

    viewer->SetDefaultLights();
    viewer->SetLightOn();
    //viewer->ActivateGrid(Aspect_GT_Rectangular, Aspect_GDM_Lines); // Optional

    view->TriedronDisplay(Aspect_TOTP_RIGHT_UPPER, Quantity_NOC_WHITE, 0.08, V3d_ZBUFFER);
    //view->SetProj(V3d_Zpos);
    view->FitAll();

    view->MustBeResized();
    view->Redraw();

    NativeViewerHandle* native = new NativeViewerHandle();
    native->view = view;
    native->context = context;
    native->hostControl = hostControl; // ? This enables cursor updates


    ViewerHandle^ handle = gcnew ViewerHandle();
    handle->ViewPtr = IntPtr(view.get());
    handle->ContextPtr = IntPtr(context.get());
    handle->NativeHandle = IntPtr(native);

    return handle;
}

void ViewerManager::ResizeViewer(IntPtr viewerHandlePtr, int width, int height)
{
    if (viewerHandlePtr == IntPtr::Zero) return;
    NativeViewerHandle* native = static_cast<NativeViewerHandle*>(viewerHandlePtr.ToPointer());
    if (!native || native->view.IsNull()) return;

    Handle(V3d_View) view = native->view;
    if (view->Window()->IsMapped())
    {
        view->Window()->DoResize();
        view->MustBeResized();
        view->Redraw();
    }
}

void ViewerManager::FitAll(IntPtr viewPtr)
{
    Handle(V3d_View) view = static_cast<V3d_View*>(viewPtr.ToPointer());
    if (!view.IsNull()) { view->FitAll(); view->Redraw(); }
}

void ViewerManager::UpdateView(IntPtr viewerHandlePtr, bool isDisposing)
{
    if (isDisposing) return;
    if (viewerHandlePtr == IntPtr::Zero) return;

    NativeViewerHandle* native = static_cast<NativeViewerHandle*>(viewerHandlePtr.ToPointer());
    if (!native) return;
    if (native->context.IsNull() || native->view.IsNull()) return;

    native->context->UpdateCurrentViewer();
    native->view->Redraw();
}

void ViewerManager::SetBackgroundColor(IntPtr viewPtr, int r, int g, int b)
{
    Handle(V3d_View) view = static_cast<V3d_View*>(viewPtr.ToPointer());
    if (!view.IsNull())
    {
        view->SetBackgroundColor(Quantity_Color(r / 255.0, g / 255.0, b / 255.0, Quantity_TOC_RGB));
        view->Redraw();
    }
}

void ViewerManager::DisposeViewer(System::IntPtr viewerHandlePtr)
{
    if (viewerHandlePtr == IntPtr::Zero) return;

    NativeViewerHandle* native = reinterpret_cast<NativeViewerHandle*>(viewerHandlePtr.ToPointer());
    if (!native) return;

    if (!native->context.IsNull())
    {
        native->context->EraseAll(Standard_True);
        native->context.Nullify();
    }

    if (!native->view.IsNull())
    {
        native->view.Nullify();
    }

    delete native;  // free memory
}


// Display a shape in the OCCT viewer
void ViewerManager::Display(IntPtr nativeHandlePtr, Handle(AIS_InteractiveObject) shape, bool redraw)
{
    if (nativeHandlePtr == IntPtr::Zero || shape.IsNull())
        return;

    NativeViewerHandle* native = static_cast<NativeViewerHandle*>(nativeHandlePtr.ToPointer());
    if (native == nullptr || native->context.IsNull())
        return;

    native->context->Display(shape, redraw);
}
// Remove a shape from the OCCT viewer
void ViewerManager::Remove(IntPtr contextPtr, IntPtr shapePtr)
{
    if (contextPtr == IntPtr::Zero || shapePtr == IntPtr::Zero)
        return;

    Handle(AIS_InteractiveContext) ctx = Handle(AIS_InteractiveContext)::DownCast(
        reinterpret_cast<AIS_InteractiveContext*>(contextPtr.ToPointer()));

    Handle(AIS_InteractiveObject) shp = Handle(AIS_InteractiveObject)::DownCast(
        reinterpret_cast<AIS_InteractiveObject*>(shapePtr.ToPointer()));

    if (!ctx.IsNull() && !shp.IsNull())
    {
        ctx->Remove(shp, false);
    }
}

// Force redraw of the view
void ViewerManager::Redraw(IntPtr viewPtr)
{
    if (viewPtr == IntPtr::Zero)
        return;

    Handle(V3d_View) view = Handle(V3d_View)::DownCast(
        reinterpret_cast<V3d_View*>(viewPtr.ToPointer()));

    if (!view.IsNull())
    {
        view->Redraw();
    }
}

void ViewerManager::Convert(IntPtr viewPtr, int x, int y, double% X, double% Y, double% Z)
{
    Handle(V3d_View) view = static_cast<V3d_View*>(viewPtr.ToPointer());
    if (view.IsNull()) return;

    double xx, yy, zz;
    view->Convert(x, y, xx, yy, zz);

    X = xx;
    Y = yy;
    Z = zz;
}

// Projects a 3D point (wx,wy,wz) into view coordinates (pixels)
void ViewerManager::WorldToScreen(System::IntPtr viewPtr,
    double wx, double wy, double wz,
    double* sx, double* sy)
{
    if (viewPtr == System::IntPtr::Zero || sx == nullptr || sy == nullptr) return;

    Handle(V3d_View) view = reinterpret_cast<V3d_View*>(viewPtr.ToPointer());
    if (view.IsNull()) return;

    Standard_Real xPix = 0.0, yPix = 0.0;

#if OCC_VERSION_HEX >= 0x070600  // OCCT 7.6 or newer
    gp_Pnt p(wx, wy, wz);
    view->Convert(p, xPix, yPix);
#else                           // OCCT 7.5 or older
    view->Convert(wx, wy, wz, xPix, yPix);
#endif

    * sx = static_cast<double>(xPix);
    *sy = static_cast<double>(yPix);
}