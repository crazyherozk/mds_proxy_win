## Add the dependencies of our library
get_filename_component(mdspxy_api_CMAKE_DIR "${CMAKE_CURRENT_LIST_FILE}" DIRECTORY)
include(CMakeFindDependencyMacro)

find_dependency(Threads REQUIRED)

## Import the targets
if (NOT TARGET mds_proxy::mdspxy_api)
    include("${mdspxy_api_CMAKE_DIR}/mdspxy_api-targets.cmake")
endif()

set(MDS_PROXY_LIBRARIES mds_proxy::mdspxy_api)