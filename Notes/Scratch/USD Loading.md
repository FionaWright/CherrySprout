# USD Loading

New library SceneLoaderUSD
SceneLoaderUSD split into two parts, importing and post-processing

Importer uses the OpenUSD library to load all data into structs that suit the USD format. Separate vertex/index buffers, etc
Post-Processing processes the data into a format that suits Scene/Scene.h

SceneLoaderUSD builds into a DLL / Lib and places it in the build directory
Make sure to keep everything SceneLoader related separate from the main build stuff
add_subdirectory is fine (?)

CherrySprout will be in charge of moving the DLL next to the exe. And using LoadLibrary at runtime to access the interface 
