cmake_minimum_required(VERSION 3.24)
project(SearchServer)

set(CMAKE_CXX_STANDARD 17)

add_executable(SearchServer
    main.cpp
    document.cpp
    document.h
    paginator.h
    process_queries.cpp
    process_queries.h
    read_input_functions.cpp
    read_input_functions.h
    request_queue.cpp
    request_queue.h
    search_server.cpp
    search_server.h
    string_processing.cpp
    string_processing.h
    test_example_functions.cpp
    test_example_functions.h
    log_duration.h
    concurrent_map.h
)
