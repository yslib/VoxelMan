# VoxelMan

T**he project is still under developement.**

A voxel editor aimed at editing enormous and dense volume data base on an out-of-core technique.


**Installation:**
----
### Requirements:
1. **Git**
2. **CMake**
3. **C++17 or heigher**

### Dependences:
The only external dependency is **[GLFW][2]**. 

Besides, it has dependent libraries **[VMCore][1]**,**[VMUtils][2]** and **[VMat][3]**, which are automatically installed by the cmake scripts
### Windows:
**[Vcpkg][5] is highly recommanded to install the GLFW**. 

```
git clone https://github.com/yslib/VoxelMan.git --recursive
```
The project could be opened in **VS2019** or **VS2017** by **Open Folder** and compiled 

### Linux:
You can use any package manager you like to install **GLFW**.
After install the external dependences:

``` 
git clone https://github.com/yslib/VoxelMan.git --recursive
cd VoxelMan
mkdir build
cd build
cmake ..
```


### macOS:
---
OpenGL is deprecated by Apple long ago. It has no latest version this project rely on.
Moreover, the performence of OpenGL on macOS is horrible. Forget it on macOS though it could be compiled successfully theratically as what to be done on Linux.

### Others:
---

[1]: https://github.com/cad420/VMCore
[2]: https://github.com/cad420/VMUtils
[3]: https://github.com/cad420/VMat

[4]: https://github.com/glfw/glfw
[5]: https://github.com/microsoft/vcpkg