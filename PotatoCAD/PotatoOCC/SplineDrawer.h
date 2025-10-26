#pragma once

using namespace System;

namespace PotatoOCC
{
    public ref class SplineDrawer
    {
    public:
        static array<int>^ DrawSplineBatch(
            System::IntPtr ctxPtr,
            array<double>^ x, array<double>^ y, array<double>^ z,
            array<int>^ r, array<int>^ g, array<int>^ b,
            array<double>^ transparency,
            int degree);

        static array<int>^ SplineDrawer::DrawSplineWithKnotsBatch(
            IntPtr ctxPtr,
            const double* xArr,
            const double* yArr,
            const double* zArr,
            const int* rArr,
            const int* gArr,
            const int* bArr,
            const double* transparencyArr,
            int degree,
            const double* knotArr,
            const int* multArr,
            int numPoints,
            int numKnots);
    };
}