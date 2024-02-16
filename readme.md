# Real-time Interactive GPGPU 3D Fluid Simulation
## Overview
This project is an interactive 3D fluid simulation with ray-traced reflection and refraction, where the underlying differential equations are solved numerically on the GPU. The quantities (velocity, pressure, distance from surface) are stored in 3D textures, which are modified at each time step by rendering to each 'slice' of the 3D textures using a fragment shader.

Essentially, this is my version of the [Nvidia 'Box of Smoke' demo](https://www.nvidia.com/en-gb/geforce/community/demos/) as described in [GPU Gems 3](https://developer.nvidia.com/gpugems/gpugems3/part-v-physics-simulation/chapter-30-real-time-simulation-and-rendering-3d-fluids), based on the methodology of Stam, Jos. 1999. *"Stable Fluids."* In Proceedings of SIGGRAPH 99, *pp. 121–128*.


![Fluid simulation preview](https://www.wjgrace.co.uk/images/fluid_thumbnail.gif) 


This was a fairly challenging choice for my second OpenGL-based project, not least because OpenGL is very good at crashing for seemingly no reason. However, the final results were pretty satisfying and I'm pleased I persisted. Performance is 'good enough' (40-50 FPS on my laptop), allowing for real-time interaction with the mouse, but frequently switching renderbuffers to render to the texture slices seems to bottleneck the speed. As such, if I were to write another GPU fluid renderer, I would probably use compute shaders or CUDA/OpenCL instead.

The window, context, app, shader program, texture, text rendering and GUI classes are re-used (with modification) from [wjgra/asteroids-opengl](https://github.com/wjgra/asteroids-opengl).

[RenderDoc](https://renderdoc.org/) was helpful for profiling.

### Issues
~~As it stands, `FluidRenderer` is a pretty large monolithic class. Whilst it does make use of nested subclasses, I think refactoring it to break it down into smaller components would be a good exercise.~~ **Update 16/02/2024:** This has been refactored to use the [Model-View-Controller pattern](https://en.wikipedia.org/wiki/Model%E2%80%93view%E2%80%93controller), with the eponynomous roles fulfilled by the classes `FluidSimulator`, `FluidRenderer` and `Fluid` respectively. I also tidied up the naming conventions/const-correctness and added the ability to rotate the scene using the arrow keys or WASD.

There is also an issue with odd-even decoupling, which you can see (if you look carefully at the GIF above) as a 16x16 grid of periodic oscillations when the fluid surface is near-flat. This is caused by using collocated grids for the simulation, together with a second-order simulation kernel (which skips every other cell). This error is present in the original Nvidia demo, but it would be nice to eliminate it. There are various solutions 'known to the art', but not all are simple to implement.

## Dependencies and Compilation
This project uses SDL for window creation and input handling, and OpenGL for rendering. [Glad](https://glad.dav1d.de/) is used for loading OpenGL API functions.

Sample GCC compilation command

```
g++ src\*.cpp src\*.c -o fluid.exe -W -Wall -Wextra -pedantic -I "C:\w64devkit\include" -I "C:\SDL-release-2.26.4\include" -I "include" -lopengl32 -lglu32 -pthread "SDL2.dll" -O3 -DNDEBUG
```
I am yet to port this project to Emscripten. I believe there are issues with rendering to scalar (i.e. non-RGB) textures in OpenGL ES 2.0 (which is used by Emscripten). Since this is used extensively in this project to improve performance, porting could be tricky. It would be good to investigate this in more detail at some point.
