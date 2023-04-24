# Harmonic Virtual Lights
An implementation of the HVL algorithm for my bachelor thesis project.

# TODO:TODO:
* HVL/HVLs distinction in report
* Examples of anisotropic/isotropic materials in report
* Double check polar/azimuthal angles and world-to-tangent-space transforms
* "TODO: fix edge case where normal == viewDir" in tangent space transform
* NUM_ANGLES should be 90 in the final version
* Try unrolling associated legendre polynomials

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