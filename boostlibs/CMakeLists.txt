include(Boost)

message(STATUS "boost_ROOT=${boost_ROOT}")

file(GLOB_RECURSE HEADERS "${boost_ROOT}/include/*.h")

list(APPEND SOURCES "${boost_ROOT}/libs/chrono/src/chrono.cpp")
list(APPEND SOURCES "${boost_ROOT}/libs/chrono/src/process_cpu_clocks.cpp")
list(APPEND SOURCES "${boost_ROOT}/libs/chrono/src/thread_clock.cpp")

list(APPEND SOURCES "${boost_ROOT}/libs/iostreams/src/file_descriptor.cpp")
list(APPEND SOURCES "${boost_ROOT}/libs/iostreams/src/gzip.cpp")
list(APPEND SOURCES "${boost_ROOT}/libs/iostreams/src/mapped_file.cpp")
list(APPEND SOURCES "${boost_ROOT}/libs/iostreams/src/zlib.cpp")

list(APPEND SOURCES "${boost_ROOT}/libs/signals/src/connection.cpp")
list(APPEND SOURCES "${boost_ROOT}/libs/signals/src/named_slot_map.cpp")
list(APPEND SOURCES "${boost_ROOT}/libs/signals/src/signal_base.cpp")
list(APPEND SOURCES "${boost_ROOT}/libs/signals/src/slot.cpp")
list(APPEND SOURCES "${boost_ROOT}/libs/signals/src/trackable.cpp")

list(APPEND SOURCES "${boost_ROOT}/libs/filesystem/src/codecvt_error_category.cpp")
list(APPEND SOURCES "${boost_ROOT}/libs/filesystem/src/operations.cpp")
list(APPEND SOURCES "${boost_ROOT}/libs/filesystem/src/path.cpp")
list(APPEND SOURCES "${boost_ROOT}/libs/filesystem/src/path_traits.cpp")
list(APPEND SOURCES "${boost_ROOT}/libs/filesystem/src/portability.cpp")
list(APPEND SOURCES "${boost_ROOT}/libs/filesystem/src/unique_path.cpp")
list(APPEND SOURCES "${boost_ROOT}/libs/filesystem/src/utf8_codecvt_facet.cpp")
list(APPEND SOURCES "${boost_ROOT}/libs/filesystem/src/windows_file_codecvt.cpp")

list(APPEND SOURCES "${boost_ROOT}/libs/thread/src/future.cpp")
list(APPEND SOURCES "${boost_ROOT}/libs/thread/src/pthread/once.cpp")
list(APPEND SOURCES "${boost_ROOT}/libs/thread/src/pthread/thread.cpp")

list(APPEND SOURCES "${boost_ROOT}/libs/system/src/error_code.cpp")

add_library(boost OBJECT ${SOURCES} ${HEADERS})
