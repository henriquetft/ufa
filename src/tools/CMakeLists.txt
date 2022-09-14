# Adding executable ufaattr
add_executable(ufaattr ufaattr.c)
target_include_directories(ufaattr PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ufaattr ufa-util ufa-core)

# Adding executable ufafind
add_executable(ufafind ufafind.c)
target_include_directories(ufafind PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ufafind ufa-util ufa-core)

# Adding executable ufatag
add_executable(ufatag ufatag.c)
target_include_directories(ufatag PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ufatag ufa-util ufa-core)