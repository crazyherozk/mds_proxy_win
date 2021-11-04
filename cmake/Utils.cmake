macro(set_win32_options)
    message(STATUS "Setting msvc options")

    set(CMAKE_EXTRA_INCLUDE_FILES winsock2.h ws2tcpip.h)
    set(CMAKE_REQUIRED_DEFINITIONS -FIwinsock2.h -FIws2tcpip.h)
    if(CMAKE_SYSTEM_NAME STREQUAL "WindowsStore")
        #add_definitions(-D_WIN32_WINNT=0x0A00)
    else()
        #add_definitions(-D_WIN32_WINNT=0x0600)
    endif()
    
    set(WARNINGS)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W2")
    #set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP /utf-8")
    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /utf-8 /std:c++latest")
    #set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS}")
    set(CMAKE_SHARED_LINKER_FLAGS_MINSIZEREL "${CMAKE_SHARED_LINKER_FLAGS_MINSIZEREL} /profile /OPT:REF /OPT:ICF")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /profile /OPT:REF /OPT:ICF")
    
    if(BUILD_SHARED_LIBS)
        message(STATUS "build shared library")
        set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /Od /RTC1 /MDd" )
        set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /Oxt /Zp8 /Gy /MD")
    else()
        message(STATUS "build static library")
        set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /Od /RTC1 /MTd" )
        set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /Oxt /Zp8 /Gy /MT")
    endif()
    
    if(INSTALL_TEMP)
        set(OKEX_INSTALL_ROOT "C:/Windows/Temp/install")
        set(CMAKE_INSTALL_PREFIX ${OKEX_INSTALL_ROOT})
        set(CMAKE_INSTALL_LIBDIR ${OKEX_INSTALL_ROOT}/lib)
        set(CMAKE_INSTALL_INCLUDEDIR ${OKEX_INSTALL_ROOT}/include)
    else()
        include(GNUInstallDirs)
    endif() 
    message(STATUS "install libdir:" ${CMAKE_INSTALL_LIBDIR})
    message(STATUS "install includedir:" ${CMAKE_INSTALL_INCLUDEDIR})
endmacro()

macro(add_mds_proxy_executable name)
    get_filename_component(extend_name ${name} EXT)
    get_filename_component(file_name ${name} NAME_WE)

    message(STATUS "add one executable program : ${file_name} ...")

    add_executable(${file_name} ${name})

    target_include_directories(${file_name} PRIVATE
        $<TARGET_PROPERTY:extlib_internal,INTERFACE_INCLUDE_DIRECTORIES>
    )

    if(BUILD_SHARED_LIBS)
        set_target_properties(${file_name} PROPERTIES INSTALL_RPATH
            "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}"
        )
        target_compile_definitions(${file_name} PRIVATE -DLINK_MDSPXY_DLL)
    endif()

    target_link_libraries(${file_name} PRIVATE mdspxy_api)
endmacro()