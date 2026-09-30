add_library(per_malloc)

target_sources(per_malloc
    PRIVATE
    per_malloc.c
    
    PUBLIC
    FILE_SET HEADERS
    FILES
    per_malloc.h
)