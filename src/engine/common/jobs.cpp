#include "jobs.h"

job_pool::job_pool(int nthreads, world *w_) : num_threads(nthreads), w(w_) {
  for (int i = 0; i < nthreads; ++i) {
    workers[i] = std::thread(do_work, this, i);
  }
}
void job_pool::do_work(job_pool *thisptr, int tid) {
  job *next_job = nullptr;
  while (true) {
    if (thisptr->q.try_dequeue(next_job)) {
      next_job->run(thisptr->w, tid);
    }
  }
}
void job_pool::add_job(job *j) { q.enqueue(j); }
