# TODO Setup Advanced

Setup stuff that is not required to begin work on the core stuff, but still needs to be done eventually

- [x] PBR BSDF from CherryPip
- [x] Environment maps
- [x] Glass
- [ ] Denoising

- [x] Hot reloading
- [x] Root constants support
- [ ] Run path-tracer without sync?
- [x] Scene switching in GUI
- [ ] Profiling (Tracy?)
- [ ] Snapshot tool (Can I make it go into my clipboard as well?)

- [ ] Python executor
- [ ] Path visualizer
- [x] Debug Info Output (Make it better)
- [ ] RMSE Tester
- [ ] PT Assert based system inspired by Viggo
- [ ] PT meta counter debug increment/F4/etc system
- [ ] Furnace Test
- [ ] BxDF Test

## BxDF Test Explained

if BXDF_TEST enabled then flip the screen symmetrically across x=0.5 
Path samples on the left side will sample a random uniform (hemi)sphere and the BxDF evaluate function
Path samples on the right will use the BxDF sample function
After each frame, readback the data and run a RMSE compute shader that compares the left and right side of the screen
Log the RMSE in the GUI. If the RMSE doesn't reach 0 then there are bugs