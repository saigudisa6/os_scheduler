add_executable(FCFSSchedulerTests
    FCFSTest.cpp
)

target_link_libraries(FCFSSchedulerTests 
    PRIVATE
    scheduler_core
    gtest 
    gtest_main
)

add_test(NAME FCFSSchedulerTests COMMAND FCFSSchedulerTests)