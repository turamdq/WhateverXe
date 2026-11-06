#include "kern_fb_controller_lifecycle.hpp"
#include <atomic>
#include <cassert>
#include <thread>
#include <vector>
#include <cstdio>

static void write(FBControllerLifecycle::Trace &trace) {
 const auto sequence=trace.reserve();
 if(!sequence)return;
 uint64_t data[FBControllerLifecycle::Words] {};
 for(unsigned i=0;i<FBControllerLifecycle::Words;++i)data[i]=sequence^(0xA5A50000ULL+i);
 trace.publish(sequence,data);
}
static void check(const uint64_t *snapshot) {
 assert(snapshot[0]==1);
 for(unsigned slot=0;slot<FBControllerLifecycle::Slots;++slot) {
  const auto row=snapshot+3+slot*(FBControllerLifecycle::Words+1);
  if(!row[0])continue;
  const auto sequence=slot+1;
  for(unsigned word=0;word<FBControllerLifecycle::Words;++word)
   assert(row[word+1]==(sequence^(0xA5A50000ULL+word)));
 }
}
int main() {
 FBControllerLifecycle::Trace trace;
 uint64_t out[3+FBControllerLifecycle::Slots*(FBControllerLifecycle::Words+1)] {};
 const auto first=trace.reserve();assert(first==1);
 trace.snapshot(out);assert(out[0]==1 && out[1]==1 && out[2]==0);
 assert(out[3]==0); // an unfinished reservation is skipped
 uint64_t firstData[FBControllerLifecycle::Words] {};
 for(unsigned i=0;i<FBControllerLifecycle::Words;++i)firstData[i]=first^(0xA5A50000ULL+i);
 trace.publish(first,firstData);

 std::atomic<bool> done {false};
 std::thread reader([&]{
  while(!done.load(std::memory_order_acquire)) {trace.snapshot(out);check(out);}
 });
 std::vector<std::thread> workers;
 for(unsigned n=0;n<8;++n)workers.emplace_back([&]{for(unsigned i=0;i<200;++i)write(trace);});
 for(auto &worker:workers)worker.join();
 done.store(true,std::memory_order_release);reader.join();
 trace.snapshot(out);check(out);
 assert(out[1]==1601 && out[2]==1601-FBControllerLifecycle::Slots);
 for(unsigned slot=0;slot<FBControllerLifecycle::Slots;++slot)
  assert(out[3+slot*(FBControllerLifecycle::Words+1)]==1);
 puts("framebuffer lifecycle trace: ordered bounded slots, concurrent snapshots and overflow accounting passed");
}
