#pragma once

int run_threads(int amount_consumers, int sleep_time_mills,
                bool debug_flag = false, std::istream &src = std::cin);
int get_tid(int n);