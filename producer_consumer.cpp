#include <pthread.h>
#include <unistd.h>
#include <iostream>
#include <vector>

class func_consumer_params {
 public:
  int *val;
  bool &ready;
  bool *finish;
  int sleep_time_max;
  bool is_debug;
  int n;
};

class func_producer_params {
 public:
  int *val;
  bool &ready;
  bool *finish;
  pthread_t interrupter;
  std::istream &src;
};

class func_interrupter_params {
 public:
  int amount_of_consumers;
  std::vector<pthread_t> &consumers;

  int get_random_consumer() const { return std::rand() % amount_of_consumers; }
};

pthread_mutex_t ready_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t finish_mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t cv_producer = PTHREAD_COND_INITIALIZER;
pthread_cond_t cv_consumer = PTHREAD_COND_INITIALIZER;

int get_tid(int n) {
  // 1 to 3+N thread ID
  static std::vector<int> threads(n + 3, 0);
  static int running_index = 0;
  static thread_local int *curr_tid = nullptr;

  if (n == 0) {
    running_index = 0;
    curr_tid = nullptr;
    return 0;
  }
  if (curr_tid == nullptr) {
    threads[running_index] = running_index + 1;
    curr_tid = &(*(threads.begin() + running_index));
    running_index++;
  }
  return *curr_tid;
}

void *producer_routine(void *arg) {
  auto *params = (func_producer_params *)arg;
  // read data, loop through each value and update the value, notify consumer,
  // wait for consumer to process

  while (params->src >> *params->val) {
    pthread_mutex_lock(&ready_mutex);
    params->ready = true;
    pthread_cond_signal(&cv_consumer);
    while (params->ready) {
      pthread_cond_wait(&cv_producer, &ready_mutex);
    }
    pthread_mutex_unlock(&ready_mutex);
  }

  pthread_mutex_lock(&finish_mutex);
  *params->finish = true;
  pthread_cond_broadcast(&cv_consumer);
  pthread_cancel(params->interrupter);
  pthread_mutex_unlock(&finish_mutex);
  return nullptr;
}

bool get_finish(bool *finish) {
  bool res;
  pthread_mutex_lock(&finish_mutex);
  res = *finish;
  pthread_mutex_unlock(&finish_mutex);
  return res;
}

void *consumer_routine(void *arg) {
  // for every update issued by producer, read the value and add to sum
  // return pointer to result (for particular consumer)

  pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, nullptr);
  auto *params = (func_consumer_params *)arg;
  long long *sum = new long long();

  while (!get_finish(params->finish)) {
    pthread_mutex_lock(&ready_mutex);
    while (!params->ready && !get_finish(params->finish)) {
      pthread_cond_wait(&cv_consumer, &ready_mutex);
    }
    if (params->ready) {
      *sum += *params->val;
      params->ready = false;
      pthread_cond_signal(&cv_producer);
    }
    pthread_mutex_unlock(&ready_mutex);

    if (params->sleep_time_max > 0) {
      int sleep_time = std::rand() % params->sleep_time_max;
      usleep(sleep_time * 1000);
    }
    if (params->is_debug) {
      std::cout << get_tid(params->n) << ":" << *sum << std::endl;
    }
  }
  return sum;
}

void *consumer_interruptor_routine(void *arg) {
  (void)arg;
  // interrupt random consumer while producer is running
  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, nullptr);
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, nullptr);

  while (true) {
    auto *params = (func_interrupter_params *)arg;
    pthread_t t_rand = params->consumers[params->get_random_consumer()];

    pthread_cancel(t_rand);
    //  std::cout << "int" << std::endl;
    // std::cout.flush();
  }

  return nullptr;
}

// the declaration of run threads can be changed as you like
int run_threads(int amount_consumers, int sleep_time_mills,
                bool debug_flag = false, std::istream &src = std::cin) {
  // start N threads and wait until they're done
  // return aggregated sum of values

  int input = 0;
  bool ready = false;
  bool finish = false;

  std::vector<pthread_t> consumers(amount_consumers);
  func_consumer_params params = {
      &input, ready, &finish, sleep_time_mills, debug_flag, amount_consumers};

  for (int i = 0; i < amount_consumers; i++) {
    if (pthread_create(&consumers[i], nullptr, consumer_routine, &params) !=
        0) {
      std::cout << "couldn't create thread: " << i + 1 << std::endl;
      exit(-1);
    }
  }

  pthread_t interruptor;
  func_interrupter_params in_params = {amount_consumers, consumers};

  if (pthread_create(&interruptor, nullptr, consumer_interruptor_routine,
                     &in_params) != 0) {
    std::cout << "couldn't create producer thread " << std::endl;
    exit(-1);
  }

  pthread_t producer;
  func_producer_params pr_params = {&input, ready, &finish, interruptor, src};

  if (pthread_create(&producer, nullptr, producer_routine, &pr_params) != 0) {
    std::cout << "couldn't create producer thread " << std::endl;
    exit(-1);
  }

  pthread_join(producer, nullptr);

  int sum = 0;
  for (int i = 0; i < amount_consumers; i++) {
    void *a = nullptr;
    pthread_join(consumers[i], &a);
    sum += *((int *)(a));
    free(a);
  }

  return sum;
}
