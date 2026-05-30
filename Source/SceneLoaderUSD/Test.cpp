#include "SceneLoaderUSD.h"
#include "Scene/SceneCPU.h"

#include <iostream>
#include <ostream>

int main(const int argc, char** argv)
{
    SceneCPU scene;
    SceneLoaderUSD::LoadUSD("Test", &scene);

    for (int i = 0; i < scene.TextureFilepaths.size(); i++)
        std::cout << scene.TextureFilepaths[i] << std::endl;
}