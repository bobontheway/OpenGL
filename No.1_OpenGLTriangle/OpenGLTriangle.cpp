#include <stdio.h>
#include <stdlib.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

static void printGLString(const char *name, GLenum s) {
	const char *v = (const char *)glGetString(s);
	fprintf(stderr, "gl %s(0x%x) = %s\n", name, s, v);
}

static void dumpGLInfo(void)
{
	printGLString("Version", GL_VERSION);
	printGLString("Vendor", GL_VENDOR);
	printGLString("Renderer", GL_RENDERER);
	printGLString("Extensions", GL_EXTENSIONS);
}

//int main(int argc, char** argv)
int main(void)
{
	dumpGLInfo();
	return 0;
}

