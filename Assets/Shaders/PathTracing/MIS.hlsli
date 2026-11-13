#ifndef H_POWER_HEURISTIC_H
#define H_POWER_HEURISTIC_H

float GenPowerHeuristic(float pdf1, float pdf2, uint N1, uint N2, float P)
{
    if (FEAT_DBG(FixedMisHalf))
        return 0.5f;

    float pdf1_P = pow(pdf1, P) * N1;
    float pdf2_P = pow(pdf2, P) * N2;

    float denom = pdf1_P + pdf2_P;

    if (denom <= 0.0f)
        return 0.0f;

    return pdf1_P / denom;
}

float GenPowerHeuristic(float pdf1, float pdf2, float pdf3, float pdf4, uint N1, uint N2, uint N3, uint N4, float P)
{
    if (FEAT_DBG(FixedMisHalf))
        return 0.5f;

    float pdf1_P = pow(pdf1, P) * N1;
    float pdf2_P = pow(pdf2, P) * N2;
    float pdf3_P = pow(pdf3, P) * N3;
    float pdf4_P = pow(pdf4, P) * N4;

    float denom = pdf1_P + pdf2_P + pdf3_P + pdf4_P;

    if (denom <= 0.0f)
        return 0.0f;

    return pdf1_P / denom;
}

float PowerHeuristic(float pdf1, float pdf2, uint N1, uint N2)
{
    return GenPowerHeuristic(pdf1, pdf2, N1, N2, 2);
}

float PowerHeuristic(float pdf1, float pdf2, float pdf3, float pdf4, uint N1, uint N2, uint N3, uint N4)
{
    return GenPowerHeuristic(pdf1, pdf2, pdf3, pdf4, N1, N2, N3, N4, 2);
}

float BalanceHeuristic(float pdf1, float pdf2, uint N1, uint N2)
{
    return GenPowerHeuristic(pdf1, pdf2, N1, N2, 1);
}

float BalanceHeuristic(float pdf1, float pdf2, float pdf3, float pdf4, uint N1, uint N2, uint N3, uint N4)
{
    return GenPowerHeuristic(pdf1, pdf2, pdf3, pdf4, N1, N2, N3, N4, 1);
}

float BalanceHeuristicRatio(float pdfRatio2_1, uint N1, uint N2)
{
    float NRatio = N2 / N1;
    return 1.0f / (1.0f + pdfRatio2_1 * NRatio);
}

// https://computergraphics.stackexchange.com/questions/13320/confusion-about-pdf-defined-in-solid-angle-area-measure
// Directions relative to area that PDF belongs to
float PdfAreaToSolidAngle(float pdfArea, float NdL, float distance)
{
    if (NdL <= 0.0f || distance <= 0.0f)
        return 0.0f;

    return pdfArea * distance * distance / NdL;
}

#endif