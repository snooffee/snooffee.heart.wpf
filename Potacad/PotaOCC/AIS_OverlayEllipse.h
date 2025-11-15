#pragma once
#include <AIS_InteractiveObject.hxx>
#include <Prs3d_Presentation.hxx>
#include <PrsMgr_PresentationManager3d.hxx>
#include <Graphic3d_ArrayOfSegments.hxx>
#include <Graphic3d_Group.hxx>
#include <Graphic3d_AspectLine3d.hxx>
#include <Quantity_Color.hxx>
#include <Graphic3d_ZLayerId.hxx>
#include <Prs3d_Root.hxx>
#include <SelectMgr_Selection.hxx>
#include <gp_Pnt.hxx>
#include <gp_Dir.hxx>
#include <gp_Vec.hxx>
#include <cmath>

class AIS_OverlayEllipse : public AIS_InteractiveObject
{
public:
    AIS_OverlayEllipse()
        : myCenter(0, 0, 0), myRadiusX(0.0), myRadiusY(0.0), isPlaneMode(false)
    {
        // Put this in top overlay layer (always visible)
        this->SetZLayer(Graphic3d_ZLayerId_TopOSD);
        Handle(Graphic3d_TransformPers) trpers = new Graphic3d_TransformPers(Graphic3d_TMF_2d);
        this->SetTransformPersistence(trpers);
    }

    // --- For 2D overlay ellipse (screen plane) ---
    void SetEllipse(const gp_Pnt& center, double radiusX, double radiusY)
    {
        myCenter = center;
        myRadiusX = radiusX;
        myRadiusY = radiusY;
        isPlaneMode = false;
        this->Redisplay(Standard_True);
    }

    // --- For ellipse on 3D plane (like circle plane mode) ---
    void SetEllipseOnPlane(const gp_Pnt& center, const gp_Dir& uDir, const gp_Dir& vDir,
        double radiusX, double radiusY)
    {
        myCenter = center;
        myU = uDir;
        myV = vDir;
        myRadiusX = radiusX;
        myRadiusY = radiusY;
        isPlaneMode = true;

        // ✅ Important: Disable screen-space transform persistence for 3D plane ellipse
        this->SetTransformPersistence(nullptr);

        this->Redisplay(Standard_True);
    }

protected:
    void Compute(const Handle(PrsMgr_PresentationManager3d)&,
        const Handle(Prs3d_Presentation)& thePresentation,
        const Standard_Integer) override
    {
        Handle(Graphic3d_Group) aGroup = Prs3d_Root::CurrentGroup(thePresentation);
        const int numSegments = 64;
        Handle(Graphic3d_ArrayOfSegments) segs = new Graphic3d_ArrayOfSegments(2 * numSegments);

        for (int i = 0; i < numSegments; ++i)
        {
            double ang1 = (2.0 * M_PI * i) / numSegments;
            double ang2 = (2.0 * M_PI * (i + 1)) / numSegments;

            gp_Pnt p1, p2;

            if (isPlaneMode)
            {
                // Compute ellipse in 3D plane using local u,v directions
                gp_Vec v1 = gp_Vec(myU) * (myRadiusX * cos(ang1)) + gp_Vec(myV) * (myRadiusY * sin(ang1));
                gp_Vec v2 = gp_Vec(myU) * (myRadiusX * cos(ang2)) + gp_Vec(myV) * (myRadiusY * sin(ang2));
                p1 = myCenter.Translated(v1);
                p2 = myCenter.Translated(v2);
            }
            else
            {
                // Compute ellipse in XY plane (overlay mode)
                double x1 = myCenter.X() + myRadiusX * cos(ang1);
                double y1 = myCenter.Y() + myRadiusY * sin(ang1);
                double x2 = myCenter.X() + myRadiusX * cos(ang2);
                double y2 = myCenter.Y() + myRadiusY * sin(ang2);
                p1 = gp_Pnt(x1, y1, myCenter.Z());
                p2 = gp_Pnt(x2, y2, myCenter.Z());
            }

            segs->AddVertex(Standard_ShortReal(p1.X()), Standard_ShortReal(p1.Y()), Standard_ShortReal(p1.Z()));
            segs->AddVertex(Standard_ShortReal(p2.X()), Standard_ShortReal(p2.Y()), Standard_ShortReal(p2.Z()));
        }

        // Green ellipse line
        Quantity_Color colLine(Quantity_NOC_GREEN);
        Handle(Graphic3d_AspectLine3d) asp = new Graphic3d_AspectLine3d(colLine, Aspect_TOL_SOLID, 1.0f);
        aGroup->SetPrimitivesAspect(asp);
        aGroup->AddPrimitiveArray(segs);
    }

    void ComputeSelection(const Handle(SelectMgr_Selection)&,
        const Standard_Integer) override
    {
        // No selection for overlays
    }

private:
    gp_Pnt myCenter;
    gp_Dir myU, myV;
    double myRadiusX, myRadiusY;
    bool isPlaneMode;
};