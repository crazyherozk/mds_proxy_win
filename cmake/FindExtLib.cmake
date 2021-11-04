function(FindExtLib)
    if (TARGET extlib_internal)
        return()
    endif()

    set(EXTLIB_SOURCE_DIR "${PROJECT_SOURCE_DIR}/ext")
    set(EXTLIB_INCLUDE_DIR "${EXTLIB_SOURCE_DIR}/include" CACHE INTERNAL "")
    set(EXTLIB_DEFINITIONS 
        "ZMQ_STATIC"
        "ZMQ_USE_TWEETNACL"
        CACHE INTERNAL ""
    )

    set(EXTLIB_LIBRARIES
        "${EXTLIB_SOURCE_DIR}/lib/libzmq.lib"
        "${EXTLIB_SOURCE_DIR}/lib/mds_api.lib"
        CACHE INTERNAL ""
    )

    add_library(extlib_internal INTERFACE)
    # 仅构建时使用这些属性，不需要安装
    target_link_libraries(extlib_internal INTERFACE
        "$<BUILD_INTERFACE:${EXTLIB_LIBRARIES}>"
    )
    target_include_directories(extlib_internal INTERFACE
        "$<BUILD_INTERFACE:${EXTLIB_INCLUDE_DIR}>"
    )
    target_compile_definitions(extlib_internal INTERFACE
        "$<BUILD_INTERFACE:${EXTLIB_DEFINITIONS}>"
    )

endfunction()
