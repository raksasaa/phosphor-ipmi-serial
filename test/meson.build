gtest_dep = dependency('gtest', main: true, required: false)
gmock_dep = dependency('gmock', required: false)
if not gtest_dep.found()
    gtest_proj = subproject('gtest', required: true)
    gtest_dep = gtest_proj.get_variable('gtest_main_dep')
    gmock_dep = gtest_proj.get_variable('gmock_dep')
endif

test_sources = [
    '../terminal_mode_parser.cpp',
    '../basic_mode_parser.cpp',
    '../message_parsers.cpp',
    '../sessions_manager.hpp',
    '../command_table.cpp',
    '../command_table.hpp',
    '../message_handler.cpp',
    '../message_handler.hpp',
    '../message.hpp',
    '../uart_channel.hpp',
]

tests = [
    'terminal_mode_parser_test',
    'basic_mode_parser_test',
    'checksum_test',
    'session_test',
    'message_handler_test',
]

foreach t : tests
    exe = executable(t, t + '.cpp',
        dependencies: [gtest_dep, gmock_dep],
        include_directories: ['..'],
    )
    test(t, exe)
endforeach
