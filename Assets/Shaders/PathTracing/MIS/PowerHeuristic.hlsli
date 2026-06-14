#ifndef H_POWER_HEURISTIC_H
#define H_POWER_HEURISTIC_H

float PowerHeuristic(float pdf1, float pdf2)
{
    pdf1_2 = pdf1 * pdf1;
    pdf2_2 = pdf2 * pdf2;
    return pdf1_2 / (pdf1_2 + pdf2_2);
}

#endif