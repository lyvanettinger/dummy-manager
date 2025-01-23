# DiaBolic

A barebones project I made to learn basic DX12 (and eventually font rendering) with. It has set out to become a bit of a framework along the way.
Screenshot of the current state:
![image](https://github.com/user-attachments/assets/17f92192-7620-4588-a973-a33b3fa31be5)

#### It features:
- A complete bindless model (for buffers + textures)
- DXC shader compilation
- Basic wrappers for command queues and descriptor heaps

#### Future plans:
- Basic font rendering

## How to build

- Build project with CMake
- Run Setup.bat

If the compiler is complaining about git submodules, run
`git submodule update --init --recursive`.

## Learning resources
Some of the most notable learning resources and references used:
- https://www.3dgep.com/category/graphics-programming/directx/
- https://rtarun9.github.io/blogs/bindless_rendering/ and their https://github.com/rtarun9/Helios/tree/master
