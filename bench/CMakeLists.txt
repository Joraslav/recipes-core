FIND_LIB(benchmark)

add_executable(ThreadOutput
  ThreadOutput.cpp
)

target_link_libraries(ThreadOutput PRIVATE
  benchmark::benchmark
  benchmark::benchmark_main
  _io
)
