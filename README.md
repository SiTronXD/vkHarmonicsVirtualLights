# Harmonic Virtual Lights
An implementation of the HVL algorithm for my bachelor thesis project.

# TODO:
* Ethics in report
* Double check clamping range for half angle
* "TODO: fix edge case where normal == viewDir" in tangent space transform
* Use storage images instead of sampled images
* Use sinc window to try and avoid ringing artefacts
* "vkHarmonicsVirtualLights"*
* Fix barrier order to have RSM, shadow map and pre-depth overlap eachother
* Optimize smoothstep
* reduce normal format to R32G32
* Fix direct + indirect contribution relation
* Remove unused assets

# Vulkan features used
* Version 1.3
* Dynamic rendering
* Synchronization 2
* Push descriptors

# Assets used
* HDR skybox (Grace Cathedral): https://www.pauldebevec.com/Probes/
* Sponza model: https://github.com/jimmiebergmann/Sponza
* MERL BRDF database: https://cdfg.csail.mit.edu/wojciech/brdfdatabase

# Libraries and APIs used
* Dear ImGUI: GUI for debugging
* EnTT: entity components system
* fast_obj: OBJ mesh importing
* GLFW: window management
* GLM: math
* stb: image importing
* Vulkan: graphics and GPU management
* Vulkan Memory allocator: GPU memory management