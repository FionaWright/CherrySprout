#ifndef H_POWER_HEURISTIC_H
#define H_POWER_HEURISTIC_H

float GenPowerHeuristic(float pdf1, float pdf2, float P)
{
    float pdf1_P = pow(pdf1, P);
    float pdf2_P = pow(pdf2, P);
    return pdf1_P / (pdf1_P + pdf2_P);
}

float PowerHeuristic(float pdf1, float pdf2)
{
    return GenPowerHeuristic(pdf1, pdf2, 2);
}

float BalanceHeuristic(float pdf1, float pdf2)
{
    return GenPowerHeuristic(pdf1, pdf2, 1);
}

#endif