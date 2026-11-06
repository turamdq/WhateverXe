#pragma once
#include <stdint.h>

// Bounded, append-only history for framebuffer/provider lifecycle callbacks.
// Snapshot header: schema, reservations, dropped events. Each slot is ready
// followed by data[Words]: sequence, FNV-1a key hash, stage, uptime ns, thread,
// object/id/provider/id, argument-provider/id, tracked/id/provider/id,
// retained/id/provider/id, outcome. Events beyond capacity are counted.
namespace FBControllerLifecycle {
constexpr unsigned Slots=32, Words=20;
struct Slot { uint32_t state {}; uint64_t data[Words] {}; };
struct Trace {
 Slot slots[Slots] {};
 uint32_t count {}, dropped {};

 uint32_t reserve() {
  const auto sequence=__atomic_add_fetch(&count,1,__ATOMIC_RELAXED);
  if(sequence<=Slots)return sequence;
  __atomic_add_fetch(&dropped,1,__ATOMIC_RELAXED);
  return 0;
 }
 void publish(uint32_t sequence,const uint64_t *data) {
  if(!sequence || sequence>Slots || !data)return;
  auto &slot=slots[sequence-1];
  for(unsigned i=0;i<Words;++i)slot.data[i]=data[i];
  __atomic_store_n(&slot.state,2,__ATOMIC_RELEASE);
 }
 void snapshot(uint64_t *out) const {
  if(!out)return;
  out[0]=1;
  out[1]=__atomic_load_n(&count,__ATOMIC_ACQUIRE);
  out[2]=__atomic_load_n(&dropped,__ATOMIC_RELAXED);
  for(unsigned i=0;i<Slots;++i) {
   auto p=out+3+i*(Words+1);
   const auto &slot=slots[i];
   const bool ready=__atomic_load_n(&slot.state,__ATOMIC_ACQUIRE)==2;
   p[0]=ready?1:0;
   for(unsigned j=0;j<Words;++j)p[j+1]=ready?slot.data[j]:0;
  }
 }
};
}
