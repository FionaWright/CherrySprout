# CherrySprout

by Fiona Wright

## Info

D3D12 Path-Tracer Research Engine  

Using CMAKE, C++, HLSL and Python  

## Features

### Engine

- USD Scene Loader
- MaterialX Support
- NVIDIA MDL Material Support
- Shader Hot Reloading
- Scene Config System
- Engine built as library which applications can use
- Window Resizing
- MSAA
- Mip Map Support
- Forward/Reverse Z Depth Buffer Options
- Environment Map Switching

### Path-Tracer

- Determinstic
- Jitter (Per-Frame or Per-Pixel)
- Alpha Testing w/ Shadow Ray Support
- Rotatable EA Environment Map
- Depth of Field
- Russian Roulette
- Firefly Threshold
- Transmission
- Anisotropy
- GBuffer Pre-Pass for Primary Ray Reconstruction / Gizmos
- Transient Rendering + Video Generator
- Abstracted BxDF System
- Principled BSDF
- Modular Microfacet Model System (Smith, VCavity, VNDF, Anisotropic, etc) 
- NEE/MIS for Environment Maps, Punctual Lights & Area Lights
- Alias Tables for performant light sampling
- Modular Feature Flags System
- ReSTIR DI & GI with Spatio-Temporal reuse
- Gradient Domain Path-Tracing using Jacobi-Iterator Screened Poisson Reconstruction

### Debug Tools

- Forward Render Backend
- Snapshot Tool With Clipboard Pasting
- Shader Assertion System
- Output Color System + Remaps
- Intensity Scales System
- NaN / INF value checking
- Path Dumper System
- BxDF & NEE Re-Evaluation Tests
- Furnance Tests
- Gizmos
- Image Comparison / Convergence Testing Tool
- Python Plotting using Bokeh
- Screen-Space Gradients for GDPT debugging
- Feature flag "Quick-Switch" mode for fast runtime use

## Third Party Dependencies 

- D3D12
- DirectXTex
- Dear ImGui
- OpenUSD
- WinPixEventRuntime
- ffmpeg
- tinyexr

### Python

- Bokeh
- Scipy
- Numpy

## Images

<img width="1700" height="575" alt="image" src="https://github.com/user-attachments/assets/8a0d7ad7-c78a-47a3-b922-b19ee4664f28" />

<img width="1695" height="575" alt="image" src="https://github.com/user-attachments/assets/ea6c82aa-8688-482e-b675-43a2b43c2379" />

<img width="800" height="451" alt="ezgif-72583bea7bed94b4" src="https://github.com/user-attachments/assets/f2a31136-9b66-4014-a4b7-8be031176cfa" />

<img width="1600" height="542" alt="image" src="https://github.com/user-attachments/assets/104a4e95-0440-4667-8468-8fce20bf7326" />

<img width="1600" height="542" alt="image" src="https://github.com/user-attachments/assets/dd196a30-fa1e-4976-99ba-a881bcbb0d8a" />

<img width="1600" height="542" alt="image" src="https://github.com/user-attachments/assets/92a387ed-dfc8-4a3b-8b57-d2ced4fc6bb1" />

<img width="1600" height="542" alt="image" src="https://github.com/user-attachments/assets/ac80a68d-c462-4f89-9b88-df8d7abe71d4" />

<img width="1600" height="542" alt="image" src="https://github.com/user-attachments/assets/f5d5c47b-9d78-4c38-a395-18905ce29164" />





