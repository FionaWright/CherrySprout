# README

This flags system is designed to minimize the redundancy in adding/removing/changing flags

ListFeature.h and ListDebug.h are all you need to change to do so

Processing.h processes those two files in different ways depending on defines
Flags.h is in charge of setting up those defines to generate the proper enums/vectors

MethodsCpp.h is for the C++ files to work with the flags
MethodsHlsl.hlsli is for the HLSL files to work with the flags