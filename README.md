# Harmonic Virtual Lights
An implementation of the HVL algorithm for my bachelor thesis project.

# TODO:

# Done:
* Resource manager for meshes and textures
* Scene management
* Dynamic rendering
* Push descriptors
* Utilize synchronization 2 for pipeline barriers
* Use EnTT
* PBR (direct light)
* Post-process pass for tonemapping + gamma correction
* Basic material system for pipelines
* HDR environment map
* Load cube map from single texture
* PBR (indirect specular from IBL)
* Use VMA
* Refactor (skybox, brdf lut and prefilter creation, texture2D/cube, pbr textures)
* Move project to it's own repo

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