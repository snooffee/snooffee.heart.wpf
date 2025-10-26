#include <AIS_InteractiveObject.hxx>
#include <Prs3d_Presentation.hxx>
#include <PrsMgr_PresentationManager3d.hxx>
#include <Graphic3d_ArrayOfSegments.hxx>
#include <Graphic3d_Group.hxx>
#include <Graphic3d_AspectLine3d.hxx>
#include <Quantity_Color.hxx>
#include <Graphic3d_TransformPers.hxx>
#include <Graphic3d_ZLayerId.hxx>
#include <Prs3d_Root.hxx>
#include <SelectMgr_Selection.hxx>
#include <SelectMgr_EntityOwner.hxx>
#include <Select3D_SensitiveSegment.hxx>
#include <gp_Pnt2d.hxx>
#include <gp_Pnt.hxx>

class AIS_OverlayLine : public AIS_InteractiveObject
{
public:
    AIS_OverlayLine()
        : myX1(0), myY1(0), myX2(0), myY2(0)
    {
        this->SetZLayer(Graphic3d_ZLayerId_TopOSD);
        Handle(Graphic3d_TransformPers) trpers = new Graphic3d_TransformPers(Graphic3d_TMF_2d);
        this->SetTransformPersistence(trpers);
    }

    void SetEndpoints(int x1, int y1, int x2, int y2)
    {
        myX1 = x1;
        myY1 = y1;
        myX2 = x2;
        myY2 = y2;
        this->Redisplay(Standard_True);
    }

    virtual void Compute(
        const Handle(PrsMgr_PresentationManager3d)& thePM,
        const Handle(Prs3d_Presentation)& thePresentation,
        const Standard_Integer theMode) override
    {
        Handle(Graphic3d_Group) aGroup = Prs3d_Root::CurrentGroup(thePresentation);

        Handle(Graphic3d_ArrayOfSegments) segs = new Graphic3d_ArrayOfSegments(2);
        segs->AddVertex(Standard_ShortReal(myX1), Standard_ShortReal(myY1), 0.0f);
        segs->AddVertex(Standard_ShortReal(myX2), Standard_ShortReal(myY2), 0.0f);

        Quantity_Color colLine(Quantity_NOC_RED);
        Handle(Graphic3d_AspectLine3d) asp = new Graphic3d_AspectLine3d(colLine, Aspect_TOL_SOLID, 1.0f);
        aGroup->SetPrimitivesAspect(asp);

        aGroup->AddPrimitiveArray(segs);
    }


    virtual void ComputeSelection(const Handle(SelectMgr_Selection)& theSelection,
        const Standard_Integer theMode) override
    {
        gp_Pnt p1(Standard_Real(myX1), Standard_Real(myY1), 0.0);
        gp_Pnt p2(Standard_Real(myX2), Standard_Real(myY2), 0.0);

        Handle(SelectMgr_EntityOwner) owner = new SelectMgr_EntityOwner(this);
        Handle(Select3D_SensitiveSegment) sensitiveSegment =
            new Select3D_SensitiveSegment(owner, p1, p2);

        theSelection->Add(sensitiveSegment);
    }


private:
    int myX1, myY1, myX2, myY2;
};