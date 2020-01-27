# Introduction To DirectX Raytracing

A barebones application to get you jump started with DirectX Raytracing (DXR)! Unlike other tutorials, this sample code _does not create or use any abstractions_ on top of the DXR Host API, and focuses on highlighting exactly what is new and different with DXR using the raw API calls. 

The code sample traces primary rays and includes a simple hit shader that samples a texture. 

![Release Mode](https://github.com/acmarrs/IntroToDXR/blob/master/IntroToDXR.png "Release Mode Output")

## Requirements

* Windows 10 v1809, "October 2018 Update" (RS5) or later
* Windows 10 SDK v1809 (10.0.17763.0) or later. [Download it here.](https://developer.microsoft.com/en-us/windows/downloads/sdk-archive) 
* Visual Studio 2017, 2019, or VS Code

## Code Organization

Data is passed through the application using structs. These structs are defined in `Structures.h` and are organized into three categories: 

* Global
* Standard D3D12
* DXR


Rendering code lives in `Graphics.h/cpp` and is broken into four namespaces to separate new DXR functionality from existing D3D12 functionality. 

### D3DResources
```c++
namespace D3DResources 
{
	void Create_Buffer(D3D12Global &d3d, D3D12BufferCreateInfo &info, ID3D12Resource** ppResource);
	void Create_Texture(D3D12Global &d3d, D3D12Resources &resources, Material &material);
	void Create_Vertex_Buffer(D3D12Global &d3d, D3D12Resources &resources, Model &model);
	void Create_Index_Buffer(D3D12Global &d3d, D3D12Resources &resources, Model &model);
	...
}
```
Contains common functions to create D3D12 resources like (vertex, index, constant) buffers, textures, samplers, and heaps.  


### D3DShaders
```c++
namespace D3DShaders 
{
	void Init_Shader_Compiler(D3D12ShaderCompilerInfo &shaderCompiler);
	void Compile_Shader(D3D12ShaderCompilerInfo &compilerInfo, RtProgram &program);
	...
}
```
Contains functions to initialize the DXCompiler as well as load and compile shaders (including - of course - the new DXR ray tracing shaders).

### D3D12
```c++
namespace D3D12 
{	
	void Create_Device(D3D12Global &d3d);
	void Create_CommandList(D3D12Global &d3d);
	...
}
```
Contains functions for common D3D12 setup operations like Device, Command List, Command Queue, Command Allocator, Fence, Swap Chain, and Root Signature creation. Note that small differences exist in Device and Command List creation when using DXR.

### DXR
```c++
namespace DXR
{
	void Create_Bottom_Level_AS(D3D12Global &d3d, DXRGlobal &dxr, D3D12Resources &resources, Model &model);
	void Create_Top_Level_AS(D3D12Global &d3d, DXRGlobal &dxr, D3D12Resources &resources);
	void Create_RayGen_Program(D3D12Global &d3d, DXRGlobal &dxr, D3D12ShaderCompilerInfo &shaderCompiler);
	void Create_Miss_Program(D3D12Global &d3d, DXRGlobal &dxr, D3D12ShaderCompilerInfo &shaderCompiler);
	void Create_Closest_Hit_Program(D3D12Global &d3d, DXRGlobal &dxr, D3D12ShaderCompilerInfo &shaderCompiler);
	void Create_Pipeline_State_Object(D3D12Global &d3d, DXRGlobal &dxr);
	void Create_Shader_Table(D3D12Global &d3d, DXRGlobal &dxr, D3D12Resources &resources);	
	...
}
```
Contains new functionality specific to DirectX Raytracing. This includes acceleration structure creation, shader table creation and update, ray tracing pipeline state object (RTPSO) creation, and ray tracing shader loading and compilation. 

## Command Line Arguments

* `-width [integer]` specifies the width (in pixels) of the rendering window
* `-height [integer]` specifies the height (in pixels) of the rendering window
* `-vsync [0|1]` specifies whether vsync is enabled or disabled
* `-model [path]` specifies the file path to a OBJ model

## Suggested Exercises
After building and running the code, first thing I recommend you do is load up the Nsight Graphics project file (IntroToDXR.nsight-gfxproj), and capture a frame of the application running. This will provide a clear view of exactly what is happening as the application is running. [You can download Nsight Graphics here](https://developer.nvidia.com/nsight-graphics).

Once you have a good understanding of how the application works, I encourage you to dig deeper into DXR by removing limitations of the current code and adding new rendering features. I suggest:

* Add antialiasing by casting multiple rays per pixel.
* Add loading and rendering of models with multiple materials (only a single material is supported now)
* Add realistic lighting and shading (lighting is currently baked!)
* Add ray traced shadows. _Extra credit:_ use Any-Hit Shaders for shadow rendering
* Add ray traced ambient occlusion.
* Add ray traced reflections for mirror-like materials.
* Add camera translation and rotation mapped to keyboard and mouse inputs.

If you've implemented the above features, you'll be just a few steps away from your very own DXR path tracer!

* Implement [Ray Tracing In One Weekend](https://www.amazon.com/Ray-Tracing-Weekend-Minibooks-Book-ebook/dp/B01B5AODD8/ref=sr_1_1?ie=UTF8&qid=1540494705&sr=8-1&keywords=ray+tracing+in+one+weekend)

## Licenses and Open Source Software

The code uses two dependencies:
* [TinyObjLoader](https://github.com/syoyo/tinyobjloader-c/blob/master/README.md), provided with an MIT license. 
* [stb_image.h](https://github.com/nothings/stb/blob/master/stb_image.h), provided with an MIT license.

The repository includes assets for use when testing the renderer:
* [Statue Image](https://pixabay.com/en/statue-sculpture-figure-1275469/), by Michael Gaida, licensed under a [CC0 1.0 Creative Commons Universal Public Domain Dedication License](https://creativecommons.org/publicdomain/zero/1.0/deed.en). 
* [Peerless Magnarc Cinema Projectors](https://sketchfab.com/models/62046af7d4f84b4ebe01d44f54970bc1), by Miguel Bandera, licensed under a [Creative Commons Attribution 4.0 International License](https://creativecommons.org/licenses/by/4.0/). 

