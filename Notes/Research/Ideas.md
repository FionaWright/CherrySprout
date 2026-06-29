# Ideas

## Neural Network

Use a NN to optimize the ReSTIR parameters, based on GBuffers / scene metadata

## ReSTIR Lobe Frames

Every couple of frames, just have all paths use the same lobe (Diffuse, specular, light sampling). Would this be useful? Can add the information to the candidates and then if the paths were good they will spread throughout the reservoirs. Probably dumb idea but I understand ReSTIR little enough that it might be genius...

## Borders

ReSTIR seems to struggle with pixels that have no previous temporal history. Could be improved by adding a border of pixels around what is rendered, that is not shown to the user/player. Or maybe we can use the GBuffer to create a very rough estimate for these new pixels, so they don't need to start from scratch