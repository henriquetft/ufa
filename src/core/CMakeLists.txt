include_directories(${FUSE_INCLUDE_DIRS} ${SQLITE_INCLUDE_DIRS})

add_library(ufa-core
        repo_sqlite.c
        )
target_link_libraries(ufa-core ${SQLITE_LDFLAGS} ufa-util)

# Adding executable ufafs
add_executable(ufafs ufafs.c repo_sqlite.c)
target_link_libraries(ufafs ${FUSE_LDFLAGS} ${SQLITE_LDFLAGS} ufa-util)

# Adding executable ufad
add_executable(ufad ufad.c repo_sqlite.c monitor_inotify.c)
target_link_libraries(ufad ${SQLITE_LDFLAGS} ufa-util Threads::Threads)





