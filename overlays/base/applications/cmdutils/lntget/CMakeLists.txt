include_directories(
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/libs/zlib
    ${REACTOS_SOURCE_DIR}/sdk/lib/3rdparty/zlib/contrib)

list(APPEND SOURCE
    category.cpp
    details.cpp
    gui.cpp
    hash.cpp
    http.cpp
    installed.cpp
    installer.cpp
    manifest.cpp
    source.cpp
    source_trust.cpp
    sqlite_index.cpp
    util.cpp)

add_rc_deps(lntget.rc
    ${CMAKE_CURRENT_SOURCE_DIR}/res/lntget.ico
    ${CMAKE_CURRENT_SOURCE_DIR}/res/ICON-SOURCE.txt)
add_executable(lntget ${SOURCE} main.cpp lntget.rc)
set_module_type(lntget win32cui UNICODE)
add_executable(thyfos-store ${SOURCE} store.cpp lntget.rc)
target_compile_definitions(thyfos-store PRIVATE THYFOS_STORE)
set_module_type(thyfos-store win32gui UNICODE)
foreach(app lntget thyfos-store)
    set_property(TARGET ${app} PROPERTY CXX_STANDARD 17)
    set_property(TARGET ${app} PROPERTY CXX_STANDARD_REQUIRED ON)
    if(MSVC)
        set_target_cpp_properties(${app} WITH_EXCEPTIONS)
    endif()
    target_link_libraries(${app} cppstl cpprt minizip zlib)
    add_importlibs(${app} advapi32 bcrypt comctl32 crypt32 gdi32 shell32 user32 wininet msvcrt kernel32 ntdll)
    add_cd_file(TARGET ${app} DESTINATION reactos/system32 FOR all)
endforeach()
