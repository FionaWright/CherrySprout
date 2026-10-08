#ifndef H_POWER_HEURISTIC_H
#define H_POWER_HEURISTIC_H

float GenPowerHeuristic(float pdf1, float pdf2, uint N1, uint N2, float P)
{
    if (DEBUG_ENABLED(FixedMisHalf))
        return 0.5f;

    float pdf1_P = pow(pdf1, P) * N1;
    float pdf2_P = pow(pdf2, P) * N2;
    return pdf1_P / (pdf1_P + pdf2_P);
}

float PowerHeuristic(float pdf1, float pdf2, uint N1, uint N2)
{
    return GenPowerHeuristic(pdf1, pdf2, N1, N2, 2);
}

float BalanceHeuristic(float pdf1, float pdf2, uint N1, uint N2)
{
    return GenPowerHeuristic(pdf1, pdf2, N1, N2, 1);
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
    return pdfArea * distance * distance / max(EPSILON, NdL);
}

#endif