cmake_minimum_required(VERSION 3.14)
project(zhouyi-master-agent VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find required packages
find_package(CURL REQUIRED)
find_package(Threads REQUIRED)

# SQLite3 - try find_package first, fallback to manual
find_package(SQLite3 QUIET)
if(NOT SQLite3_FOUND)
    # Try pkg-config
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(SQLITE3 QUIET sqlite3)
    endif()
    if(NOT SQLITE3_FOUND)
        # Fallback: just link -lsqlite3
        set(SQLITE3_LIBRARIES sqlite3)
        set(SQLITE3_INCLUDE_DIRS "")
    endif()
endif()

# Source files (header-only library, only main.cpp needs compilation)
set(SOURCES
    src/main.cpp
)

# Create executable
add_executable(zhouyi-master-agent ${SOURCES})

# Include directories
target_include_directories(zhouyi-master-agent PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CURL_INCLUDE_DIRS}
)

if(SQLite3_FOUND)
    target_link_libraries(zhouyi-master-agent PRIVATE
        ${CURL_LIBRARIES}
        SQLite::SQLite3
        Threads::Threads
    )
elseif(SQLITE3_FOUND)
    target_include_directories(zhouyi-master-agent PRIVATE ${SQLITE3_INCLUDE_DIRS})
    target_link_libraries(zhouyi-master-agent PRIVATE
        ${CURL_LIBRARIES}
        ${SQLITE3_LIBRARIES}
        Threads::Threads
    )
else()
    target_link_libraries(zhouyi-master-agent PRIVATE
        ${CURL_LIBRARIES}
        sqlite3
        Threads::Threads
    )
endif()

# Compiler warnings
target_compile_options(zhouyi-master-agent PRIVATE -Wall -Wextra -O2)
