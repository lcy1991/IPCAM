LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:=               \
	AHandler.cpp         \
	ALooper.cpp          \
	ALooperRoster.cpp    \
	AMessage.cpp         \
	AString.cpp          \
	AAtomizer.cpp        \
	ARTSPConnection.cpp  \
	MyRTSPHandler.cpp    \
	ABuffer.cpp          \
	our_md5.cpp          \
	ARTPSource.cpp       \
	ARTPConnection.cpp   \
	base64.cpp           \
	IPCAM.cpp            \
	libuuid/clear.c            \
	libuuid/compare.c          \
	libuuid/copy.c             \
	libuuid/gen_uuid.c         \
	libuuid/isnull.c           \
	libuuid/pack.c             \
	libuuid/parse.c            \
	libuuid/randutils.c        \
	libuuid/unpack.c           \
	libuuid/unparse.c          \
	libuuid/uuid_time.c

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/include \
	$(JNI_H_INCLUDE)

LOCAL_LDLIBS := -llog

LOCAL_CFLAGS += -g -Wno-multichar -DHAVE_USLEEP -DANDROID

LOCAL_MODULE:= stage

#LOCAL_PRELINK_MODULE:= false

include $(BUILD_SHARED_LIBRARY)
