FindWs32()
FindExtLib()

set(LIBNAME mdspxy_api)
set(LIBNAMESPACE mds_proxy)

aux_source_directory(. MDS_PROXY_SOURCES)

file(GLOB_RECURSE MDS_PROXY_HEADERS
    ${PROJECT_SOURCE_DIR}/include/mdspxy_api/*.h ./*.h
    )
source_group("Header Files" FILES ${MDS_PROXY_HEADERS})
source_group("Source Files" FILES ${MDS_PROXY_SOURCES})

add_library(${LIBNAME} ${MDS_PROXY_SOURCES} ${MDS_PROXY_HEADERS})

if(BUILD_SHARED_LIBS)
    target_compile_definitions(${LIBNAME} PRIVATE -DBUILD_MDSPXY_DLL)
endif()

target_compile_options(${LIBNAME} PUBLIC /utf-8 /MP)

target_include_directories(${LIBNAME}
        PUBLIC
            $<INSTALL_INTERFACE:include>
            $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
            ${THREADS_INCLUDE_DIR})

if(NOT BUILD_SHARED_LIBS)
    target_link_libraries(${LIBNAME} PUBLIC extlib_internal)
else()
    target_link_libraries(${LIBNAME} PRIVATE extlib_internal)
endif()

target_link_libraries(${LIBNAME} PUBLIC ws32_internal)

set(API_TARGETS ${LIBNAME})
set(API_INSTALL_CFGDIR ${CMAKE_INSTALL_LIBDIR}/cmake/${LIBNAME})

if (TARGET ws32_internal)
    list(APPEND API_TARGETS ws32_internal)
endif()

if(NOT BUILD_SHARED_LIBS)
    if (TARGET extlib_internal)
       list(APPEND API_TARGETS extlib_internal)
    endif()
endif()

install(TARGETS ${API_TARGETS}
        EXPORT ${LIBNAME}-targets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/${LIBNAME}
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

include(CMakePackageConfigHelpers)
configure_package_config_file(
        ${PROJECT_SOURCE_DIR}/cmake/${LIBNAME}-config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/cmake/${LIBNAME}-config.cmake
        INSTALL_DESTINATION ${API_INSTALL_CFGDIR}
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/cmake/${LIBNAME}-config.cmake
        DESTINATION ${API_INSTALL_CFGDIR})

install(EXPORT ${LIBNAME}-targets
        FILE ${LIBNAME}-targets.cmake
        NAMESPACE ${LIBNAMESPACE}::
        DESTINATION ${API_INSTALL_CFGDIR})