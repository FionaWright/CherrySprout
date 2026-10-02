#include "Utils/SharedUtils.h"
#include "Utils/CBVs.h"

#include "Scene/Vertex.h"
#include "Scene/Material.h"
#include "Scene/InstanceData.h"

StructuredBuffer<InstanceData> gMegaBufferInstances : register(t0);
StructuredBuffer<Material> gMegaBufferMaterials : register(t1);
StructuredBuffer<Vertex> gMegaBufferVertex : register(t2);
StructuredBuffer<uint3> gMegaBufferIndex : register(t3);

RWStructuredBuffer<uint> gEmissiveInstanceToInstanceMap  : register(u0);

bool IsEmissiveInstance(InstanceData instance)
{
    Material material = gMegaBufferMaterials[instance.MaterialIndex];

    float3 emission = material.EmissiveColor * material.EmissiveStrength;

    return any(emission > 0);
}

[numthreads(1,1,1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    uint count;
    uint stride;
    gMegaBufferInstances.GetDimensions(count, stride);

    for (uint i = 0; i < count; i++)
    {
        uint emissiveInstanceCount = 0;

        InstanceData instance = gMegaBufferInstances[i];

        if (IsEmissiveInstance(instance))
        {
            gEmissiveInstanceToInstanceMap[emissiveInstanceCount] = i;
        }
    }
}
