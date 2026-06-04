# Top-level Tests/*.ll — the three public cases the course repo ships
# (stmt.ll, buf_overflow.ll, null_deref.ll).  Registered separately from
# the categorised cases/ corpus so the public smoke tests still run in CI
# when the cases/ subtree is absent (e.g. on the teaching/template repo).
file(GLOB public_ae_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/../Tests "${CMAKE_CURRENT_SOURCE_DIR}/../Tests/*.ll")
foreach(filename ${public_ae_files})
    add_test(
            NAME ass3-public-py/${filename}
            COMMAND python3 test-ae.py ${CMAKE_CURRENT_SOURCE_DIR}/../Tests/${filename}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
endforeach()

# loops over ae_assert_files and run "ae $bc_file"
file(GLOB ae_assert_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/../Tests "${CMAKE_CURRENT_SOURCE_DIR}/../Tests/ae/*.ll")
foreach(filename ${ae_assert_files})
    add_test(
            NAME ass3-ae-py/${filename}
            COMMAND python3 test-ae.py ${CMAKE_CURRENT_SOURCE_DIR}/../Tests/${filename}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
endforeach()

file(GLOB buf_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/../Tests "${CMAKE_CURRENT_SOURCE_DIR}/../Tests/buf/*.ll")
foreach(filename ${buf_files})
    add_test(
            NAME ass3-buf-py/${filename}
            COMMAND python3 test-ae.py ${CMAKE_CURRENT_SOURCE_DIR}/../Tests/${filename}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
endforeach()

# Categorised abstract-execution corpus (Tests/cases). Each case self-validates
# via svf_assert / UNSAFE_* checkpoints, so a clean exit means pass. Only the
# passing corpus lives under Tests/cases; known-failing cases are kept under
# Tests/cases-unpass and are intentionally NOT registered.
file(GLOB_RECURSE case_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/../Tests "${CMAKE_CURRENT_SOURCE_DIR}/../Tests/cases/*.ll")
foreach(filename ${case_files})
    add_test(
            NAME ass3-cases-py/${filename}
            COMMAND python3 test-ae.py ${CMAKE_CURRENT_SOURCE_DIR}/../Tests/${filename}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
    set_tests_properties(ass3-cases-py/${filename} PROPERTIES TIMEOUT 180)
endforeach()
include(CTest)


