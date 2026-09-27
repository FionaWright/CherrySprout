# CherrySprout

by Fiona Wright

## Info

D3D12 Path-Tracer Research Engine  

Using CMAKE, C++, HLSL and Python  

Created for my bachelors final year project at Trinity College Dublin (Due April 2027)  

Plan is to combine Gradient-Domain Path-Tracing with ReSTIR PT and Path-Guiding 

## Features

### Engine

- USD Scene Loader
- MaterialX Support
- NVIDIA MDL Material Support
- Shader Hot Reloading
- Scene Config System
- Window Resizing

### Path-Tracer

- Determinstic
- Jitter
- Alpha Testing
- Rotatable EA Environment Map
- Depth of Field
- Russian Roulette
- Firefly Threshold
- Transmission
- Anisotropy
- GBuffer Pre-Pass for Primary Ray Reconstruction
- Transient Rendering + Video Generator
- Abstracted BxDF System
- Principled BSDF
- Modular Microfacet Model System (Smith, VCavity, VNDF, Anisotropic, etc) 
- Environment Map + Punctual Light NEE with MIS 
- Alias Tables for fast light sampling
- Modular Feature Flags System
- ReSTIR DI
- Gradient Domain Path-Tracing using Jacobi-Iterator Screened Poisson Reconstruction

### Debug Tools

- Forward Render Backend
- Snapshot Tool With Clipboard Pasting
- Shader Assertion System
- Output Color System + Remaps
- Intensity Scales System
- Path Dumper System
- BxDF Sample/Eval Tests
- Furnance Test
- Gizmos
- Image Comparison / Convergence Testing Tool
- Python Plotting using Bokeh

## Third Party Dependencies 

- D3D12
- DirectXTex
- Dear ImGui
- OpenUSD
- WinPixEventRuntime
- ffmpeg

### Python

- Bokeh
- Scipy
- Numpy

## Images

<img width="1700" height="575" alt="image" src="https://github.com/user-attachments/assets/8a0d7ad7-c78a-47a3-b922-b19ee4664f28" />

<img width="1695" height="575" alt="image" src="https://github.com/user-attachments/assets/ea6c82aa-8688-482e-b675-43a2b43c2379" />

<img width="800" height="451" alt="ezgif-72583bea7bed94b4" src="https://github.com/user-attachments/assets/f2a31136-9b66-4014-a4b7-8be031176cfa" />





