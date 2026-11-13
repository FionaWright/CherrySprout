struct float3;
struct Vec;
struct Ray;

Vec r, rdx0, rdy0, rdx1, rdy1;
for (short i = 0; i < spp; i++)
{
    Path basePath;
    double jacobian;

    basePath.prob = 1.0f;
    Vec rayDir = CalculateRayDir(camera);
    Ray ray(rayDir);

    if (!TracePath(ray))
        continue;
    
    Vec sumOfContribs;
    float sumOfProbs = 0.0f;

    for (int vert = 0; vert < basePath.vertCount; vert++)
    {
        Path auxPath;
        copy(auxPath.vertices, basePath.vertices);
        auxPath.vertCount = vert + 1;

        Vec3 currentContrib = EstimatePath(auxPath, ray);
        float currentProb = auxPath.prob;

        if (currentProb <= 0.0f)l
            continue;
        
        float3 leftPixel, upperPixel;
        float3 rightPixel, bottomPixel;
        double wX0 = 1, wY0 = 1;
        double wX1 = 1, wY1 = 1;

        r += (currentContrib / currentProb) / spp;

        Path oPath; // Shifted path

        float3 offsetRayDir = CalculateOffsetRayDir();
        Ray offsetRay(offsetRayDir);
        if (ShiftPath(auxPath, oPath, offsetRay))
        {
            leftPixel = EstimatePath(offsetRay) * jacobian;
            double p = oPath.prob * jacobian;
            wX0 = currentProb / (currentProb + p);
        }

        offsetRayDir = CalculateNextOffsetRayDir();
        Ray offsetRay(offsetRayDir);
        if (ShiftPath(auxPath, oPath, offsetRay))
        {
            upperPixel = EstimatePath(offsetRay) * jacobian;
            double p = oPath.prob * jacobian;
            wY0 = currentProb / (currentProb + p);
        }

        offsetRayDir = CalculateNextOffsetRayDir();
        Ray offsetRay(offsetRayDir);
        if (ShiftPath(auxPath, oPath, offsetRay))
        {
            rightPixel = EstimatePath(offsetRay) * jacobian;
            double p = oPath.prob * jacobian;
            wX1 = currentProb / (currentProb + p);
        }

        offsetRayDir = CalculateNextOffsetRayDir();
        Ray offsetRay(offsetRayDir);
        if (ShiftPath(auxPath, oPath, offsetRay))
        {
            bottomPixel = EstimatePath(offsetRay) * jacobian;
            double p = oPath.prob * jacobian;
            wY1 = currentProb / (currentProb + p);
        }
        
        rdxO += (currentContrib — leftPixel) * (wX0 / (currentProb * SPP));
        rdyO += (currentContrib — upperPixel) * (wY0 / (currentProb * SPP));
        rdxl += (rightPixel — currentContrib) * (wX1 / (currentProb * SPP));
        rdyl += (bottomPixel — currentContrib) * (wY1 / (currentProb * SPP)); 
    } 
}