#ifndef H_FEAT_FLAG_COMBINATIONS_H
#define H_FEAT_FLAG_COMBINATIONS_H

#define PATH_VERTICES_REQUIRED_D FEAT_CORE_D(GradientDomain) || FEAT_CORE_D(RestirPT) || FEAT_CORE_D(RestirGPT) || FEAT_DBG_D(EvaluatePathTest)
#define PATH_VERTICES_REQUIRED FEAT_CORE(GradientDomain) || FEAT_CORE(RestirPT) || FEAT_CORE(RestirGPT) || FEAT_DBG(EvaluatePathTest)

#endif