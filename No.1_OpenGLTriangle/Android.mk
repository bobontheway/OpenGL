LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := OpenGLTriangle.cpp

LOCAL_SHARED_LIBRARIES := \
	libcutils \
	libEGL \
	libGLESv2 \
	libui \
	libgui \
	libutils

LOCAL_MODULE := opengl_triangle

LOCAL_MODULE_TAGS := optional

LOCAL_CFLAGS := -DGL_GLEXT_PROTOTYPES

include $(BUILD_EXECUTABLE)

						  

