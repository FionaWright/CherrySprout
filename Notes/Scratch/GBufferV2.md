# GBuffer V2

Make sure to include MSAA and mip maps! 
Also make sure to have all PT flags set for restir, and use alias tables for LSD 

Run animation pass to update vertex buffer + RTAS

Sort objects and perform frustum culling

Run gbuffer pass (With per-frame jitter) to get:
- Mat Idx
- UV
- MV
- Normals
- Depth
- Emission

Blur emission with 2-pass gaussian to get bloom

Comput SSAO using normal and depth

Execute ReSTIR DI using gbuffers (Run ReSTIR manager) 
or
Run shadow map passes to compute shadow buffers

Execute ReSTIR GI as well for reflections/etc

Compute final lighting 

Denoise using mild A-Trous filter, try and have some semblance of temporal accumulation

## TODO

- [x] Set up deferred rendering
- [ ] Sorting objects
- [ ] Frustum Culling
- [ ] Bloom
- [ ] SSAO
- [ ] Update MVs for animation
- [ ] ReSTIR DI
- [ ] ReSTIR GI

## Possible optimizations if required

Hardcode weights in gaussian filter or do that clever trick where you use the bilinear sampler