# FirstDX12Renderer
This project wants to mainly showcase two topics:
- **Base CMake Project Configuration**
  - Part1 and Part2 are target executables. These targets have dependencies on defined target libraries (both internal and external).
  - Part1 has the simplest configuration, while Part2 links against a wider range of libraries.
  - You can read my [CMake Configuration Article](https://logins.github.io/programming/2020/05/17/CMakeInVisualStudio.html).
- **DX12-powered Render Window**
  - Simplest example on how to use Direct3D 12 for real-time graphics applications (e.g. videogames).
  - Part1 executable project is the simplest, and it will show to instatiate and run a resizable D3D12 Window.
  - Part2 executable target will render a cube that can be rotated and moved upon mouse interaction.
  - Part3 executable target will perform the same as Part2 but with a full graphics abstraction layer.


>Note: This repo was tested in VisualStudio 2019 using FolderView Mode and CMake (Ninja generator + msvc compiler).

>Note: Part of D3D12 code was heavily inspired to the 3DGEP DirectX12 guides, by Jeremiah van Oosten, [you can find them by clicking here](https://www.3dgep.com/learning-directx-12-1/). He also has a corresponding GitHub repo, that [can be found here](https://github.com/jpvanoosten/LearningDirectX12/tree/v0.0.1).

>Note: Code you will find is solely my own and does not express the views or opinions of my employer.
