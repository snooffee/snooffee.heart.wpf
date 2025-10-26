#pragma managed(push, off)
#undef _WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <winnls.h>
#include <BRepMesh_IncrementalMesh.hxx>

void PerformUnmanagedMeshOperations(const TopoDS_Shape& shape)
{
    try
    {
        BRepMesh_IncrementalMesh(shape, 0.1);
    }
    catch (Standard_Failure& e)
    {
        std::cerr << "[TextDrawer] BRepMesh_IncrementalMesh failed: "
            << e.GetMessageString() << std::endl;
    }
}

#pragma managed(pop)

#include "TextDrawer.h"
#include "NativeViewerHandle.h"
#include <gp_Dir.hxx>
#include <TopoDS_Shape.hxx>
#include <AIS_Shape.hxx>
#include <V3d_Viewer.hxx>
#include <V3d_View.hxx>
#include <msclr/marshal.h>
#include <msclr/marshal_cppstd.h>
#include <NCollection_String.hxx>
#include <Font_BRepTextBuilder.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>

#include <BRepPrimAPI_MakePrism.hxx>

#include <Standard_Failure.hxx>
#include <iostream>

#include <fstream>
#include <string>

using namespace msclr::interop;
using namespace PotatoOCC;

bool IsUTF8Encoded(const std::string& text)
{
    // Check for BOM (Byte Order Mark) for UTF-8
    return text.size() >= 3 && (unsigned char)text[0] == 0xEF &&
        (unsigned char)text[1] == 0xBB && (unsigned char)text[2] == 0xBF;
}

std::wstring ConvertToUnicode(const std::string& input, bool isUtf8)
{
    if (isUtf8)
    {
        // Direct conversion from UTF-8 to wide string (UTF-16)
        int size = MultiByteToWideChar(CP_UTF8, 0, input.c_str(), -1, nullptr, 0);
        std::wstring output(size, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, input.c_str(), -1, &output[0], size);
        return output;
    }
    else
    {
        // For ANSI text, convert to UTF-16 (wide char)
        int size = MultiByteToWideChar(CP_ACP, 0, input.c_str(), -1, nullptr, 0);
        std::wstring output(size, L'\0');
        MultiByteToWideChar(CP_ACP, 0, input.c_str(), -1, &output[0], size);
        return output;
    }
}

void TextDrawer::DrawTextBatch(
    System::IntPtr viewerHandlePtr,
    array<double>^ x,
    array<double>^ y,
    array<double>^ z,
    array<System::String^>^ texts,
    array<double>^ modelHeights,
    array<double>^ rotations,
    array<double>^ widthFactors,
    array<int>^ r,
    array<int>^ g,
    array<int>^ b,
    array<double>^ transparency,
    double sceneScale)
{
    if (viewerHandlePtr == System::IntPtr::Zero) return;

    PotatoOCC::NativeViewerHandle* native =
        reinterpret_cast<PotatoOCC::NativeViewerHandle*>(viewerHandlePtr.ToPointer());
    if (!native || native->context.IsNull() || native->view.IsNull()) return;

    Handle(AIS_InteractiveContext) context = native->context;
    Handle(V3d_View) view = native->view;

    marshal_context ctx;
    int n = texts->Length;

    Font_BRepTextBuilder builder;

    for (int i = 0; i < n; ++i)
    {
        gp_Pnt basePnt(x[i] * sceneScale,
            y[i] * sceneScale,
            z[i] * sceneScale);

        System::String^ managed = texts[i] == nullptr ? System::String::Empty : texts[i];
        System::String^ normalized = NormalizeText(managed);

        std::wstring wstr = ctx.marshal_as<std::wstring>(normalized);

        int utf8len = ::WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (utf8len <= 0) continue;
        std::string utf8;
        utf8.resize(utf8len);
        ::WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &utf8[0], utf8len, nullptr, nullptr);
        if (!utf8.empty() && utf8.back() == '\0') utf8.pop_back();

        double fontSize = 1.0;
        if (modelHeights != nullptr && i < modelHeights->Length && modelHeights[i] > 0.0)
            fontSize = modelHeights[i] * sceneScale;

        // Set Font Properties
        Font_BRepFont font(NCollection_String("Yu Gothic UI"), Font_FA_Regular, fontSize);

        gp_Ax3 placement(basePnt, gp_Dir(0, 0, 1));
        NCollection_String occtText(utf8.c_str());

        Graphic3d_HorizontalTextAlignment hAlign = Graphic3d_HTA_LEFT;
        Graphic3d_VerticalTextAlignment   vAlign = Graphic3d_VTA_BOTTOM;

        TopoDS_Shape outlineShape;
        try {
            outlineShape = builder.Perform(font, occtText, placement, hAlign, vAlign);
        }
        catch (Standard_Failure& e) {
            std::cerr << "[TextDrawer] Font_BRepTextBuilder::Perform failed: "
                << e.GetMessageString() << std::endl;
            continue;
        }
        if (outlineShape.IsNull()) continue;

        // convert wires → faces to get filled text
        BRep_Builder compBuilder;
        TopoDS_Compound compound;
        compBuilder.MakeCompound(compound);

        //bool hasValidFaces = false;
        //for (TopExp_Explorer ex(outlineShape, TopAbs_WIRE); ex.More(); ex.Next())
        //{
        //    const TopoDS_Wire& w = TopoDS::Wire(ex.Current());
        //    BRepBuilderAPI_MakeFace mf(w, /* planar tolerance */ Precision::Confusion());

        //    if (mf.IsDone()) {
        //        compBuilder.Add(compound, mf.Face());
        //        hasValidFaces = true;
        //    }
        //}

        /*if (!hasValidFaces) {
            std::cerr << "[TextDrawer] No faces generated from text shape!" << std::endl;
            continue;
        }*/

        //TopoDS_Shape finalShape = compound.IsNull() ? outlineShape : compound;

        //PerformUnmanagedMeshOperations(finalShape);

        TopoDS_Shape finalShape = outlineShape;

        Handle(AIS_Shape) aisText = new AIS_Shape(finalShape);


        Quantity_Color qcol(
            (r != nullptr && i < r->Length ? r[i] / 255.0 : 0.5),
            (g != nullptr && i < g->Length ? g[i] / 255.0 : 0.5),
            (b != nullptr && i < b->Length ? b[i] / 255.0 : 0.5),
            Quantity_TOC_RGB
        );
        aisText->SetColor(qcol);

        // Combine rotation + widthFactor into one transform
        gp_Trsf transform;
        if (rotations != nullptr && i < rotations->Length && std::abs(rotations[i]) > 1e-6)
        {
            gp_Trsf rot;
            rot.SetRotation(gp_Ax1(basePnt, gp_Dir(0, 0, 1)), rotations[i] * M_PI / 180.0);
            transform.Multiply(rot);
        }
        if (widthFactors != nullptr && i < widthFactors->Length && std::abs(widthFactors[i] - 1.0) > 1e-6)
        {
            gp_Trsf scale;
            scale.SetScale(basePnt, widthFactors[i]);
            transform.Multiply(scale);
        }
        aisText->SetLocalTransformation(transform);

        //aisText->SetDisplayMode(AIS_Shaded); // This can shade the text but the output is very terrible so better dont use it, just keep it outline
        context->Display(aisText, Standard_False);

        native->ais2DShapes.push_back(aisText);
    }

    context->UpdateCurrentViewer();
    view->Redraw();

}

System::String^ TextDrawer::NormalizeText(System::String^ input)
{
    System::Text::StringBuilder^ sb = gcnew System::Text::StringBuilder(input->Length);

    for each(wchar_t ch in input)
    {
        // Full-width A-Z → ASCII
        if (ch >= 0xFF21 && ch <= 0xFF3A) sb->Append((wchar_t)(ch - 0xFF21 + 0x41));
        // Full-width a-z → ASCII
        else if (ch >= 0xFF41 && ch <= 0xFF5A) sb->Append((wchar_t)(ch - 0xFF41 + 0x61));
        // Full-width 0-9 → ASCII
        else if (ch >= 0xFF10 && ch <= 0xFF19) sb->Append((wchar_t)(ch - 0xFF10 + 0x30));
        // Full-width space → normal space
        else if (ch == 0x3000) sb->Append(L' ');

        // Half-width Katakana symbols
        else if (ch >= 0xFF61 && ch <= 0xFF65)
        {
            switch (ch)
            {
            case 0xFF61: sb->Append(L'。'); break; // ｡
            case 0xFF62: sb->Append(L'「'); break; // ｢
            case 0xFF63: sb->Append(L'」'); break; // ｣
            case 0xFF64: sb->Append(L'、'); break; // ､
            case 0xFF65: sb->Append(L'・'); break; // ･  ? Middle dot
            case 0x30FB: sb->Append(L'・'); break; // ･  ? Middle dot
            }
        }
        // Half-width Katakana characters U+FF66?U+FF9D → full-width Katakana
        else if (ch >= 0xFF66 && ch <= 0xFF9D)
        {
            wchar_t full = ch - 0xFF66 + 0x30A6; // adjust start if needed
            sb->Append(full);
        }
        else sb->Append(ch); // Kanji, ASCII, punctuation remain
    }

    return sb->ToString();
}