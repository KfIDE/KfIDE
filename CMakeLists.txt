cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)
project(KfIDE VERSION 0.1.0 LANGUAGES C)
set(CMAKE_C_STANDARD 90)

if(WIN32)
    set(PLAT_IMPL "${CMAKE_SOURCE_DIR}/src/platform/platform_win32.c")
    set(KF_PLATFORM_DEF "KF_PLATFORM_WIN32")
elseif(APPLE)
    set(PLAT_IMPL "${CMAKE_SOURCE_DIR}/src/platform/platform_apple.c")
    set(KF_PLATFORM_DEF "KF_PLATFORM_APPLE")
elseif(UNIX)
    set(PLAT_IMPL "${CMAKE_SOURCE_DIR}/src/platform/platform_unix.c")
    set(KF_PLATFORM_DEF "KF_PLATFORM_UNIX")
endif()
add_compile_definitions("${KF_PLATFORM_DEF}")

add_executable(KfIDE "src/kf.c" ${PLAT_IMPL})
target_include_directories(KfIDE PUBLIC "${CMAKE_SOURCE_DIR}/include")

set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)

# Moved these to the plat-specific thingies

# target_include_directories(KfIDE PUBLIC ${OPENGL_egl_INCLUDE_DIRS})
# target_link_libraries(KfIDE ${OPENGL_egl_LIBRARY} ${SILICON_LIBRARY})

if(KF_DEBUG)
    add_compile_definitions("KF_DEBUG")
endif()

if(WIN32)
elseif(APPLE)
	find_library(FOUNDATION_FW Foundation)
	find_library(OPENGL_FW OpenGL)
	find_library(APPKIT_FW AppKit)
	find_library(COREVIDEO_FW CoreVideo)
	find_library(SILICON_FW Silicon)

	target_link_libraries(KfIDE ${FOUNDATION_FW} ${FOUNDATION_FW} ${OPENGL_FW} ${APPKIT_FW} ${COREVIDEO_FW})
	include_directories(/usr/local/include)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -L/usr/local/lib -lSilicon -Wno-pointer-sign")
elseif(UNIX)
    find_package(X11 REQUIRED)
    target_include_directories(KfIDE PUBLIC ${X11_INCLUDE_DIRS})
    target_link_libraries(KfIDE ${X11_LIBRARIES})

    find_library(GLIBC_MATH
    	NAMES m libm
    	PATHS "/usr/lib" "/usr/local/lib")
    target_link_libraries(KfIDE ${OPENGL_glx_LIBRARY} ${OPENGL_LIBRARY} ${GLIBC_MATH})
endif()