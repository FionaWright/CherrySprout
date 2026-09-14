#ifndef H_SAMPLE_GRADIENT_H
#define H_SAMPLE_GRADIENT_H

float3 UnnamedFunction(PathSample mainPathSample, float2 delta)
{
    float3 shiftedOrigin = mainPathSample.Origin;
    float3 shiftedDirection = mainPathSample.Direction;
    ShiftPrimaryRay(shiftedOrigin, shiftedDirection, delta);

    ? differential = ReplayPath(mainPathSample, shiftedOrigin, shiftedDirection);

    return EvaluateGradient(mainPathSample, differential);
}

// After Trace()
GradientSample SampleGradient(PathSample mainPathSample)
{
    GradientSample gradientSample;
    gradientSample.Primal = mainPathSample.Lo;

    gradientSample.GX = UnnamedFunction(mainPathSample, float2(EPSILON, 0));
    gradientSample.GY = UnnamedFunction(mainPathSample, float2(0, EPSILON));

    return gradientSample;
}

#endif