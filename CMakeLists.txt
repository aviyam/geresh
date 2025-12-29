cmake_minimum_required(VERSION 3.14)
project(geresh VERSION 0.6.3 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FindPkgConfig)
include(CheckFunctionExists)
include(CheckIncludeFile)
include(CheckTypeSize)
include(CheckSymbolExists)

# Dependencies
# Try to find ncursesw
set(CURSES_NEED_NCURSES TRUE)
set(CURSES_NEED_WIDE TRUE)

if(APPLE)
    # ncurses is keg-only on Homebrew, so we need to give CMake a hint
    find_program(BREW_PROG brew)
    if(BREW_PROG)
        execute_process(COMMAND ${BREW_PROG} --prefix ncurses OUTPUT_VARIABLE NCURSES_PREFIX OUTPUT_STRIP_TRAILING_WHITESPACE)
        if(EXISTS "${NCURSES_PREFIX}")
            list(APPEND CMAKE_PREFIX_PATH "${NCURSES_PREFIX}")
            include_directories("${NCURSES_PREFIX}/include")
            link_directories("${NCURSES_PREFIX}/lib")
        endif()
    endif()
endif()

find_package(Curses REQUIRED)

find_package(Iconv REQUIRED)
find_package(Intl)

pkg_check_modules(FRIBIDI REQUIRED fribidi)

# Configuration checks
check_include_file("dirent.h" HAVE_DIRENT_H)
check_include_file("sys/ndir.h" HAVE_SYS_NDIR_H)
check_include_file("sys/dir.h" HAVE_SYS_DIR_H)
check_include_file("ndir.h" HAVE_NDIR_H)
check_include_file("getopt.h" HAVE_GETOPT_LONG)
check_include_file("locale.h" HAVE_LOCALE_H)

check_function_exists("vsnprintf" HAVE_VSNPRINTF)
check_function_exists("vasprintf" HAVE_VASPRINTF)
check_function_exists("use_default_colors" HAVE_USE_DEFAULT_COLORS)
check_function_exists("curs_set" HAVE_CURS_SET)
check_function_exists("wctob" HAVE_WCTOB)
check_function_exists("btowc" HAVE_BTOWC)
check_function_exists("setlocale" HAVE_SETLOCALE)

# Endianness
include(TestBigEndian)
TEST_BIG_ENDIAN(IS_BIG_ENDIAN)
if(IS_BIG_ENDIAN)
    set(INTERNAL_ENCODING "UCS-4BE")
else()
    set(INTERNAL_ENCODING "UCS-4LE")
endif()

# Check iconv signature
include(CheckCXXSourceCompiles)
set(CMAKE_REQUIRED_INCLUDES ${Iconv_INCLUDE_DIRS})
set(CMAKE_REQUIRED_LIBRARIES Iconv::Iconv)
check_cxx_source_compiles("
#include <iconv.h>
int main() {
    iconv_t cd = 0;
    const char* in = 0;
    iconv(cd, &in, 0, 0, 0);
    return 0;
}
" ICONV_SECOND_ARGUMENT_IS_CONST)

if(ICONV_SECOND_ARGUMENT_IS_CONST)
    set(ICONV_CONST "const")
else()
    set(ICONV_CONST "")
endif()

set(DEFAULT_FILE_ENCODING "ISO-8859-8")

if(CURSES_FOUND)
    set(HAVE_WIDE_CURSES 1)
    set(HAVE_COLOR 1)
endif()

if(Intl_FOUND)
    set(USE_GETTEXT 1)
endif()

if(Iconv_FOUND)
    set(USE_ICONV 1)
endif()

if(Iconv_FOUND)
    set(USE_ICONV 1)
endif()

# Generate config.h
set(PACKAGE "geresh")
set(VERSION ${PROJECT_VERSION})
configure_file(cmake_config.h.in config.h)

# Include directories

# Include directories
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${FRIBIDI_INCLUDE_DIRS})
if(Intl_FOUND)
    include_directories(${Intl_INCLUDE_DIRS})
endif()

# Sources
set(SOURCES
    basemenu.cc bidi.cc bindings.cc converters.cc dbg.cc dialogline.cc
    editbox.cc editbox2.cc editor.cc event.cc helpbox.cc inputline.cc
    io.cc iso88598.cc label.cc main.cc menus.cc mk_wcwidth.cc
    question.cc scrollbar.cc shaping.cc speller.cc statusline.cc
    terminal.cc themes.cc transtbl.cc types.cc undo.cc utf8.cc widget.cc
)

add_executable(geresh ${SOURCES})

if(APPLE)
    target_link_directories(geresh PRIVATE /opt/homebrew/lib)
endif()

if(FRIBIDI_LIBRARY_DIRS)
    target_link_directories(geresh PRIVATE ${FRIBIDI_LIBRARY_DIRS})
endif()

target_link_libraries(geresh PRIVATE ${FRIBIDI_LIBRARIES})

target_compile_definitions(geresh PRIVATE PKGDATADIR="${CMAKE_INSTALL_PREFIX}/share/geresh")

if(CURSES_FOUND)
    target_link_libraries(geresh PRIVATE ${CURSES_LIBRARIES})
endif()
if(Iconv_FOUND)
    target_link_libraries(geresh PRIVATE Iconv::Iconv)
endif()
if(Intl_FOUND)
    target_link_libraries(geresh PRIVATE ${Intl_LIBRARIES})
endif()

install(TARGETS geresh DESTINATION bin)
