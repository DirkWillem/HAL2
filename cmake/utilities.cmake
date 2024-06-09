function (link_library_hdronly TARGET VISIBILITY LIBRARY)
    get_target_property(HDRS ${LIBRARY} INCLUDE_DIRECTORIES)

    target_include_directories(${TARGET} ${VISIBILITY} ${HDRS})
endfunction()