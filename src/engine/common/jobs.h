#include "common/constants.h"
#include "extern/concurrentqueue/concurrentqueue.h"
#include <thread>
class world;
class job {
public:
  virtual void run(world *w, int tid) = 0;
};
class job_pool {
private:
  world *w;
  std::thread workers[constants::MAX_CONCURRENCY];
  moodycamel::ConcurrentQueue<job *> q;
  int num_threads;

public:
  job_pool(int nthreads, world *w_);
  static void do_work(job_pool *jp, int tid);
  void add_job(job *j);
};
