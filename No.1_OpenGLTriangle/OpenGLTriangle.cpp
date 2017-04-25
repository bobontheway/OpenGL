#include <stdio.h>
#include <stdlib.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <gui/SurfaceControl.h>

#include <gui/SurfaceComposerClient.h>
#include <gui/ISurfaceComposer.h>
#include <gui/Surface.h>
#include <ui/DisplayInfo.h>

using namespace android;

static void checkEglError(const char *op)
{
	for (EGLint error = eglGetError(); error != EGL_SUCCESS;
		error = eglGetError()) {
		fprintf(stderr, "after %s() eglError 0x%x\n", op, error);
	}
}

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

//EGLNativeWindowType initSurface(void)
static void initSurface(void)
{
	EGLBoolean ret;
	status_t err;

	printf("====> initSurface\n");

	sp<SurfaceComposerClient> client = new SurfaceComposerClient;
	err = client->initCheck();
	if (err != NO_ERROR) {
		fprintf(stderr, "SurfaceComposerClient initCheck error:0x%x\n", err);
		return;
	}

	// Get main display parameters
	sp<IBinder> mainDisplay = SurfaceComposerClient::getBuiltInDisplay(
		ISurfaceComposer::eDisplayIdMain);

	DisplayInfo mainDisplayInfo;
	err = SurfaceComposerClient::getDisplayInfo(mainDisplay, &mainDisplayInfo);
	if (err != NO_ERROR) {
		fprintf(stderr, "unable to get display characteristics: 0x%x\n", err);
		return;
	}

	uint32_t width, height;
	if (mainDisplayInfo.orientation != DISPLAY_ORIENTATION_0 &&
		mainDisplayInfo.orientation != DISPLAY_ORIENTATION_180) {
		// rotated
		width = mainDisplayInfo.h;
		height = mainDisplayInfo.w;
	} else {
		width = mainDisplayInfo.w;
		height = mainDisplayInfo.h;
	}

	printf("======> %s mWidth=%u mHeight=%u\n", __func__, mainDisplayInfo.h, mainDisplayInfo.w);

	sp<SurfaceControl> sc = client->createSurface(String8("Triangle"), width,
		height, PIXEL_FORMAT_RGBX_8888, ISurfaceComposerClient::eOpaque);
	if (sc == NULL || !sc->isValid()) {
		fprintf(stderr, "Failed to create SurfaceControl\n");
		return;
	}

	SurfaceComposerClient::openGlobalTransaction();
	err = sc->setLayer(0x7FFFFFFF);  // always on top
	if (err != NO_ERROR) {
		fprintf(stderr, "SurfaceComposer::setLayer error:0x%x\n", err);
		return;
	}

	err = sc->show();
	if (err != NO_ERROR) {
		fprintf(stderr, "SurfaceComposer::show error:0x%x\n", err);
		return;
	}
	SurfaceComposerClient::closeGlobalTransaction();

	// xbdong
	sp<Surface> s = sc->getSurface();

	EGLConfig config = {0};
	EGLDisplay dinfo;
	EGLSurface surface;
	EGLContext context;
	EGLint majorVersion, minorVersion;

	EGLint context_attrs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
	EGLint attrs[] = {
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_NONE};

	checkEglError("<init>");
	dinfo = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	checkEglError("eglGetDisplay");
	if (dinfo == EGL_NO_DISPLAY) {
		fprintf(stderr, "eglGetDisplay return EGL_NO_DISPLAY.\n");
		return;
	}

	ret = eglInitialize(dinfo, &majorVersion, &minorVersion);
	checkEglError("eglInitialize");
	fprintf(stderr, "EGL version %d.%d\n", majorVersion, minorVersion);
	if (ret != EGL_TRUE) {
		fprintf(stderr, "eglInitialize failed\n");
		return;
	}

	EGLint numConfigs;
	if (eglChooseConfig(dinfo, attrs, &config, 1, &numConfigs) ==
		EGL_FALSE) {
		fprintf(stderr, "eglChooseConfig error\n");
		return;
	}

	surface = eglCreateWindowSurface(dinfo, config, s.get(), NULL);
	checkEglError("eglCreateWindowSurface");
	if (surface == EGL_NO_SURFACE) {
		fprintf(stderr, "eglCreateWindowSurface failed.\n");
		return;
	}

	context = eglCreateContext(dinfo, config, EGL_NO_CONTEXT, context_attrs);
	checkEglError("eglCreateContext");
	if (context == EGL_NO_CONTEXT) {
		fprintf(stderr, "eglCreawteContext failed.\n");
		return;
	}

	ret = eglMakeCurrent(dinfo, surface, surface, context);
	checkEglError("eglMakeCurrent");
	if (ret == EGL_FALSE) {
		fprintf(stderr, "eglMakeCurrent failed.\n");
		return;
	}

	printf("<==== initSurface\n");
	//return (EGLNativeWindowType)window.get();
}

//int main(int argc, char** argv)
int main(void)
{
	printf("====> %s ====[1]====\n", __func__);

	initSurface();

	printf("====> %s ====[2]====\n", __func__);

	dumpGLInfo();

	printf("====> %s ====[3]====\n", __func__);
	return 0;
}

