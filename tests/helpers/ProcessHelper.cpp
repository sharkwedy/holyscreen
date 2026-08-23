#include <chrono>
#include <iostream>
#include <string>
#include <thread>

int main(int argc, char **argv)
{
    if (argc < 2) return 2;

    const std::string mode = argv[1];
    if (mode == "--echo") {
        const std::string value = argc > 2 ? argv[2] : std::string{};
        std::cout << "slide " << value << '\n';
        std::cerr << "erro\n";
        return 0;
    }
    if (mode == "--sleep") {
        const auto milliseconds = argc > 2 ? std::stoi(argv[2]) : 30000;
        std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
        return 0;
    }
    if (mode == "--large-output") {
        const auto bytes = argc > 2 ? std::stoi(argv[2]) : 70000;
        std::cout << std::string(static_cast<std::size_t>(bytes), 'o');
        std::cerr << std::string(static_cast<std::size_t>(bytes), 'e');
        return 0;
    }
    return 3;
}
