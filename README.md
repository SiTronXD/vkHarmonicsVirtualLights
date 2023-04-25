# Harmonic Virtual Lights
An implementation of the HVL algorithm for my bachelor thesis project.

# TODO:TODO:
* HVL/HVLs distinction in report
* Examples of anisotropic/isotropic materials in report
* Double check polar/azimuthal angles and world-to-tangent-space transforms
* "TODO: fix edge case where normal == viewDir" in tangent space transform
* Try unrolling associated legendre polynomials
* Optimize factorials
* Remove repeating terms (specifically in getCoeffL)
* Direct lighting and spotlight fade at edges
* Use storage images instead of sampled images

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