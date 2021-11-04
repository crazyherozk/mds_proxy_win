FindExtLib()

foreach(name IN ITEMS
        read_fsub.c
        read_sub.c
        read_file.c
        read_file_sync.c
    )
    add_mds_proxy_executable(${name})
endforeach()

install(FILES
    mds_tester_local.conf mds_tester.conf
    DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)
