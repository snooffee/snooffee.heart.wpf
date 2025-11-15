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
#include <SelectMgr_EntityOwner.hxx>
#include <Select3D_SensitiveSegment.hxx>
#include <gp_Pnt.hxx>
#include <gp_Dir.hxx>
#include <algorithm>

class AIS_OverlayRectangle : public AIS_InteractiveObject
{
public:
    AIS_OverlayRectangle()
        : myP1(0, 0, 0), myP2(0, 0, 0), myP3(0, 0, 0), myP4(0, 0, 0)
    {
        this->SetZLayer(Graphic3d_ZLayerId_TopOSD);
        Handle(Graphic3d_TransformPers) trpers = new Graphic3d_TransformPers(Graphic3d_TMF_2d);
        this->SetTransformPersistence(trpers);
    }

    // --- Set rectangle using 2 points (XY fallback) ---
    void SetRectangle(const gp_Pnt& p1, const gp_Pnt& p2)
    {
        myP1 = p1;
        myP2 = gp_Pnt(p2.X(), p1.Y(), p1.Z());
        myP3 = p2;
        myP4 = gp_Pnt(p1.X(), p2.Y(), p2.Z());
        this->Redisplay(Standard_True);
    }

    // --- Set rectangle using 4 points (plane mode) ---
    void SetRectangle(const gp_Pnt& p1, const gp_Pnt& p2, const gp_Pnt& p3, const gp_Pnt& p4)
    {
        myP1 = p1;
        myP2 = p2;
        myP3 = p3;
        myP4 = p4;
        this->Redisplay(Standard_True);
    }

    // --- Draw rectangle ---
    virtual void Compute(
        const Handle(PrsMgr_PresentationManager3d)& /*thePM*/,
        const Handle(Prs3d_Presentation)& thePresentation,
        const Standard_Integer /*theMode*/) override
    {
        Handle(Graphic3d_Group) aGroup = Prs3d_Root::CurrentGroup(thePresentation);

        Handle(Graphic3d_ArrayOfSegments) segs = new Graphic3d_ArrayOfSegments(8);

        segs->AddVertex(Standard_ShortReal(myP1.X()), Standard_ShortReal(myP1.Y()), Standard_ShortReal(myP1.Z()));
        segs->AddVertex(Standard_ShortReal(myP2.X()), Standard_ShortReal(myP2.Y()), Standard_ShortReal(myP2.Z()));

        segs->AddVertex(Standard_ShortReal(myP2.X()), Standard_ShortReal(myP2.Y()), Standard_ShortReal(myP2.Z()));
        segs->AddVertex(Standard_ShortReal(myP3.X()), Standard_ShortReal(myP3.Y()), Standard_ShortReal(myP3.Z()));

        segs->AddVertex(Standard_ShortReal(myP3.X()), Standard_ShortReal(myP3.Y()), Standard_ShortReal(myP3.Z()));
        segs->AddVertex(Standard_ShortReal(myP4.X()), Standard_ShortReal(myP4.Y()), Standard_ShortReal(myP4.Z()));

        segs->AddVertex(Standard_ShortReal(myP4.X()), Standard_ShortReal(myP4.Y()), Standard_ShortReal(myP4.Z()));
        segs->AddVertex(Standard_ShortReal(myP1.X()), Standard_ShortReal(myP1.Y()), Standard_ShortReal(myP1.Z()));

        Quantity_Color colLine(Quantity_NOC_RED);
        Handle(Graphic3d_AspectLine3d) asp = new Graphic3d_AspectLine3d(colLine, Aspect_TOL_SOLID, 1.0f);
        aGroup->SetPrimitivesAspect(asp);
        aGroup->AddPrimitiveArray(segs);
    }

    // --- Define selection for rectangle edges ---
    virtual void ComputeSelection(
        const Handle(SelectMgr_Selection)& theSelection,
        const Standard_Integer /*theMode*/) override
    {
        Handle(SelectMgr_EntityOwner) owner = new SelectMgr_EntityOwner(this);

        theSelection->Add(new Select3D_SensitiveSegment(owner, myP1, myP2));
        theSelection->Add(new Select3D_SensitiveSegment(owner, myP2, myP3));
        theSelection->Add(new Select3D_SensitiveSegment(owner, myP3, myP4));
        theSelection->Add(new Select3D_SensitiveSegment(owner, myP4, myP1));
    }

private:
    gp_Pnt myP1, myP2, myP3, myP4;
};