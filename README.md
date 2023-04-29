# Harmonic Virtual Lights
An implementation of the HVL algorithm for my bachelor thesis project.

# TODO:TODO:
* Ethics in report
* Double check polar/azimuthal angles and world-to-tangent-space transforms
* "TODO: fix edge case where normal == viewDir" in tangent space transform
* Use storage images instead of sampled images
* Use sinc window to try and avoid ringing artefacts
* "vkHarmonicsVirtualLights"*
* Fix barrier order to have RSM, shadow map and pre-depth overlap eachother
* Optimize smoothstep
* reduce normal format to R32G32

# Vulkan features used
* Version 1.3
* Dynamic rendering
* Synchronization 2
* Push descriptors

# Libraries and APIs used
* Dear ImGUI: GUI for debugging
* EnTT: entity components system
* fast_obj: OBJ mesh importing
* GLFW: window management
* GLM: math
* stb: image importing
* Vulkan: graphics and GPU management
* Vulkan Memory allocator: GPU memory management