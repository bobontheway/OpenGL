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

static const char pVertexSource[] = "attribute vec4 vPosition;\n"
	"void main() {\n"
	"  gl_Position = vPosition;\n"
	"}\n";

static const char pFragmentSource[] = "precision mediump float;\n"
	"void main() {\n"
	"  gl_FragColor = vec4(0.0, 1.0, 0.0, 1.0);\n"
	"}\n";

const GLfloat gTriangleVertices[] = {0.0f, 0.5f, -0.5f, -0.5f, 0.5f, -0.5f};

GLuint loadShader(GLenum shaderType, const char *pSource)
{
	GLuint shader = glCreateShader(shaderType);
	if (shader) {
		glShaderSource(shader, 1, &pSource, NULL);
		glCompileShader(shader);
		GLint compiled  = 0;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
		if (!compiled) {
			GLint infoLen = 0;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
			if (infoLen) {
				char *buf = (char *)malloc(infoLen);
				if (buf) {
					glGetShaderInfoLog(shader, infoLen, NULL, buf);
					fprintf(stderr, "Could not compile shader %d:\n%s\n",
						shaderType, buf);
					free(buf);
				}
				glDeleteShader(shader);
				shader = 0;
			}
		}
	} else {
		fprintf(stderr, "Create shader %d failed\n", shaderType);
	}

	return shader;
}

static void checkEglError(const char *op)
{
	for (EGLint error = eglGetError(); error != EGL_SUCCESS;
		error = eglGetError()) {
		fprintf(stderr, "after %s() eglError 0x%x\n", op, error);
	}
}

static void checkGLError(const char *op)
{
	for (EGLint error = glGetError(); error; error = glGetError()) {
		fprintf(stderr, "after %s() glError 0x%x\n", op, error);
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
	//printGLString("Extensions", GL_EXTENSIONS);
}

static void initSurface(EGLint &w, EGLint &h, EGLDisplay &dinfo, EGLSurface &surface)
{
	EGLBoolean ret;
	status_t err;

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

	sp<Surface> s = sc->getSurface();

	EGLConfig config = {0};
	//EGLDisplay dinfo;
	//EGLSurface surface;
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

	eglQuerySurface(dinfo, surface, EGL_WIDTH, &w);
	eglQuerySurface(dinfo, surface, EGL_HEIGHT, &h);


	printf("====> display=0x%x  surface=0x%x\n", dinfo, surface);
	printf("====> w=%d h=%d\n", w, h);
}

GLuint gProgram;
GLuint gvPositionHandle;

bool setupGraphics(int w, int h)
{
	GLuint vertexShader = loadShader(GL_VERTEX_SHADER, pVertexSource);
	if (!vertexShader) {
		fprintf(stderr, "load vertex shader failed\n");
		return EXIT_FAILURE;
	}

	GLuint pixelShader = loadShader(GL_FRAGMENT_SHADER, pFragmentSource);
	if (!pixelShader) {
		fprintf(stderr, "load fragment shader failed\n");
		return EXIT_FAILURE;
	}

	gProgram = glCreateProgram();
	if (gProgram) {
		printf("====> %s program=%d\n", __func__, gProgram);

		glAttachShader(gProgram, vertexShader);
		checkGLError("glAttachShader");
		glAttachShader(gProgram, pixelShader);
		checkGLError("glAttachShader");
		glLinkProgram(gProgram);
		checkGLError("glLinkProgram");
		GLint linkStatus = GL_FALSE;
		glGetProgramiv(gProgram, GL_LINK_STATUS, &linkStatus);
		if (linkStatus != GL_TRUE) {
			GLint bufLength = 0;
			glGetProgramiv(gProgram, GL_INFO_LOG_LENGTH, &bufLength);
			if (bufLength) {
				char *buf = (char *)malloc(bufLength);
				if (buf) {
					glGetProgramInfoLog(gProgram, bufLength, NULL, buf);
					fprintf(stderr, "Could not link program:\n%s\n", buf);
					free(buf);
				}
			}
			glDeleteProgram(gProgram);
			gProgram = 0;
		}
	} else {
		fprintf(stderr, "crete program failed: %u\n", gProgram);
		return EXIT_FAILURE;
	}

	gvPositionHandle = glGetAttribLocation(gProgram, "vPosition");
	checkGLError("glGetAttribLocation");
	fprintf(stderr, "glGetAttribLocation(\"vPosition\") = %d\n",
		gvPositionHandle);

	glViewport(0, 0, w, h);
	checkGLError("glViewport");

	// xbdong
	printf("after glViewport, w=%d h=%d\n", w, h);

	return true;
}

void renderFrame(void)
{
	glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
	checkGLError("glClearColor");
	//glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	glClear(GL_COLOR_BUFFER_BIT);
	checkGLError("glClear");

	glUseProgram(gProgram);
	checkGLError("glUseProgram");

	glVertexAttribPointer(gvPositionHandle, 2, GL_FLOAT, GL_FALSE, 0,
		gTriangleVertices);
	checkGLError("glVertexAttribPointer");
	glEnableVertexAttribArray(gvPositionHandle);
	checkGLError("glEnableVertexAttribArray");
	glDrawArrays(GL_TRIANGLES, 0, 3);
	checkGLError("glDrawArrays");
}

//int main(int argc, char** argv)
int main(void)
{
	EGLint width, height;

	//initSurface(width, height, display, surface);
	EGLBoolean ret;
	status_t err;

	sp<SurfaceComposerClient> client = new SurfaceComposerClient;
	err = client->initCheck();
	if (err != NO_ERROR) {
		fprintf(stderr, "SurfaceComposerClient initCheck error:0x%x\n", err);
		return EXIT_FAILURE;
	}

	// Get main display parameters
	sp<IBinder> mainDisplay = SurfaceComposerClient::getBuiltInDisplay(
		ISurfaceComposer::eDisplayIdMain);

	DisplayInfo mainDisplayInfo;
	err = SurfaceComposerClient::getDisplayInfo(mainDisplay, &mainDisplayInfo);
	if (err != NO_ERROR) {
		fprintf(stderr, "unable to get display characteristics: 0x%x\n", err);
		return EXIT_FAILURE;
	}

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
		return EXIT_FAILURE;
	}

	SurfaceComposerClient::openGlobalTransaction();
	err = sc->setLayer(0x7FFFFFFF);  // always on top
	if (err != NO_ERROR) {
		fprintf(stderr, "SurfaceComposer::setLayer error:0x%x\n", err);
		return EXIT_FAILURE;
	}

	err = sc->show();
	if (err != NO_ERROR) {
		fprintf(stderr, "SurfaceComposer::show error:0x%x\n", err);
		return EXIT_FAILURE;
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
		return EXIT_FAILURE;
	}

	ret = eglInitialize(dinfo, &majorVersion, &minorVersion);
	checkEglError("eglInitialize");
	fprintf(stderr, "EGL version %d.%d\n", majorVersion, minorVersion);
	if (ret != EGL_TRUE) {
		fprintf(stderr, "eglInitialize failed\n");
		return EXIT_FAILURE;
	}

	EGLint numConfigs;
	if (eglChooseConfig(dinfo, attrs, &config, 1, &numConfigs) ==
		EGL_FALSE) {
		fprintf(stderr, "eglChooseConfig error\n");
		return EXIT_FAILURE;
	}

	surface = eglCreateWindowSurface(dinfo, config, s.get(), NULL);
	checkEglError("eglCreateWindowSurface");
	if (surface == EGL_NO_SURFACE) {
		fprintf(stderr, "eglCreateWindowSurface failed.\n");
		return EXIT_FAILURE;
	}

	context = eglCreateContext(dinfo, config, EGL_NO_CONTEXT, context_attrs);
	checkEglError("eglCreateContext");
	if (context == EGL_NO_CONTEXT) {
		fprintf(stderr, "eglCreawteContext failed.\n");
		return EXIT_FAILURE;
	}

	ret = eglMakeCurrent(dinfo, surface, surface, context);
	checkEglError("eglMakeCurrent");
	if (ret == EGL_FALSE) {
		fprintf(stderr, "eglMakeCurrent failed.\n");
		return EXIT_FAILURE;
	}

	printf("====> display=0x%x  surface=0x%x\n", dinfo, surface);
	// initSurface - end


	printf("====> %s ====[w=%d h=%d]====\n", __func__, width, height);

	setupGraphics(width, height);
	dumpGLInfo();

	printf("display=0x%x  surface=0x%x\n", dinfo, surface);

	for (;;) {
		renderFrame();
		eglSwapBuffers(dinfo, surface);
		checkEglError("eglSwapBuffers");
	}

	return 0;
}

