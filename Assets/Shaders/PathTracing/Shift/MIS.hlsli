#ifndef H_SHIFT_MIS_H
#define H_SHIFT_MIS_H

// TODO: Should make sure last ray was not dirac delta before applying NEE weights

//#define GDPT_MIS_WEIGHT_PATH 0.5f
//#define GDPT_MIS_WEIGHT_NEE 0.5f

void getMisWeights_BxdfSubpath(
    PathStateShifted pathStateShifted,
    float pdf_prefix_m,
    float pdf_prefix_s,
    float pdf_bxdf_m,
    float pdf_bxdf_s,
    float pdf_nee_m,
    float pdf_nee_s,
    float jacobian,

    out float mis_nee_s,
    out float mis_nee_m,
    out float mis_path)
{
    float mainPathPDF_BXDF = pdf_prefix_m * pdf_bxdf_m;
    float shiftedPathPDF_BXDF = jacobian * pdf_prefix_s * pdf_bxdf_s;

    if (!FEAT_CORE(NEE) || !pathStateShifted.IsUsingNeeMis)
    {
        // Selecting (Main) PDF
        mis_path = BalanceHeuristic(
            mainPathPDF_BXDF, shiftedPathPDF_BXDF,
            1, 1);

        mis_nee_s = 1.0f;
        mis_nee_m = 1.0f;

#ifdef GDPT_MIS_WEIGHT_PATH
        mis_path = GDPT_MIS_WEIGHT_PATH;
#endif
        return;
    }

    float mainPathPDF_NEE = pdf_prefix_m * pdf_nee_m;
    float shiftedPathPDF_NEE = jacobian * pdf_prefix_s * pdf_nee_s;

    //mis_path = BalanceHeuristic(
    //    mainPathPDF_BXDF, mainPathPDF_NEE, shiftedPathPDF_BXDF, shiftedPathPDF_NEE,
    //    1, gSettings.DirectNumSamples, 1, gSettings.DirectNumSamples);

    //mis_path = BalanceHeuristic(
    //    pdf_prefix_m, jacobian * pdf_prefix_s,
    //    1, 1);

    mis_path = BalanceHeuristic(
        mainPathPDF_BXDF, shiftedPathPDF_BXDF,
        1, 1);

    mis_nee_m = BalanceHeuristic(
        pdf_bxdf_m, pdf_nee_m,
        1, gSettings.DirectNumSamples);

    mis_nee_s = BalanceHeuristic(
        pdf_bxdf_s, pdf_nee_s,
        1, gSettings.DirectNumSamples);

#ifdef GDPT_MIS_WEIGHT_PATH
    mis_path = GDPT_MIS_WEIGHT_PATH;
#endif
#ifdef GDPT_MIS_WEIGHT_NEE
    mis_nee_m = GDPT_MIS_WEIGHT_NEE;
    mis_nee_s = GDPT_MIS_WEIGHT_NEE;
#endif
}

void getMisWeights_NeeSubpath(
    PathStateShifted pathStateShifted,
    float pdf_prefix_m,
    float pdf_prefix_s,
    float pdf_bxdf_m,
    float pdf_bxdf_s,
    float pdf_nee_m,
    float pdf_nee_s,
    float jacobian,

    out float mis_nee_s,
    out float mis_nee_m,
    out float mis_path)
{
    float mainPathPDF_NEE = pdf_prefix_m * pdf_nee_m;
    float shiftedPathPDF_NEE = jacobian * pdf_prefix_s * pdf_nee_s;

    if (!FEAT_CORE(NEE) || !pathStateShifted.IsUsingNeeMis)
    {
        // Selecting (Main) PDF
        mis_path = BalanceHeuristic(
            mainPathPDF_NEE, shiftedPathPDF_NEE,
            1, 1);

        mis_nee_s = 1.0f;
        mis_nee_m = 1.0f;

#ifdef GDPT_MIS_WEIGHT_PATH
        mis_path = GDPT_MIS_WEIGHT_PATH;
#endif
        return;
    }

    float mainPathPDF_BXDF = pdf_prefix_m * pdf_bxdf_m;
    float shiftedPathPDF_BXDF = jacobian * pdf_prefix_s * pdf_bxdf_s;

    //mis_path = BalanceHeuristic(
    //    mainPathPDF_NEE, mainPathPDF_BXDF, shiftedPathPDF_NEE, shiftedPathPDF_BXDF,
    //    gSettings.DirectNumSamples, 1, gSettings.DirectNumSamples, 1);

    //mis_path = BalanceHeuristic(
    //    pdf_prefix_m, jacobian * pdf_prefix_s,
    //    1, 1);

    mis_path = BalanceHeuristic(
        mainPathPDF_NEE, shiftedPathPDF_NEE,
        1, 1);

    mis_nee_m = BalanceHeuristic(
        pdf_nee_m, pdf_bxdf_m,
        gSettings.DirectNumSamples, 1);

    mis_nee_s = BalanceHeuristic(
        pdf_nee_s, pdf_bxdf_s,
        gSettings.DirectNumSamples, 1);

#ifdef GDPT_MIS_WEIGHT_PATH
    mis_path = GDPT_MIS_WEIGHT_PATH;
#endif
#ifdef GDPT_MIS_WEIGHT_NEE
    mis_nee_m = GDPT_MIS_WEIGHT_NEE;
    mis_nee_s = GDPT_MIS_WEIGHT_NEE;
#endif
}

#endif