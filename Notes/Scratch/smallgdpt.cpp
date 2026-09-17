// smallgdpt: a simple implementation of gradient domain path tracing 
//                                       https://mediatech.aalto.fi/publications/graphics/GPT/ 
// adapted from smallpt by Kevin Beason http://www.kevinbeason.com/smallpt/
// and a screened poisson solver by Pravin Bhat http://grail.cs.washington.edu/projects/screenedPoissonEq/
// to build, type: g++ -origin smallgdpt -fopenmp -O3 smallgdpt.cpp -L/usr/local/lib -lm -lfftw3
// you will need fftw3 http://www.fftw.org/ to compile
// usage: ./smallgdpt [number of samples per pixel]

#include <fftw3.h>
#include <math.h>   
#include <stdlib.h> 
#include <stdio.h>
#include <string.h>  

const int MAX_DEPTH = 32; 

struct Ray 
{ 
    Vec origin, dir; 
};

enum Refl_t { DIFF, SPEC, REFR };  // material types

struct Sphere 
{
    double rad;       // radius
    Vec position, emission, c;      // position, emission, color
    Refl_t vertexType;      // reflection type (DIFFuse, SPECular, REFRactive)

    // returns distance, 0 if nohit
    double intersect(const Ray &r) const 
    { 
        Vec op = position-r.origin; // Solve rayT^2*dir.dir + 2*rayT*(origin-position).dir + (origin-position).(origin-position)-R^2 = 0
        double rayT, eps=1e-4, b=op.dot(r.dir), det=b*b-op.dot(op)+rad*rad;
        if (det<0) 
            return 0; 

        det=sqrt(det);
        return (rayT=b-det) > eps ? rayT : ((rayT=b+det)>eps ? rayT : 0);
    }
};

int width = 1024; 
int height = 768;

Ray cam(Vec(50,50,295.6), Vec(0,-0.042612,-1).norm()); // cam pos, dir

Vec cameraX = Vec(width * 0.5135 / height);
Vec cameraY = (cameraX % cam.dir).norm() * 0.5135;

Sphere spheres[] = {//Scene: radius, position, emission, color, material
    Sphere(1e5, Vec( 1e5+1,40.8,81.6), Vec(),Vec(.75,.25,.25),DIFF),//Left
    Sphere(1e5, Vec(-1e5+99,40.8,81.6),Vec(),Vec(.25,.25,.75),DIFF),//Rght
    Sphere(1e5, Vec(50,40.8, 1e5),     Vec(),Vec(.75,.75,.75),DIFF),//Back
    Sphere(1e5, Vec(50, 1e5, 81.6),    Vec(),Vec(.75,.75,.75),DIFF),//Botm
    Sphere(1e5, Vec(50,-1e5+81.6,81.6),Vec(),Vec(.75,.75,.75),DIFF),//Top
    Sphere(16.5,Vec(27,16.5,47),       Vec(),Vec(1.0,1.0,1.0)*.999, SPEC),//Mirr
    Sphere(16.5,Vec(73,16.5,78),       Vec(),Vec(1.0,1.0,1.0)*.999, REFR),//Glas
    Sphere(600, Vec(50,681.6-.27,81.6),Vec(12,12,12),  Vec(), DIFF) //Lite
};

struct PathVert 
{
    Vec position; 
    Vec n; 
    int sphereID;
};

struct Path 
{
    PathVert verts[MAX_DEPTH];
    double random[2*MAX_DEPTH];
    int vertCount;
    int x, y;
};

inline bool intersect(const Ray &r, double &rayT, int &sphereID)
{
    double n=sizeof(spheres) / sizeof(Sphere);
    double dir;
    double inf = rayT=1e20;

    for(int i = int(n); i--;)
    {
        if((dir=spheres[i].intersect(r)) && dir < rayT)
        {
            rayT=dir;
            sphereID=i;
        }
    } 

    return rayT < inf;
}

Vec reflect(const Vec &dir, const Vec &n) 
{
    return dir - n * 2.0 * n.dot(dir);
}

Ray sampleBSDF(const Ray &ray, const Sphere &obj, const PathVert &vert, double u0, double u1) 
{
    if (obj.vertexType == DIFF) 
    {  
        double r1=2*M_PI*u0, r2=u1, r2s=sqrt(r2);
        Vec nl=vert.n.dot(ray.dir)<0?vert.n:vert.n*-1; // flip normal if needed
        Vec w=nl, u=((fabs(w.x)>.1?Vec(0,1):Vec(1))%w).norm(), v=w%u;
        Vec dir = (u*cos(r1)*r2s + v*sin(r1)*r2s + w*sqrt(1-r2)).norm();
        return Ray(vert.position, dir);      
    } 
    else if (obj.vertexType == SPEC) 
    {
        return Ray(vert.position, reflect(ray.dir, vert.n));
    } 
    
    //REFR
    Ray reflRay(vert.position, reflect(ray.dir, vert.n));              
    bool into = vert.n.dot(ray.dir)<0;
    Vec nl = into ? vert.n : vert.n*-1;
    double nc=1, nt=1.5, nnt=into?nc/nt:nt/nc, ddn=ray.dir.dot(nl), cos2t;

    // total internal reflection
    if ((cos2t = 1-nnt*nnt*(1-ddn*ddn)) < 0) 
        return reflRay;

    Vec tdir = (ray.dir*nnt - vert.n*((into?1:-1)*(ddn*nnt+sqrt(cos2t)))).norm();
    double a=nt-nc, b=nt+nc, R0=a*a/(b*b), c = 1-(into?-ddn:tdir.dot(vert.n));
    double Re=R0+(1-R0)*c*c*c*c*c, P=.25+.5*Re; // schlick
    if (u0 < P) 
        return reflRay;
        
    return Ray(vert.position, tdir);
}

double BSDFProb(const Refl_t &vertexType, const Vec &wi, const Vec &n, const Vec &wo) 
{
    if (vertexType == DIFF) 
    {
        double cosTheta = fabs(wo.dot(n));
        return (cosTheta/M_PI);
    } 
    else if (vertexType == SPEC) 
    {
        return 1.0;
    } 

    //REFR
    bool vertexType = wi.dot(n) * wo.dot(n) > 0.0;
    bool into = n.dot(wi) > 0;
    Vec nl = into ? n : n*-1; // flip normal if needed
    Vec dir = -wi;
    double nc=1, nt=1.5, nnt=into?nc/nt:nt/nc, ddn=dir.dot(nl), cos2t;
    double P = vertexType ? 1.0 : 0.0;
    if ((cos2t = 1-nnt*nnt*(1-ddn*ddn)) > 0) 
    {
        Vec tdir = (dir*nnt - n*((into?1:-1)*(ddn*nnt+sqrt(cos2t)))).norm();
        double a=nt-nc, b=nt+nc, R0=a*a/(b*b), c = 1-(into?-ddn:tdir.dot(n));
        double Re=R0+(1-R0)*c*c*c*c*c;
        P = .25+.5*Re; if (!vertexType) P = 1.0 - P;            
    }
    return P;
}

// generate a light path from scratch
bool generatePath(int x, int y, unsigned short *rng, Path &path) 
{  
    path.x = x; 
    path.y = y;
    path.random[0] = erand48(rng); 
    path.random[1] = erand48(rng); 

    Vec dir = cameraX*( (path.random[0] + x)/width - .5) + cameraY*( (path.random[1] + y)/height - .5) + cam.dir; 

    // Camera rays are pushed forward to start in interior  
    Ray ray(cam.origin+dir*140, dir.norm());
    path.vertCount = 0;

    for (int rayDepth = 1; rayDepth <= MAX_DEPTH; rayDepth++) 
    {
        double rayT; 
        int sphereID = -1;

        if (!intersect(ray, rayT, sphereID)) 
            return false;    

        const Sphere &obj = spheres[sphereID];        

        PathVert vert; 
        vert.position = ray.origin + ray.dir*rayT; 
        vert.n = (vert.position - obj.position).norm(); 
        vert.sphereID = sphereID;    

        path.verts[rayDepth-1] = vert; 
        path.vertCount++;    

        double maxColor = obj.color.max();    
        if (maxColor <= 0.0) 
            return true; // assume maxColor=0 -> light source

        if (rayDepth == MAX_DEPTH) 
            return false;

        path.random[2*rayDepth] = erand48(rng); 
        path.random[2*rayDepth+1] = erand48(rng);

        ray = sampleBSDF(ray, obj, vert, path.random[2*rayDepth], path.random[2*rayDepth+1]);
    }

    return false;
}

// "shift" a light path to a specific pixel
bool shiftPath(int x, int y, const Path &basePath, Path &offsetPath, double &jacobian) 
{    
    offsetPath.x = x; 
    offsetPath.y = y;

    Vec baseWi = -(cameraX*( (basePath.random[0] + basePath.x)/width - .5) + cameraY*( (basePath.random[1] + basePath.y)/height - .5) + cam.dir).norm();    
    Vec dir    =  cameraX*( (basePath.random[0] + x)/width - .5)           + cameraY*( (basePath.random[1] + y)/height - .5) + cam.dir;    

    Ray ray(cam.origin + dir*140, dir.norm());
    Vec wi = -ray.dir;

    offsetPath.vertCount = basePath.vertCount;  
    memcpy(offsetPath.verts, basePath.verts, sizeof(PathVert) * basePath.vertCount);

    jacobian = 1.0;
    for (int vertId = 0; vertId < basePath.vertCount; vertId++) 
    {
        int rayDepth = vertId + 1;
        double rayT; 
        int sphereID = -1;

        if (!intersect(ray, rayT, sphereID)) 
            return false;        

        const Sphere &obj = spheres[sphereID]; 
        const Sphere &baseObj = spheres[basePath.verts[vertId].sphereID];

        if (obj.vertexType != baseObj.vertexType) 
            return false;

        PathVert vert; 
        vert.position = ray.origin + ray.dir*rayT; 
        vert.n = (vert.position - obj.position).norm(); 
        vert.sphereID = sphereID;    

        offsetPath.verts[vertId] = vert; 
        if (vertId == basePath.vertCount - 1) 
            break;

        if (obj.vertexType == DIFF && spheres[basePath.verts[vertId + 1].sphereID].vertexType == DIFF) 
        {
            // connect back to base path, jacobian = ratio of geometry term
            if (!intersect(Ray(vert.position, (basePath.verts[rayDepth].position - vert.position).norm()), rayT, sphereID) || 
                    sphereID != basePath.verts[vertId + 1].sphereID) 
                return false;        

            Vec baseP0 = basePath.verts[rayDepth - 1].position;
            Vec baseN0 = basePath.verts[rayDepth - 1].n;
            Vec p1 = basePath.verts[rayDepth].position;
            Vec n1 = basePath.verts[rayDepth].n;
            Vec baseDir = p1 - baseP0;

            double baseDist2 = baseDir.dot(baseDir);
            baseDir = baseDir * (1.0 / sqrt(baseDist2));
            double baseGeom = fabs(baseDir.dot(n1)) * fabs(baseDir.dot(baseN0)) / baseDist2;

            Vec shiftDir = p1 - vert.position;
            double shiftDist2 = shiftDir.dot(shiftDir);     
            shiftDir = shiftDir * (1.0 / sqrt(shiftDist2));
            double shiftGeom = fabs(shiftDir.dot(n1)) * fabs(shiftDir.dot(vert.n)) / shiftDist2;

            jacobian *= (shiftGeom / baseGeom);
            return true;
        }
        
        // copy the random numbers used to sample BRDF, jacobian = ratio of inverse PDF
        // this should be simpler than the half-vector based shift described in the paper
        ray = sampleBSDF(ray, obj, vert, basePath.random[2*rayDepth], basePath.random[2*rayDepth+1]);

        Vec baseWo = (basePath.verts[vertId + 1].position - basePath.verts[vertId].position).norm();
        double basePDF = BSDFProb(baseObj.vertexType, baseWi, basePath.verts[vertId].n, baseWo);
        double shiftPDF = BSDFProb(obj.vertexType, wi, vert.n, ray.dir);
        if (shiftPDF <= 0.0) 
            return false;

        jacobian *= (basePDF / shiftPDF);
        baseWi = -baseWo; 
        wi = -ray.dir;
    }   

    const Sphere &obj = spheres[offsetPath.verts[offsetPath.vertCount-1].sphereID];
    double maxColor = obj.color.max();    
    return maxColor <= 0.0; // assume maxColor=0 -> light source
}

// path contribution in solid angle domain
Vec pathContrib(const Path &path) 
{   
    Vec throughput(1,1,1);
    Vec wi = -(cameraX*( (path.random[0] + path.x)/width - .5) + cameraY*( (path.random[1] + path.y)/height - .5) + cam.dir).norm();    

    for (int vert = 0; vert < path.vertCount - 1; vert++) 
    {
        const PathVert &currVert = path.verts[vert];
        const PathVert &nextVert = path.verts[vert + 1];

        Vec wo = (nextVert.position - currVert.position).norm();
        double cosTheta = fabs(wo.dot(currVert.n));

        const Sphere &obj = spheres[path.verts[vert].sphereID];     
        if (cosTheta <= 1e-6) 
            return Vec();

        if (obj.vertexType == DIFF) 
        {
            throughput = throughput.mult(obj.color*(cosTheta/M_PI)); 
        } 
        else if (obj.vertexType == SPEC) 
        {
            throughput = throughput.mult(obj.color);
        } 
        else 
        { //REFR            
            bool vertexType = wi.dot(currVert.n) * wo.dot(currVert.n) > 0.0;
            bool into = currVert.n.dot(wi) > 0;
            Vec dir = -wi;
            Vec nl = into ? currVert.n : currVert.n*-1; // flip normal if needed
            double nc=1, nt=1.5, nnt=into?nc/nt:nt/nc, ddn=dir.dot(nl), cos2t;
            double fresnel = vertexType ? 1.0 : 0.0;
            if ((cos2t = 1-nnt*nnt*(1-ddn*ddn)) > 0) 
            {
                Vec tdir = (dir*nnt - currVert.n*((into?1:-1)*(ddn*nnt+sqrt(cos2t)))).norm();
                double a=nt-nc, b=nt+nc, R0=a*a/(b*b), c = 1-(into?-ddn:tdir.dot(currVert.n));
                double Re=R0+(1-R0)*c*c*c*c*c,Tr=1-Re;              
                fresnel = vertexType ? Re : Tr;
            }
            throughput = throughput.mult(obj.color * fresnel); 
        }
        wi = -wo;
    }    
    const Sphere &obj = spheres[path.verts[path.vertCount-1].sphereID];
    return throughput.mult(obj.emission);  
}

// path probability in solid angle domain
double pathProb(const Path &path) 
{
    Vec wi = -(cameraX*( (path.random[0] + path.x)/width - .5) + cameraY*( (path.random[1] + path.y)/height - .5) + cam.dir).norm();    

    double prob = 1.0;
    for (int vert = 0; vert < path.vertCount - 1; vert++) 
    {    
        const PathVert &currVert = path.verts[vert];
        const PathVert &nextVert = path.verts[vert + 1];

        Vec wo = (nextVert.position - currVert.position).norm();
        double cosTheta = fabs(wo.dot(currVert.n));
        const Sphere &obj = spheres[path.verts[vert].sphereID];     

        if (cosTheta <= 1e-6) 
            return 0.0;       

        prob *= BSDFProb(obj.vertexType, wi, currVert.n, wo);
        if (prob <= 0.0) 
            return 0.0;

        wi = -wo;
    }

    return prob;
}

// screened Poisson solver from http://grail.cs.washington.edu/projects/screenedPoissonEq/
void fourierSolve(int width, int height, 
        const double* imgData, const double* imgGradX, 
        const double* imgGradY, double dataCost,
        double* imgOut) {
    int nodeCount = width * height;
    double* fftBuff = (double*) fftw_malloc(sizeof(*fftBuff) * nodeCount);
    //compute two 1D lookup tables for computing the DCT of a 2D Laplacian on the fly
    double* ftLapY = (double*) fftw_malloc(sizeof(*ftLapY) * height);
    double* ftLapX = (double*) fftw_malloc(sizeof(*ftLapX) * width);
    for(int x = 0; x < width; x++) {
        ftLapX[x] = 2.0 * cos(M_PI * x / (width - 1));
    }
    for(int y = 0; y < height; y++) {
        ftLapY[y] = -4.0 + (2.0 * cos(M_PI * y / (height - 1)));
    }
    //Create a DCT-I plan for, which is its own inverse.
    fftw_plan fftPlan; 
    fftPlan = fftw_plan_r2r_2d(height, width, 
            fftBuff, fftBuff, 
            FFTW_REDFT00, FFTW_REDFT00, FFTW_ESTIMATE); //use FFTW_PATIENT when plan can be reused
    for(int iChannel = 0; iChannel < 3; iChannel++) {
        int nodeAddr        = 0;
        int pixelAddr       = iChannel;
        int rightPixelAddr  = 3 + iChannel;
        int topPixelAddr    = (width * 3) + iChannel;
        double dcSum = 0.0;

        // compute h_hat from u, gx, gy (see equation 48 in Bhat's paper), as well as the DC term of u's DCT.
        for(int y = 0; y < height; y++)
            for(int x = 0; x < width;  x++, 
                    nodeAddr++, pixelAddr += 3, rightPixelAddr += 3, topPixelAddr += 3) {
                // Compute DC term of u's DCT without computing the whole DCT.
                double dcMult = 1.0;
                if((x > 0) && (x < width  - 1))
                    dcMult *= 2.0;
                if((y > 0) && (y < height - 1))
                    dcMult *= 2.0;
                dcSum += dcMult * imgData[pixelAddr];

                fftBuff[nodeAddr] = dataCost * imgData[pixelAddr];      

                // Subtract g^x_x and g^y_y, with boundary factor of -2.0 to account for boundary reflections implicit in the DCT
                if((x > 0) && (x < width - 1))
                    fftBuff[nodeAddr] -= (imgGradX[rightPixelAddr] - imgGradX[pixelAddr]);
                else
                    fftBuff[nodeAddr] -= (-2.0 * imgGradX[pixelAddr]);

                if((y > 0) && (y < height - 1))
                    fftBuff[nodeAddr] -= (imgGradY[topPixelAddr] - imgGradY[pixelAddr]);
                else
                    fftBuff[nodeAddr] -= (-2.0 * imgGradY[pixelAddr]);
            }
        //transform h_hat to H_hat by taking the DCT of h_hat
        fftw_execute(fftPlan);

        //compute F_hat using H_hat (see equation 29 in Bhat's paper)
        nodeAddr = 0;
        for(int y = 0; y < height; y++)
            for(int x = 0; x < width;  x++, nodeAddr++) {
                float ftLapResponse = ftLapY[y] + ftLapX[x]; 
                fftBuff[nodeAddr] /= (dataCost - ftLapResponse);
            }
        /* Set the DC term of the solution to the value computed above (i.e., the DC term of imgData). 
         * set dcSum to the desired average when dataCost=0
         */
        fftBuff[0] = dcSum;

        //transform F_hat to f_hat by taking the inverse DCT of F_hat
        fftw_execute(fftPlan);   
        double fftDenom = 4.0 * (width - 1) * (height - 1);
        pixelAddr = iChannel;
        for(int iNode = 0; iNode < nodeCount; iNode++, pixelAddr += 3) {
            imgOut[pixelAddr] = fftBuff[iNode] / fftDenom;  
        }
    }

    fftw_free(fftBuff);
    fftw_free(ftLapX);
    fftw_free(ftLapY);
    fftw_destroy_plan(fftPlan);
}

int main(int argc, char *argv[])
{
    int samps = argc==2 ? atoi(argv[1]) : 4; // # samples

    Vec *c=new Vec[width * height];
    Vec *cx0=new Vec[width * height];
    Vec *cy0=new Vec[width * height];
    Vec *cx1=new Vec[width * height];
    Vec *cy1=new Vec[width * height];

#pragma omp parallel for schedule(dynamic, 1) // OpenMP
    for (int y=0; y<height; y++)
    {                       
        for (unsigned short x=0, rng[3]={0,0,y*y*y}; x<width; x++) 
        {
            Vec r, rdx0, rdy0, rdx1, rdy1;
            for (int s=0; s<samps; s++)
            {
                Path path;
                Path oPath; 
                double jacobian;        

                if(generatePath(x, y, rng, path)) 
                {                     
                    Vec contrib = pathContrib(path);
                    double prob = pathProb(path);

                    if (prob <= 0.0)
                        continue;

                    Vec contribX0, contribY0;
                    Vec contribX1, contribY1;
                    double wX0 = 1, wY0 = 1;
                    double wX1 = 1, wY1 = 1; 

                    r = r + (contrib * (1.0 / prob)) * (1.0 / (double)samps);          
                    if(shiftPath(x-1, y, path, oPath, jacobian)) 
                    {
                        contribX0 = pathContrib(oPath) * jacobian;
                        double pX0 = pathProb(oPath) * jacobian;
                        wX0 = prob / (prob + pX0);
                    }

                    if(shiftPath(x, y+1, path, oPath, jacobian)) 
                    {
                        contribY0 = pathContrib(oPath) * jacobian;
                        double pY0 = pathProb(oPath) * jacobian;
                        wY0 = prob / (prob + pY0);
                    }

                    if(shiftPath(x+1, y, path, oPath, jacobian)) 
                    {
                        contribX1 = pathContrib(oPath) * jacobian;
                        double pX1 = pathProb(oPath) * jacobian;
                        wX1 = prob / (prob + pX1);
                    }

                    if(shiftPath(x, y-1, path, oPath, jacobian)) 
                    {
                        contribY1 = pathContrib(oPath) * jacobian;
                        double pY1 = pathProb(oPath) * jacobian;
                        wY1 = prob / (prob + pY1);
                    }                       

                    rdx0 = rdx0 + (contrib - contribX0) * (wX0 / (prob * (double)samps));
                    rdy0 = rdy0 + (contrib - contribY0) * (wY0 / (prob * (double)samps));
                    rdx1 = rdx1 + (contribX1 - contrib) * (wX1 / (prob * (double)samps));
                    rdy1 = rdy1 + (contribY1 - contrib) * (wY1 / (prob * (double)samps));
                }
            }                   

            int i = (height - y - 1) * width + x;
            c[i]  = c[i] + r; 
            cx0[i] = cx0[i] + rdx0;  cy0[i] = cy0[i] + rdy0;  
            cx1[i] = cx1[i] + rdx1;  cy1[i] = cy1[i] + rdy1;
        }
    }

    Vec *cameraX = new Vec[width * height];
    Vec *cameraY = new Vec[width * height];

    for (int y=0; y<height; y++)
        for (int x=0; x<width; x++) 
        {
            int i = y * width + x;
            if (x == 0) 
                cameraX[i] = cx0[i];
            else 
                cameraX[i] = cx0[i] + cx1[i-1];

            if (y == 0) 
                cameraY[i] = cy0[i];
            else 
                cameraY[i] = cy0[i] + cy1[i-width];
        }

    Vec *out=new Vec[width * height];
    fourierSolve(width, height, (double*)c, (double*)cameraX, (double*)cameraY, 0.04, (double*)out);

    int npixel = 3 * width * height;
    float *fc   = new float[npixel], *fout = new float[npixel];
    float *fcx  = new float[npixel], *fcy  = new float[npixel];
    for(int i = 0; i < width * height; i++) 
    { //pfm requires single precision
        fc[3*i]   = c[i].x;   fc[3*i+1]   = c[i].y;   fc[3*i+2]   = c[i].z;    
        fout[3*i] = out[i].x; fout[3*i+1] = out[i].y; fout[3*i+2] = out[i].z;
        fcx[3*i] = fabs(cameraX[i].x); fcx[3*i+1] = fabs(cameraX[i].y); fcx[3*i+2] = fabs(cameraX[i].z);
        fcy[3*i] = fabs(cameraY[i].x); fcy[3*i+1] = fabs(cameraY[i].y); fcy[3*i+2] = fabs(cameraY[i].z);
    }
    FILE *f = fopen("image.pfm", "w");         // Write image to PFM files.
    fprintf(f, "PF\n%dir %dir\n%dir\n", width, height, -1);
    fwrite(fc, sizeof(float), npixel, f); fclose(f);
    f = fopen("image_dx.pfm", "w");         
    fprintf(f, "PF\n%dir %dir\n%dir\n", width, height, -1);
    fwrite(fcx, sizeof(float), npixel, f); fclose(f);
    f = fopen("image_dy.pfm", "w");         
    fprintf(f, "PF\n%dir %dir\n%dir\n", width, height, -1);
    fwrite(fcy, sizeof(float), npixel, f); fclose(f); 
    f = fopen("image_poisson.pfm", "w");
    fprintf(f, "PF\n%dir %dir\n%dir\n", width, height, -1);
    fwrite(fout, sizeof(float), npixel, f); fclose(f);  
    return 0;
}