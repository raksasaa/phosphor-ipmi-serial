#include "main.hpp"

#include "command_table.hpp"
#include "config.h"
#include "event_loop.hpp"
#include "message_handler.hpp"
#include "message_parsers.hpp"
#include "sessions_manager.hpp"
#include "uart_channel.hpp"

#include <getopt.h>
#include <poll.h>
#include <sdbusplus/bus.hpp>
#include <systemd/sd-bus.h>

#include <phosphor-logging/lg2.hpp>

#include <csignal>
#include <cstdlib>
#include <cstring>
#include <iostream>

static std::shared_ptr<sdbusplus::bus::bus> g_sdBus;
static bool g_debugFrames = false;
static bool g_verbose = false;
volatile sig_atomic_t g_running = 1;

sd_bus* serialipmi::ipmid_get_sd_bus_connection()
{
    return nullptr;
}
std::shared_ptr<sdbusplus::bus::bus> serialipmi::getSdBus()
{
    return g_sdBus;
}
bool serialipmi::isDebugFrames()
{
    return g_debugFrames;
}
bool serialipmi::isVerbose()
{
    return g_verbose;
}

void signal_handler(int sig)
{
    if (sig == SIGINT || sig == SIGTERM)
    {
        g_running = 0;
    }
}

void print_usage(const char* program)
{
    std::cout << "Usage: " << program << " [OPTIONS]\n";
    std::cout << "\n";
    std::cout << "phosphor-ipmi-serial\n";
    std::cout << "\n";
    std::cout << "Options:\n";
    std::cout << "  -d, --device PATH      Serial device path (default: "
              << SERIAL_DEVICE << ")\n";
    std::cout << "  -b, --baud NUM         Baud rate (default: 115200)\n";
    std::cout << "  -m, --mode MODE        Parser mode (terminal/basic, "
                 "default: terminal)\n";
    std::cout << "  -c, --channel NAME     Channel name (default: serial0)\n";
    std::cout << "  -v, --verbose          Verbose logging\n";
    std::cout << "      --debug-frames     Log raw IPMI frames\n";
    std::cout << "      --session-timeout NUM\n";
    std::cout
        << "                         Session timeout (seconds, default: 60)\n";
    std::cout << "      --max-retries NUM  Maximum retries (default: 3)\n";
    std::cout << "  -h, --help             Show this help message\n";
}

int main(int argc, char** argv)
{
    std::string device{SERIAL_DEVICE};
    int baud = 115200;
    std::string mode{"terminal"};
    std::string channel{"serial0"};
    [[maybe_unused]] int sessionTimeout = 60;
    [[maybe_unused]] int maxRetries = 3;

    static struct option long_options[] = {
        {"device", required_argument, nullptr, 'd'},
        {"baud", required_argument, nullptr, 'b'},
        {"mode", required_argument, nullptr, 'm'},
        {"channel", required_argument, nullptr, 'c'},
        {"verbose", no_argument, nullptr, 'v'},
        {"debug-frames", no_argument, nullptr, 1000},
        {"session-timeout", required_argument, nullptr, 1001},
        {"max-retries", required_argument, nullptr, 1002},
        {"help", no_argument, nullptr, 'h'},
        {nullptr, 0, nullptr, 0}};

    int opt;
    int option_index = 0;
    while ((opt = getopt_long(argc, argv, "d:b:m:c:vh", long_options,
                              &option_index)) != -1)
    {
        switch (opt)
        {
            case 'd':
                device = optarg;
                break;
            case 'b':
                try
                {
                    baud = std::stoi(optarg);
                }
                catch (const std::exception& e)
                {
                    lg2::error("Invalid baud rate: {ERROR}", "ERROR", e.what());
                    return EXIT_FAILURE;
                }
                break;
            case 'm':
                mode = optarg;
                break;
            case 'c':
                channel = optarg;
                break;
            case 'v':
                g_verbose = true;
                break;
            case 1000:
                g_debugFrames = true;
                break;
            case 1001:
                try
                {
                    sessionTimeout = std::stoi(optarg);
                }
                catch (const std::exception& e)
                {
                    lg2::error("Invalid session timeout: {ERROR}", "ERROR",
                               e.what());
                    return EXIT_FAILURE;
                }
                break;
            case 1002:
                try
                {
                    maxRetries = std::stoi(optarg);
                }
                catch (const std::exception& e)
                {
                    lg2::error("Invalid max retries: {ERROR}", "ERROR",
                               e.what());
                    return EXIT_FAILURE;
                }
                break;
            case 'h':
                print_usage(argv[0]);
                return EXIT_SUCCESS;
            case '?':
                return EXIT_FAILURE;
            default:
                abort();
        }
    }

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    try
    {
        g_sdBus = std::make_shared<sdbusplus::bus::bus>(
            sdbusplus::bus::new_default_system());

        auto protocolMode = (mode == "basic")
                                ? serialipmi::parser::ProtocolMode::Basic
                                : serialipmi::parser::ProtocolMode::Terminal;

        serialipmi::session::Manager::get().managerInit(channel);

        serialipmi::command::Table::get().registerSessionCommands();

        auto uartChan = std::make_shared<serialipmi::uart::Channel>(
            device, static_cast<uint32_t>(baud));
        auto parser = serialipmi::parser::createParser(protocolMode);
        auto handler = std::make_shared<serialipmi::message::Handler>(
            uartChan,
            std::shared_ptr<serialipmi::parser::IParser>(std::move(parser)));

        auto& loop = serialipmi::eventloop::EventLoop::getInstance(
            *g_sdBus, *uartChan, handler);
        loop.startEventLoop();
    }
    catch (const std::exception& e)
    {
        lg2::error("Fatal error: {ERROR}", "ERROR", e.what());
        return EXIT_FAILURE;
    }

    return 0;
}
