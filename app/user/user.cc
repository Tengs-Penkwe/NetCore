#include <iostream>
#include <chrono>
#include <thread>
#include <interface/rpc.h>

void* test_handler (void* arg) {
    (void) arg;
    std::cout << "test_handler" << std::endl;
    return nullptr;
}

int main(int argc, char* argv[]) {
    (void) argc;
    (void) argv;
    std::cout << "Hello, World!" << std::endl;

    // sleep 10 seconds
    std::this_thread::sleep_for(std::chrono::seconds(3));

    rpc_t rpc;
    rpc_id_t id = {"test"};
    establish_rpc(&rpc, RPC_LOCL, id, nullptr);

    std::this_thread::sleep_for(std::chrono::seconds(10));
    return 0;
}
