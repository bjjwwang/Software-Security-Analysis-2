include_directories(${Z3MGR_DIR})
file(GLOB SOURCES
        *.cpp
)
list(REMOVE_ITEM SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/test-ae.cpp")
set(LIB ${SVF_LIB} ${llvm_libs})
add_library(assign3 ${SOURCES})
target_link_libraries(assign3 ${LIB})
set_target_properties(assign3 PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)
add_executable(ass3 test-ae.cpp)
target_link_libraries(ass3 ${LIB} assign3)
set_target_properties(ass3 PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)


# Top-level Tests/*.ll — the three public cases the course repo ships
# (stmt.ll, buf_overflow.ll, null_deref.ll).  Registered separately from
# the categorised cases/ corpus so the public smoke tests still run in CI
# when the cases/ subtree is absent (e.g. on the teaching/template repo).
file(GLOB public_ae_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/../Tests "${CMAKE_CURRENT_SOURCE_DIR}/../Tests/*.ll")
foreach(filename ${public_ae_files})
    add_test(
            NAME ass3-public-cpp/${filename}
            COMMAND ass3 ${CMAKE_CURRENT_SOURCE_DIR}/../Tests/${filename}
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )
endforeach()

# loops over ae_assert_files and run "ae $bc_file"
file(GLOB ae_assert_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/../Tests "${CMAKE_CURRENT_SOURCE_DIR}/../Tests/ae/*.ll")
foreach(filename ${ae_assert_files})
    add_test(
            NAME ass3-ae-cpp/${filename}
            COMMAND ass3 ${CMAKE_CURRENT_SOURCE_DIR}/../Tests/${filename}
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )
endforeach()

file(GLOB buf_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/../Tests "${CMAKE_CURRENT_SOURCE_DIR}/../Tests/buf/*.ll")
foreach(filename ${buf_files})
    add_test(
            NAME ass3-buf-cpp/${filename}
            COMMAND ass3 -overflow ${CMAKE_CURRENT_SOURCE_DIR}/../Tests/${filename}
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )
endforeach()

# Categorised abstract-execution corpus (Tests/cases). Each case self-validates
# via svf_assert / UNSAFE_* checkpoints, so a clean exit means pass. Only the
# passing corpus lives under Tests/cases; known-failing cases are kept under
# Tests/cases-unpass and are intentionally NOT registered.
file(GLOB_RECURSE case_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/../Tests "${CMAKE_CURRENT_SOURCE_DIR}/../Tests/cases/*.ll")
foreach(filename ${case_files})
    add_test(
            NAME ass3-cases-cpp/${filename}
            COMMAND ass3 ${CMAKE_CURRENT_SOURCE_DIR}/../Tests/${filename}
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )
    set_tests_properties(ass3-cases-cpp/${filename} PROPERTIES TIMEOUT 120)
endforeach()
include(CTest)


