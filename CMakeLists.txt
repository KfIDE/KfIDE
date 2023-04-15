cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)
project(KfIDE VERSION 0.1.0 LANGUAGES C)
set(CMAKE_C_STANDARD 90)

if(WIN32)
	set(PLAT_IMPL "${CMAKE_SOURCE_DIR}/src/platform/platform_win32.c")
elseif(UNIX)
	set(PLAT_IMPL "${CMAKE_SOURCE_DIR}/src/platform/platform_unix.c")
endif()

add_executable(KfIDE "src/kf.c" ${PLAT_IMPL})
target_include_directories(KfIDE PUBLIC "${CMAKE_SOURCE_DIR}/include")

set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)
target_include_directories(KfIDE PUBLIC ${OPENGL_egl_INCLUDE_DIRS})
target_link_libraries(KfIDE ${OPENGL_egl_LIBRARY})

if(WIN32)
elseif(UNIX)
	find_package(X11 REQUIRED)
	target_include_directories(KfIDE PUBLIC ${X11_INCLUDE_DIRS})
	target_link_libraries(KfIDE ${X11_LIBRARIES})
endif()