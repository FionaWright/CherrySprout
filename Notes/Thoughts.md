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

No more wide strings anywhere. Make everything strings and they can be converted to wstrings for APIs that want them that way

Path tracer should be a CS instead of a PS

Try to avoid GPU sync if I can? There must be some clever way to do it