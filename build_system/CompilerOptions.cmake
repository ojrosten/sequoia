if (MSVC)
    add_compile_options(/W4)
    add_definitions(/bigobj)
else()
    add_compile_options(-Wall -Wextra -pedantic)
endif()