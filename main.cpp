#include <iostream>
#include <string>
#include "producer_consumer.h"

int main(int argc, const char **argv) {
  if (argc == 3) {
    int amount_of_threads = std::atoi(argv[1]);
    int sleep_time = std::atoi(argv[2]);
    std::cout << run_threads(amount_of_threads, sleep_time);
    return 0;
  }

  if (argc == 4) {
    std::string param = argv[3];
    if (param != "-debug") {
      exit(-1);
    }
    int amount_of_threads = std::atoi(argv[1]);
    int sleep_time = std::atoi(argv[2]);
    std::cout << run_threads(amount_of_threads, sleep_time, true);
    return 0;
  }

  return 0;
}
