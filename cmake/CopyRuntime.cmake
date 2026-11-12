# Static SFML builds have no runtime DLL list. Treat that as a valid no-op.
if(RUNTIME_DLLS)
    string(REPLACE "|" ";" dlls "${RUNTIME_DLLS}")
    file(COPY ${dlls} DESTINATION "${DESTINATION}")
endif()
