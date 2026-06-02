# Thoughts

This is not a production-ready graphics engine, don't get caught up in pointless side-tangents or over-engineering stuff
You don't need python bindings, just write to files and execute a py script from the C++ side

Try to keep debug classes in the app
Make sure client is turned into a lib which is linked to the app exe, rather than one simply owning the other
Do the same with the USD Loader? I can't have the USD Loader redownloading OpenUSD every time I regenerate the cmake

Materials, textures, etc will be bindless ONLY from now on

Avoid pointers like hell, arrays are the ways
Use unique_ptr always unless you can't

Try to come up with a better polymorphic system for HLSL, templates maybe?

Let's try and make most of the shaders C++ compilable, so I can run unit tests on them. Google test? 
Or maybe not, it's a lot of effort. Maybe if I get a hlsl glue library from elsewhere

No more wide strings anywhere. Make everything strings and they can be converted to wstrings for APIs that want them that way

Try to avoid GPU sync if I can? There must be some clever way to do it

Design everything path-tracer first, raster should be built around PT
Assume static meshes for everything, don't need to care about updating M, MTI
Path tracer should be a CS instead of a PS

Abstraction Idea: Buffer<T>, contains std::vector<T> and D12Resource. Has Load(T*) and Upload(D3D*) functions 
    Nah bad abstraction, saves little effort and hides a lot