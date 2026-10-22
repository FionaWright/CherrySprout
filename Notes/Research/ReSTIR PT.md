# ReSTIR PT

Given all subpaths from NEE / BXDF / PG sampling generated within a single path sample, candidate samples are a single selected subpath

If you let light vertices only be sampled by NEE, then the ReSTIR MIS weight is 1

Given path sample of depth $D$, vertices $\{x_0, x_1, x_2, \dots, x_{D-1}\}$ 

NEE subpath is $\{x_0, x_1, \dots x_k\}$ 

Target function is just contribution

If BxDF can sample lights, now both can be used as candidate paths. Each $m_i(x_i)$ accounts for the other sampling technique of the same depth

I think the main difference between GI and PT is that GI only stores the next vertex, and if you fail to connect then give up
Whereas PT stores a vertex list, and the path traces a full shifted with reconnection states?

## GDPT Shift Mapping

Identity shift?
An area measure jacobian

$T[x_0, x_1, x_2, \dots, x_{D-1}] = [y_0, y_1, x_2, \dots, x_{D-1}]$ 

Looks a lot like the reconnection shift but idk
Should be ReconnectVis()

Can use the same shift mappings from GDPT

NOTE: It mentions that when v1, v2 are diffuse and you are ready to do a reconnection shift. The current y vertex CANNOT be specular otherwise the shift fails. Does this mean I need to detect whether the vertex is glossy before shift mapping? Surely that doesn't involve the BxDF so maybe just check roughness/metallic

GRIS paper has lots of info on local shift mappings apparently. There's also global methods like manifold exploration which solve specular chains

Also see "A survey on gradient domain rendering 2019" 

## Modified ReSTIR Shift Mapping (Hybrid Shift)

Similarly postpones reconnection as GDPT SM

Random-Replay Shift = Copy the rng state at each bounce and re-trace the next bounce with the method used by the base path

Precomputes the reconnection vertex on the base path and uses random replay to shift earlier path segments so that the base path does not have to be stored 

Uses additional distance condition similar to Manzi et al 2014, to avoid creating short reconnection segments 

Classifies a vertex using ONLY roughness of the sampled lobe, optimizing resampling on multi-layer materials 

Requires constant storage per pixel, only a reconnection vertex and an RNG generating seed in addition to other reservoir data 

This can significantly improve the quality of glossy/refractive materials compared to reconnection shift 

### Ensuring Invertibility

To be connectible, a reconnection vertex needs to satisfy two conditions:

Distance Condition:
```cpp
float dX = LengthSquared(x2 - x1);
float dY = LengthSquared(x2 - y1);
if (dX >= D_THRESHOLD && dY >= D_THRESHOLD)
{
    Reconnect();
}
```

Roughness Condition:
```cpp
float alpha1 = Lobe(x1).Alpha;
float alpha2 = Lobe(x2).Alpha;
float alphaY = Lobe(y1).Alpha;
if (alpha1 >= ALPHA_THRESHOLD && 
    alpha2 >= ALPHA_THRESHOLD && 
    alphaY >= ALPHA_THRESHOLD)
{
    Reconnect();
}
```

If a vertex contains multiple lobes (If it samples lights for NEE and evaluates all lobes) then pick the lobe that maximizes the alpha

When tracing the base path, the first vertex (That isn't the primary vertex) that satisfies both conditions is marked as the reconnection vertex

When the shifted path runs, the offset path is generated using random replay until it reaches the reconnection vertex index. Then it checks both conditions 

I'd recommend trying to implement hybrid shift into GDPT before starting ReSTIR PT