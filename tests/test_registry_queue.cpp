#include "../onx_registry_queue.h"
#include <cassert>
#include <string>
#include <thread>
int main() {
  onx_registry::PayloadQueue<std::string, std::mutex, 2> queue;
  std::string a="first",b="second",c="third",out;
  assert(queue.put(a) && a.empty()); assert(queue.put(b));
  assert(!queue.put(c) && c=="third");
  assert(queue.take(out) && out=="first"); out.clear();
  assert(queue.take(out) && out=="second"); out.clear();
  assert(!queue.take(out));
  std::thread producer([&] {
    for (int i=0;i<10000;++i) {
      auto text=std::to_string(i);
      while (!queue.put(text)) std::this_thread::yield();
    }
  });
  for (int i=0;i<10000;++i) {
    while (!queue.take(out)) std::this_thread::yield();
    assert(out==std::to_string(i)); out.clear();
  }
  producer.join();
  assert(queue.put(c)); queue.clear(); assert(!queue.take(out));
}
