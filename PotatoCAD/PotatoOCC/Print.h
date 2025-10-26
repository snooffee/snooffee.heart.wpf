#pragma once
#include <vcclr.h>
#include <TopoDS.hxx>

using namespace System;

namespace PotatoOCC {

    public ref class Print
    {
    public:
        static void ShapeDetails(Handle(AIS_Shape) aisShape);
        static void EdgeDetails(TopoDS_Shape& detectedShape);
        static void VertexDetails(TopoDS_Shape& detectedShape);
        static void PrintWireDetails(TopoDS_Shape& detectedShape);
    };
}