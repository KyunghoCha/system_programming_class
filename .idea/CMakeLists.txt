cmake_minimum_required(VERSION 4.0)
project(system_programming_class C)

# set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(_idea
        c_cpp_properties.json
        editor.xml
        modules.xml
        system_programming_mine.iml
        vcs.xml
        workspace.xml
        ../assignment1/merge_txt.c
        ../assignment2/ftype.c
        ../assignment2/fchmod.c
        ../assignment2/touch.c
        ../assignment2/cptime.c
        ../assignment2/list1.c
        ../assignment2/list2.c
        ../assignment2/link.c
        ../assignment2/unlink.c
        ../assignment2/slink.c
        ../assignment2/rlink.c
        ../assignment1/merge_txt.c)
