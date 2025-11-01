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
#include <Select3D_SensitiveBox.hxx>
#include <gp_Pnt.hxx>
#include <gp_Dir.hxx>
#include <Bnd_Box.hxx>
#include <algorithm> // for std::min, std::max
#include <Select3D_SensitiveSegment.hxx>

class AIS_OverlayRectangle : public AIS_InteractiveObject
{
public:
    AIS_OverlayRectangle()
        : myP1(0, 0, 0), myP2(0, 0, 0)
    {
        this->SetZLayer(Graphic3d_ZLayerId_TopOSD);
        Handle(Graphic3d_TransformPers) trpers = new Graphic3d_TransformPers(Graphic3d_TMF_2d);
        this->SetTransformPersistence(trpers);
    }

    void SetRectangle(const gp_Pnt& p1, const gp_Pnt& p2)
    {
        myP1 = p1;
        myP2 = p2;
        this->Redisplay(Standard_True);
    }

    virtual void Compute(const Handle(PrsMgr_PresentationManager3d)& thePM,
        const Handle(Prs3d_Presentation)& thePresentation,
        const Standard_Integer theMode) override
    {
        Handle(Graphic3d_Group) aGroup = Prs3d_Root::CurrentGroup(thePresentation);

        // Compute corner points
        double x1 = myP1.X();
        double y1 = myP1.Y();
        double x2 = myP2.X();
        double y2 = myP2.Y();
        double z = myP1.Z();

        gp_Pnt pA(x1, y1, z);
        gp_Pnt pB(x2, y1, z);
        gp_Pnt pC(x2, y2, z);
        gp_Pnt pD(x1, y2, z);

        // Create line segments around the rectangle
        Handle(Graphic3d_ArrayOfSegments) segs = new Graphic3d_ArrayOfSegments(8);
        segs->AddVertex(Standard_ShortReal(pA.X()), Standard_ShortReal(pA.Y()), Standard_ShortReal(z));
        segs->AddVertex(Standard_ShortReal(pB.X()), Standard_ShortReal(pB.Y()), Standard_ShortReal(z));

        segs->AddVertex(Standard_ShortReal(pB.X()), Standard_ShortReal(pB.Y()), Standard_ShortReal(z));
        segs->AddVertex(Standard_ShortReal(pC.X()), Standard_ShortReal(pC.Y()), Standard_ShortReal(z));

        segs->AddVertex(Standard_ShortReal(pC.X()), Standard_ShortReal(pC.Y()), Standard_ShortReal(z));
        segs->AddVertex(Standard_ShortReal(pD.X()), Standard_ShortReal(pD.Y()), Standard_ShortReal(z));

        segs->AddVertex(Standard_ShortReal(pD.X()), Standard_ShortReal(pD.Y()), Standard_ShortReal(z));
        segs->AddVertex(Standard_ShortReal(pA.X()), Standard_ShortReal(pA.Y()), Standard_ShortReal(z));

        // Draw with red border
        Quantity_Color colLine(Quantity_NOC_RED);
        Handle(Graphic3d_AspectLine3d) asp = new Graphic3d_AspectLine3d(colLine, Aspect_TOL_SOLID, 1.0f);
        aGroup->SetPrimitivesAspect(asp);
        aGroup->AddPrimitiveArray(segs);
    }

    virtual void ComputeSelection(const Handle(SelectMgr_Selection)& theSelection,
        const Standard_Integer theMode) override
    {
        Handle(SelectMgr_EntityOwner) owner = new SelectMgr_EntityOwner(this);

        // Define 2D rectangle corners
        gp_Pnt pA(myP1.X(), myP1.Y(), myP1.Z());
        gp_Pnt pB(myP2.X(), myP1.Y(), myP1.Z());
        gp_Pnt pC(myP2.X(), myP2.Y(), myP1.Z());
        gp_Pnt pD(myP1.X(), myP2.Y(), myP1.Z());

        // Add 4 sensitive segments representing the rectanglefs border
        theSelection->Add(new Select3D_SensitiveSegment(owner, pA, pB));
        theSelection->Add(new Select3D_SensitiveSegment(owner, pB, pC));
        theSelection->Add(new Select3D_SensitiveSegment(owner, pC, pD));
        theSelection->Add(new Select3D_SensitiveSegment(owner, pD, pA));
    }


private:
    gp_Pnt myP1;
    gp_Pnt myP2;
};