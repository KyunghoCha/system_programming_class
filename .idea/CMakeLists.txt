cmake_minimum_required(VERSION 4.0)
project(_idea)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(_idea
        c_cpp_properties.json
        editor.xml
        modules.xml
        system_programming_mine.iml
        vcs.xml
        workspace.xml
        ../assignment2/file_type_out.c
        ../assignment2/change_permission.c
        ../assignment1/merge_txt.c)
