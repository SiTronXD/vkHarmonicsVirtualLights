# Harmonic Virtual Lights
An implementation of the HVL algorithm for my bachelor thesis project.

# TODO:
* Ethics in report
* Double check clamping range for half angle
* "TODO: fix edge case where normal == viewDir" in tangent space transform
* Use sinc window to try and avoid ringing artefacts
* "vkHarmonicsVirtualLights"*
* Fix barrier order to have RSM, shadow map and pre-depth overlap eachother
* Optimize smoothstep
* reduce normal format to R32G32
* Fix direct + indirect contribution relation
* Remove unused assets
* Consider removing spotlight cutoff, since HVLs are distributed in a square
* Enable/disable imgui

# Vulkan features used
* Version 1.3
* Dynamic rendering
* Synchronization 2
* Push descriptors

# Assets used
* MERL BRDF database: https://cdfg.csail.mit.edu/wojciech/brdfdatabase
* Code for efficient SH evaluation: https://jcgt.org/published/0002/02/06/
* HDR skybox (Grace Cathedral): https://www.pauldebevec.com/Probes/
* Sponza model: https://github.com/jimmiebergmann/Sponza
* Dragon model: http://graphics.stanford.edu/data/3Dscanrep/

# Libraries and APIs used
* Dear ImGUI: GUI for debugging
* EnTT: entity components system
* fast_obj: OBJ mesh importing
* GLFW: window management
* GLM: math
* stb: image importing
* Vulkan: graphics and GPU management
* Vulkan Memory allocator: GPU memory management