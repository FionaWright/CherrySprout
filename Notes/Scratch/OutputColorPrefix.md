# Output Color System "Prefix" Update

A new stack containing prefixes is kept  
Prefixes are mapped to strings which will be displayed by the GUI (Use similar to DebugID.h)  
Push prefixes before outputting values, pop them afterwards  

```cpp
DBG_PUSH_PREFIX(PREFIX_GD); // Gradient Domain
bool isMiss;
HitInfo hitInfo;
ComputeRayHit(q, pixelCoord, pathState, isMiss, hitInfo);
DBG_POP_PREFIX();
```

Whenever a push/pop occurs, a global timestamp value gets incremented. This is used to keep track of the order values get assigned  
    Note: May be better to just have static order since it'll make it easier to find stuff  
Both the stack and timestamp value are dumped during path dumping  

The GUI is now formatted as collapsable categories

Values:
    HitInfo:
        TexAlbedo
        Roughness
        ...
    BxDF:
        D
        F
        PDF
        ...
    NEE:
        PDF
        BxDF:
            D
            F
            PDF
            ...
    GradientDomain:
        HitInfo:
            TexAlbedo
            Roughness
            ...
        BxDF:
            D
            F
            PDF
            ...