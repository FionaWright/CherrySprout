# Ideas

## Neural Network

Use a NN to optimize the ReSTIR parameters, based on GBuffers / scene metadata

## ReSTIR Lobe Frames

Every couple of frames, just have all paths use the same lobe (Diffuse, specular, light sampling). Would this be useful? Can add the information to the candidates and then if the paths were good they will spread throughout the reservoirs. Probably dumb idea but I understand ReSTIR little enough that it might be genius...