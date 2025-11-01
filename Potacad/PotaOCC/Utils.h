#pragma once

// Precompiled header (if your project uses it)
#include "pch.h"

// Standard headers
#include <string>
#include <unordered_map>

// OCCT headers
#include <gp_Pnt.hxx>
#include <TopoDS_Shape.hxx>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <Aspect_TypeOfLine.hxx>

// Utils namespace
namespace PotaOCC {

    inline Aspect_TypeOfLine GetLineStyle(const std::string& edgeTypeStr)
    {
        static const std::unordered_map<std::string, Aspect_TypeOfLine> styleMap = {
            { "実線",    Aspect_TOL_SOLID },
            { "点線1",   Aspect_TOL_DOT },
            { "点線2",   Aspect_TOL_DASH },
            { "点線3",   Aspect_TOL_DASH },
            { "一点鎖1", Aspect_TOL_DOTDASH },
            { "一点鎖2", Aspect_TOL_DOTDASH },
            { "二点鎖1", Aspect_TOL_DOTDASH },
            { "二点鎖2", Aspect_TOL_DOTDASH },
            { "補助線",  Aspect_TOL_DOT }
        };

        if (edgeTypeStr.rfind("ランダム線", 0) == 0)
            return Aspect_TOL_SOLID;

        auto it = styleMap.find(edgeTypeStr);
        if (it != styleMap.end())
            return it->second;

        return Aspect_TOL_SOLID;
    }

    inline gp_Pnt GetShapeCenter(const TopoDS_Shape& shape)
    {
        Bnd_Box bnd;
        BRepBndLib::Add(shape, bnd);
        Standard_Real xmin, ymin, zmin, xmax, ymax, zmax;
        bnd.Get(xmin, ymin, zmin, xmax, ymax, zmax);
        return gp_Pnt((xmin + xmax) / 2.0,
            (ymin + ymax) / 2.0,
            (zmin + zmax) / 2.0);
    }

} // namespace PotatoOCC