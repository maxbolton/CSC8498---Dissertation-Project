set(PROJECT_NAME CSC8498)

################################################################################
# Source groups
################################################################################
file(GLOB ALL_FILES "*.cpp" "*.h")

# Create the library target only once
add_executable(${PROJECT_NAME} ${ALL_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION_RELEASE "TRUE"
)

target_precompile_headers(${PROJECT_NAME} PRIVATE
    <memory>
    <unordered_set>
    <vector>
    <string>
    <fstream>
    <sstream>
    <iostream>
    <iosfwd>
    <set>
    <map> 
    <chrono>
    <thread>
    <filesystem>
    <functional>
    <algorithm>
    <assert.h>
)

################################################################################
# Compile and link options
################################################################################
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        ${DEFAULT_CXX_DEBUG_INFORMATION_FORMAT};
        ${DEFAULT_CXX_EXCEPTION_HANDLING};
    )
endif()

include_directories("../OpenGLRendering/")
include_directories("../NCLCoreClasses/")
include_directories("../CSC8503CoreClasses/")

target_link_libraries(${PROJECT_NAME} LINK_PUBLIC NCLCoreClasses)
target_link_libraries(${PROJECT_NAME} LINK_PUBLIC CSC8503CoreClasses)
target_link_libraries(${PROJECT_NAME} LINK_PUBLIC OpenGLRendering)