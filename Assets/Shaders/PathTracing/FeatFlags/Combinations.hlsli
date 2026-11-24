#ifndef H_FEAT_FLAG_COMBINATIONS_H
#define H_FEAT_FLAG_COMBINATIONS_H

#define PATH_VERTICES_REQUIRED_D FEAT_ADV_D(GradientDomain) || FEAT_ADV_D(RestirPT) || FEAT_ADV_D(RestirGPT) || FEAT_DBG_D(EvaluatePathTest)
#define PATH_VERTICES_REQUIRED FEAT_ADV(GradientDomain) || FEAT_ADV(RestirPT) || FEAT_ADV(RestirGPT) || FEAT_DBG(EvaluatePathTest)
#define RESTIR_ENABLED FEAT_ADV(RestirDI) || FEAT_ADV(RestirGI) || FEAT_ADV(RestirPT) || FEAT_ADV(RestirGPT) || FEAT_ADV(RestirPG)

#endif