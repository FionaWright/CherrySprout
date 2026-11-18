# Animation Plan

## Buffers

KeyFrameMegaBuffer (GPU)
- Contains chunks of keyframes per bone per animation clip per object
- Assumes keyframes will always be evenly spaced (KeyFrameIndex = KeyFrameCount * floor(Time * Duration))

KeyFrameMegaBuffer:
    Object 0:
        Clip 0:
            KeyFrame 0:
                `float Time`
                Bone 0:
                    `affine3x4 BoneTransform` 
                Bone 1:
                    `affine3x4 BoneTransform` 
                ...
            KeyFrame 1:
                `float Time`
                Bone 0:
                    `affine3x4 BoneTransform` 
                Bone 1:
                    `affine3x4 BoneTransform` 
                ...
            ...
        ...
    ...

AnimationClipMegaBuffer (GPU)
- Contains information about the keyframes
- In my engine, I'm only going to test for single animation clips per object but I'll structure things properly 

AnimationClipMegaBuffer:
    Object 0:
        Clip 0:
            `float Duration`
            `uint KeyFrameMegaBufferOffset` 
            `uint KeyFrameMegaBufferCount` 

BoneInverseBindMegaBuffer (GPU)
- Contains chunks of inverse bind transforms per bone per object

BoneInverseBindMegaBuffer:
    Object 0:
        Bone 0:
            `affine3x4 InverseBind`
        Bone 1:
            `affine3x4 InverseBind`
        ...
    ...

BoneTransformMegaBuffer (GPU)
- Contains the bone transforms for the current frame
- Post-interpolation and post-multiplication by the inverse bind transforms
  
BoneTransformMegaBuffer:
    Object 0:
        Bone 0:
            `affine3x4 BoneTransform`
        Bone 1:
            `affine3x4 BoneTransform`
        ...
    ...

VertexBoneIndicesMegaBuffer (GPU)
- For each vertex, contains bone indices and weights (4 max)
- Bone indices will be local (0 is the first bone in the object)

VertexBoneIndicesMegaBuffer:
    Object 0:
        Vertex 0:
            `uint BoneIndicesLocal[4]`
            `float BoneWeights[4]` 
        ...
    ...

VertexBindMegaBuffer (GPU)
- Rename of the original vertex buffer
- Stores the bind pose / static vertices

VertexSkinnedMegaBuffer (GPU)
- Contains the vertex buffer post-animation
- Any non-animated objects have their vertices copied to this buffer on scene load and are not modifed after

VertexSkinnedMegaBuffer:
    Object 0:
        Skinned Vertex 0
        Skinned Vertex 1
        ...
    ...

Each object now contains:
    `uint BoneCount`
    `uint BoneTransformMegaBufferOffset`
    `uint AnimationClipMegaBufferOffset`


## Passes

### Scene Load

### Per Frame

For each object:
    For each bone:
        For the current time, interpolate between the keyframes and multiply it by the inverse bind transform
        Store in BoneTransformsMegaBuffer

For each object:
    For each vertex:
        Get the original vertex from the VertexBindMegaBuffer
        For each bone index:
            Get the bone transform from BoneTransformsMegaBuffer
            Transform the vertex 
        Update VertexSkinnedMegaBuffer