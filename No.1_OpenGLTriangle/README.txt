1.在 main 函数中直接调用 dumpGLInfo 来输出 OpenGL 信息，错误。相关信息为空；
需要初始化 EGL？
- 只有在执行了 eglMakeCurrent 之后，才能通过 dumpGLInfo 获取 OpenGL 的信息。

2.不添加下面的代码行会出现编译出错
using namespace android;
