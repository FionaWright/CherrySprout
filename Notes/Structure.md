# Path-Tracer Shaders Structure

https://diagramgen.app/tree-diagram-generator/

```
graph LR
    PathTracer("PathTracer CS") --> Core
    Core --> InitRNG
    Core --> SamplePath
    Core --> Accumulate
    Core --> GDPT{"GDPT"}
    SamplePath --> GetPrimaryRay
    SamplePath --> TracePath
    GetPrimaryRay --> Jitter{"Jitter"}
    GetPrimaryRay --> DoF{"DoF"}
    TracePath --> ComputeRayHit
    TracePath --> Miss
    TracePath --> AlphaTest{"AlphaTest"}
    TracePath --> Hit
    TracePath --> RR{"RR"}
    ComputeRayHit --> Reconstruct{"Reconstruct"}
    ComputeRayHit --> GetHitInfo
    GetHitInfo --> ExtractInterpolatedAttributes
    GetHitInfo --> ApplyMaterialTextures
    GetHitInfo --> ApplyNormalMap
    Reconstruct --> ApplyMaterialTextures
    ExtractInterpolatedAttributes --> GetTriangleArea
    ExtractInterpolatedAttributes --> ExtractBary
    ExtractInterpolatedAttributes --> ExtractVertices
    Miss --> MissEnvMap{"EnvMap"}
    MissEnvMap --> MissNEE{"NEE"}
    Miss --> DirLight{"DirLight"}
    Hit --> EvalEmission
    EvalEmission --> EmissionNEE{"NEE"}
    Hit --> HitNEE{"NEE"}
    Hit --> SampleIndirect
    HitNEE --> SampleDirect
    SampleDirect --> SDReSTIR{"ReSTIR"}
    SDReSTIR --> SampleReservoir
    SampleDirect --> SampleNEE
    SampleDirect --> Transient{"Transient"}
    SampleReservoir --> TraceShadowRes["TraceShadow"]
    SampleReservoir --> BxdfEvalRes["BxdfEval"]
    SampleIndirect --> BxdfSample
    BxdfSample --> BxDF
    BxdfEval --> BxDF
    BxDF --> MicrofacetModel
    BxDF --> Trans{"Transmission"}
    BxDF --> Aniso{"Anisotropy"}

    subgraph NEESubGraph["NEE"]
    direction LR

    SampleNEE --> SampleLight
    SampleNEE --> TraceShadow
    SampleNEE --> BxdfEval
    SampleLight --> SampleLSD
    SampleLSD --> Alias{"AliasTables"}
    SampleLSD --> BinarySearch
    SampleLight --> SampleEnvMap
    SampleEnvMap --> SampleEnvMapCDF
    SampleLight --> EvalPunctual
    EvalPunctual --> EvalPointLight
    EvalPunctual --> EvalDistantLight
    EvalPunctual --> EvalSpotLight
    SampleLight --> SampleEmissive
    SampleEmissive --> SampleBary
    TraceShadow --> ShadowAlpha{"AlphaTest"}

    end

    subgraph Gradient["Gradient Domain"]
    direction LR

    GDPT --> SampleGradients
    SampleGradients --> SampleShiftedPath
    SampleShiftedPath --> GetPrimaryRayShift["GetPrimaryRay"]
    SampleShiftedPath --> TraceShiftedPath
    TraceShiftedPath --> ComputeRayHitShift["ComputeRayHit"]
    TraceShiftedPath --> MissShift["Miss"]
    TraceShiftedPath --> BxdfEvalShift["BxDFEval"]
    TraceShiftedPath --> HitShifted
    HitShifted --> ShiftReconnect
    HitShifted --> ShiftReconnectEnvironment
    HitShifted --> ShiftHalfVector
    HitShifted --> HitShiftNEE{"NEE"}

    end

    subgraph PrePasses["Pre-Passes"]
    direction LR

    ReSTIRGen{"ReSTIR"} --> GenerateCandidates("GenerateCandidates CS")
    GenerateCandidates --> InitRNGReSTIR["InitRNG"]
    GenerateCandidates --> ReconstructReSTIR["Reconstruct"]
    GenerateCandidates --> CreateReservoir
    GenerateCandidates --> GetPrimaryRayReSTIR["GetPrimaryRay"]
    GenerateCandidates --> WRS
    WRS --> Generate
    WRS --> Target
    WRS --> PDF
    WRS --> ReservoirUpdate

    end

    classDef default fill:#DBEAFE,stroke:#2563EB,color:#1E3A8A,stroke-width:1.5px
    classDef diamond fill:#FEF3C7,stroke:#D97706,color:#92400E,stroke-width:1.5px
    classDef nee fill:#FECACA,stroke:#DC2626,color:#991B1B,stroke-width:1.5px
    classDef gradient fill:#DCFCE7,stroke:#16A34A,color:#166534,stroke-width:1.5px
    classDef restir fill:#F3E8FF,stroke:#9333EA,color:#6B21A8,stroke-width:1.5px

    class Jitter,DoF,AlphaTest,RR,Reconstruct,MissEnvMap,DirLight,SDReSTIR,Transient,Alias,Trans,Aniso,ShadowAlpha diamond

    class MissNEE,EmissionNEE,HitNEE,HitShiftNEE,SampleNEE,SampleLight,SampleLSD,BinarySearch,SampleEnvMap,SampleEnvMapCDF,EvalPunctual,EvalPointLight,EvalDistantLight,EvalSpotLight,SampleEmissive,SampleBary nee

    class GDPT,SampleGradients,SampleShiftedPath,TraceShiftedPath,HitShifted,ShiftReconnect,ShiftReconnectEnvironment,ShiftHalfVector gradient

    class SDReSTIR,SampleReservoir,ReSTIRGen,GenerateCandidates,CreateReservoir,WRS,Generate,Target,PDF,ReservoirUpdate restir
```