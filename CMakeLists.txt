project(lunapy)

# CMake 最低版本号要求
cmake_minimum_required (VERSION 2.8)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

set(INCLUDE_FILES
	src/lp.h
	src/lp_internal.h
	src/mem.h
	src/opcode.h
	src/sym.h
	src/tokenize.h
	)

set(SRC_FILES
	src/lp.c
	src/list.c
	src/dict.c
	src/misc.c
	src/string.c
	src/builtins.c
	src/gc.c
	src/ops.c
	src/vm.c
	src/lpmain.c
	src/encode.c
	src/parse.c
	src/tokenize.c
	src/sym.c
	src/mem.c
	src/tests.c
	)

set(MATH_FILES
	modules/math/math.c
	)

set(RANDOM_FILES
	modules/random/random.c
	)

set(RE_FILES
	modules/re/init.c
	modules/re/regexpr.c
	)

set(TIME_FILES
	modules/time/time.c
	)

source_group("include" FILES ${INCLUDE_FILES})
source_group("src" FILES ${SRC_FILES})
source_group("math" FILES ${MATH_FILES})
source_group("random" FILES ${RANDOM_FILES})
source_group("re" FILES ${RE_FILES})
source_group("time" FILES ${TIME_FILES})

list(APPEND SOURCE_FILES
			${INCLUDE_FILES}
			${SRC_FILES}
			${MATH_FILES}
			${RANDOM_FILES}
			${RE_FILES}
			${TIME_FILES})

include_directories(
	"${CMAKE_CURRENT_SOURCE_DIR}/src"
	)


add_executable(lp ${SOURCE_FILES})
