project(
    'phosphor-ipmi-serial',
    'cpp',
    version: '1.0.0',
    meson_version: '>=0.58.0',
    default_options: [
        'warning_level=3',
        'werror=false',
        'cpp_std=c++20',
        'buildtype=debugoptimized',
        'b_lto=true',
    ],
)

conf_data = configuration_data()
conf_data.set('TERMINAL_MODE', get_option('terminal_mode').enabled())
conf_data.set('BASIC_MODE', get_option('basic_mode').enabled())
conf_data.set('SERIAL_DEVICE', get_option('serial-device'))
configure_file(
    output: 'config.h',
    configuration: conf_data,
    install: false,
)

systemd_dep = dependency('systemd', required: true)
sdbusplus_dep = dependency('sdbusplus', required: true)
phosphor_logging_dep = dependency('phosphor-logging', required: true)
phosphor_dbus_interfaces_dep = dependency('phosphor-dbus-interfaces', required: true)
libsystemd_dep = dependency('libsystemd', required: true)

deps = [
    libsystemd_dep,
    phosphor_logging_dep,
    phosphor_dbus_interfaces_dep,
    sdbusplus_dep,
]

sources = [
    'main.cpp',
    'uart_channel.cpp',
    'event_loop.cpp',
    'message_handler.cpp',
    'message_parsers.cpp',
    'terminal_mode_parser.cpp',
    'basic_mode_parser.cpp',
    'sessions_manager.cpp',
    'command_table.cpp',
    'command/channel_auth.cpp',
    'command/session_cmds.cpp',
    'command/get_message.cpp',
    'debug/frame_dumper.cpp',
    'debug/statistics.cpp',
]

project_inc = include_directories('.')

executable(
    'serialipmid',
    sources,
    implicit_include_directories: false,
    include_directories: [project_inc, 'command', 'debug'],
    dependencies: deps,
    install: true,
    install_dir: get_option('bindir'),
)

systemd_system_unit_dir = systemd_dep.get_variable(
    pkgconfig: 'systemdsystemunitdir',
)

configure_file(
    input: 'phosphor-ipmi-serial@.service',
    output: 'phosphor-ipmi-serial@.service',
    copy: true,
    install_dir: systemd_system_unit_dir,
)

install_data('phosphor-ipmi-serial.service',
    install_dir: systemd_system_unit_dir
)

if get_option('tests').enabled()
    subdir('test')
endif
