#include <condition_variable>
#include <cstdio>
#include <cstring>
#include <map>
#include <memory>
#include <mutex>
#include <semaphore>
#include <string>
#include <vector>

#include "core/memory/memory_manager.h"
#include "libkernel.h"

namespace Core {
namespace Libraries {
namespace Kernel {

static int32_t g_posix_errno = 0;

int32_t *__Error() { return &g_posix_errno; }

void ErrSceToPosix(int32_t error) {
  // TODO: Mapping logic
  g_posix_errno = error;
}

int32_t ErrnoToSceKernelError(int32_t error) {
  // TODO: Mapping logic
  return error;
}

// HLE Function Implementations

int32_t sceKernelError(int32_t posix_error) {
  if (posix_error == 0)
    return 0;
  return posix_error; // + ORBIS_KERNEL_ERROR_UNKNOWN (placeholder)
}

// Stub for sceKernelGetSystemSwVersion
struct SwVersionStruct {
  uint64_t struct_size;
  char text_representation[0x1c];
  uint32_t hex_representation;
};

int32_t sceKernelGetSystemSwVersion(SwVersionStruct *ret) {
  if (!ret)
    return 0;
  ret->hex_representation = 0x11000000; // Firmware 11.00
  std::snprintf(ret->text_representation, 28, "11.00");
  printf("[libkernel] sceKernelGetSystemSwVersion called\n");
  return 0;
}

int32_t sceKernelGetAllowedSdkVersionOnSystem(int32_t *ver) {
  if (!ver)
    return -1;
  *ver = 0x11000000;
  printf("[libkernel] sceKernelGetAllowedSdkVersionOnSystem called: 0x%X\n",
         *ver);
  return 0;
}

// Threading Implementation

struct InternalPthreadAttr {
  size_t stack_size = 1024 * 1024; // 1MB default
};

int32_t scePthreadAttrInit(PthreadAttrT *attr) {
  if (!attr)
    return -1;
  *attr = new InternalPthreadAttr();
  return 0;
}

int32_t scePthreadAttrDestroy(PthreadAttrT *attr) {
  if (!attr || !*attr)
    return -1;
  delete static_cast<InternalPthreadAttr *>(*attr);
  *attr = nullptr;
  return 0;
}

int32_t scePthreadAttrSetstacksize(PthreadAttrT *attr, size_t stacksize) {
  if (!attr || !*attr)
    return -1;
  static_cast<InternalPthreadAttr *>(*attr)->stack_size = stacksize;
  return 0;
}

int32_t scePthreadCreate(PthreadT *thread, const PthreadAttrT *attr,
                         void *(*entry)(void *), void *arg, const char *name) {
  // TODO: Map to KernelManager::CreateThread
  printf("[libkernel] scePthreadCreate called: %s\n", name ? name : "unnamed");
  if (thread)
    *thread = (void *)0x1234; // Dummy handle
  return 0;
}

PthreadT scePthreadSelf() {
  return (PthreadT)0x1234; // Dummy
}

void scePthreadExit(void *value) {
  printf("[libkernel] scePthreadExit called\n");
}

int32_t scePthreadJoin(PthreadT thread, void **value_ptr) {
  printf("[libkernel] scePthreadJoin called\n");
  return 0;
}

// Mutex Implementation

struct InternalMutex {
  std::recursive_mutex mutex;
  std::string name;
};

int32_t scePthreadMutexInit(PthreadMutexT *mutex, const PthreadMutexAttrT *attr,
                            const char *name) {
  if (!mutex)
    return -1;
  auto *m = new InternalMutex();
  if (name)
    m->name = name;
  *mutex = m;
  return 0;
}

int32_t posix_pthread_mutex_init(PthreadMutexT *mutex,
                                 const PthreadMutexAttrT *attr) {
  return scePthreadMutexInit(mutex, attr, nullptr);
}

int32_t posix_pthread_mutex_lock(PthreadMutexT *mutex) {
  if (!mutex || !*mutex)
    return -1;
  static_cast<InternalMutex *>(*mutex)->mutex.lock();
  return 0;
}

int32_t posix_pthread_mutex_unlock(PthreadMutexT *mutex) {
  if (!mutex || !*mutex)
    return -1;
  static_cast<InternalMutex *>(*mutex)->mutex.unlock();
  return 0;
}

int32_t posix_pthread_mutex_destroy(PthreadMutexT *mutex) {
  if (!mutex || !*mutex)
    return -1;
  delete static_cast<InternalMutex *>(*mutex);
  *mutex = nullptr;
  return 0;
}

// Condition Variable Implementation

struct InternalCond {
  std::condition_variable_any cv;
  std::string name;
};

int32_t scePthreadCondInit(PthreadCondT *cond, const PthreadCondAttrT *attr,
                           const char *name) {
  if (!cond)
    return -1;
  auto *c = new InternalCond();
  if (name)
    c->name = name;
  *cond = c;
  return 0;
}

int32_t posix_pthread_cond_init(PthreadCondT *cond,
                                const PthreadCondAttrT *attr) {
  return scePthreadCondInit(cond, attr, nullptr);
}

int32_t posix_pthread_cond_wait(PthreadCondT *cond, PthreadMutexT *mutex) {
  if (!cond || !*cond || !mutex || !*mutex)
    return -1;
  auto *c = static_cast<InternalCond *>(*cond);
  auto *m = static_cast<InternalMutex *>(*mutex);
  c->cv.wait(m->mutex);
  return 0;
}

int32_t posix_pthread_cond_signal(PthreadCondT *cond) {
  if (!cond || !*cond)
    return -1;
  static_cast<InternalCond *>(*cond)->cv.notify_one();
  return 0;
}

int32_t posix_pthread_cond_broadcast(PthreadCondT *cond) {
  if (!cond || !*cond)
    return -1;
  static_cast<InternalCond *>(*cond)->cv.notify_all();
  return 0;
}

int32_t posix_pthread_cond_destroy(PthreadCondT *cond) {
  if (!cond || !*cond)
    return -1;
  delete static_cast<InternalCond *>(*cond);
  *cond = nullptr;
  return 0;
}

// Event Flag Implementation

class EventFlagInternal {
public:
  enum class WaitMode { And, Or };
  enum class ClearMode { None, All, Bits };

  EventFlagInternal(const std::string &name, uint64_t bits)
      : m_name(name), m_bits(bits) {}

  int Wait(uint64_t bits, WaitMode wait_mode, ClearMode clear_mode,
           uint64_t *result, uint32_t *timeout) {
    std::unique_lock<std::mutex> lock(m_mutex);

    auto wait_func = [&] {
      if (wait_mode == WaitMode::And)
        return (m_bits & bits) == bits;
      return (m_bits & bits) != 0;
    };

    if (timeout) {
      if (!m_cond.wait_for(
              lock, std::chrono::microseconds(static_cast<long long>(*timeout)),
              wait_func)) {
        if (result)
          *result = m_bits;
        return -1; // ETIMEDOUT
      }
    } else {
      m_cond.wait(lock, wait_func);
    }

    if (result)
      *result = m_bits;
    if (clear_mode == ClearMode::All)
      m_bits = 0;
    else if (clear_mode == ClearMode::Bits)
      m_bits &= ~bits;

    return 0;
  }

  void Set(uint64_t bits) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_bits |= bits;
    m_cond.notify_all();
  }

  void Clear(uint64_t bits) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_bits &= bits;
  }

private:
  std::string m_name;
  uint64_t m_bits;
  std::mutex m_mutex;
  std::condition_variable m_cond;
};

int32_t
sceKernelCreateEventFlag(OrbisKernelEventFlag *ef, const char *pName,
                         uint32_t attr, uint64_t initPattern,
                         const OrbisKernelEventFlagOptParam *pOptParam) {
  if (!ef)
    return -1;
  *ef = new EventFlagInternal(pName ? pName : "unnamed", initPattern);
  return 0;
}

int32_t sceKernelDeleteEventFlag(OrbisKernelEventFlag ef) {
  if (!ef)
    return -1;
  delete static_cast<EventFlagInternal *>(ef);
  return 0;
}

int32_t sceKernelSetEventFlag(OrbisKernelEventFlag ef, uint64_t bitPattern) {
  if (!ef)
    return -1;
  static_cast<EventFlagInternal *>(ef)->Set(bitPattern);
  return 0;
}

int32_t sceKernelClearEventFlag(OrbisKernelEventFlag ef, uint64_t bitPattern) {
  if (!ef)
    return -1;
  static_cast<EventFlagInternal *>(ef)->Clear(bitPattern);
  return 0;
}

int32_t sceKernelWaitEventFlag(OrbisKernelEventFlag ef, uint64_t bitPattern,
                               uint32_t waitMode, uint64_t *pResultPat,
                               uint32_t *pTimeout) {
  if (!ef)
    return -1;
  auto wait = (waitMode & 0x01) ? EventFlagInternal::WaitMode::And
                                : EventFlagInternal::WaitMode::Or;
  auto clear = EventFlagInternal::ClearMode::None;
  if (waitMode & 0x10)
    clear = EventFlagInternal::ClearMode::All;
  else if (waitMode & 0x20)
    clear = EventFlagInternal::ClearMode::Bits;

  return static_cast<EventFlagInternal *>(ef)->Wait(bitPattern, wait, clear,
                                                    pResultPat, pTimeout);
}

int32_t sceKernelPollEventFlag(OrbisKernelEventFlag ef, uint64_t bitPattern,
                               uint32_t waitMode, uint64_t *pResultPat) {
  uint32_t timeout = 0;
  return sceKernelWaitEventFlag(ef, bitPattern, waitMode, pResultPat, &timeout);
}

int32_t sceKernelCancelEventFlag(OrbisKernelEventFlag ef, uint64_t setPattern,
                                 int32_t *pNumWaitThreads) {
  // Basic cancel: set pattern and notify all
  if (!ef)
    return -1;
  static_cast<EventFlagInternal *>(ef)->Set(setPattern);
  return 0;
}

// Memory Management HLE

uint64_t sceKernelGetDirectMemorySize() {
  return ::Core::Memory::MemoryManager::GetInstance()->GetTotalDirectSize();
}

int32_t sceKernelAllocateDirectMemory(int64_t searchStart, int64_t searchEnd,
                                      uint64_t len, uint64_t alignment,
                                      int32_t memoryType,
                                      int64_t *physAddrOut) {
  auto *mm = ::Core::Memory::MemoryManager::GetInstance();
  uint64_t addr =
      mm->Allocate(searchStart, searchEnd, len, alignment, memoryType);
  if (addr == (uint64_t)-1)
    return -1;
  if (physAddrOut)
    *physAddrOut = addr;
  return 0;
}

int32_t sceKernelAllocateMainDirectMemory(uint64_t len, uint64_t alignment,
                                          int32_t memoryType,
                                          int64_t *physAddrOut) {
  return sceKernelAllocateDirectMemory(0, sceKernelGetDirectMemorySize(), len,
                                       alignment, memoryType, physAddrOut);
}

int32_t sceKernelReleaseDirectMemory(uint64_t start, uint64_t len) {
  return ::Core::Memory::MemoryManager::GetInstance()->Free(start, len, false);
}

int32_t sceKernelMapNamedDirectMemory(void **addr, uint64_t len, int32_t prot,
                                      int32_t flags, int64_t phys_addr,
                                      uint64_t alignment, const char *name) {
  auto *mm = ::Core::Memory::MemoryManager::GetInstance();
  return mm->MapMemory(addr, addr ? reinterpret_cast<uintptr_t>(*addr) : 0, len,
                       prot, flags, 0 /* Direct */, name ? name : "anon", true,
                       phys_addr, alignment);
}

int32_t sceKernelMapDirectMemory(void **addr, uint64_t len, int32_t prot,
                                 int32_t flags, int64_t phys_addr,
                                 uint64_t alignment) {
  return sceKernelMapNamedDirectMemory(addr, len, prot, flags, phys_addr,
                                       alignment, "anon");
}

int32_t sceKernelReserveVirtualRange(void **addr, uint64_t len, int32_t flags,
                                     uint64_t alignment) {
  auto *mm = ::Core::Memory::MemoryManager::GetInstance();
  return mm->MapMemory(addr, addr ? reinterpret_cast<uintptr_t>(*addr) : 0, len,
                       0 /* NoAccess */, flags, 1 /* Reserved */, "anon", true,
                       -1, alignment);
}

int32_t sceKernelVirtualQuery(const void *addr, int32_t flags,
                              OrbisVirtualQueryInfo *info, uint64_t infoSize) {
  auto *mm = ::Core::Memory::MemoryManager::GetInstance();
  return mm->VirtualQuery(reinterpret_cast<uintptr_t>(addr), flags, info);
}

int32_t sceKernelMunmap(void *addr, uint64_t len) {
  return ::Core::Memory::MemoryManager::GetInstance()->UnmapMemory(
      reinterpret_cast<uintptr_t>(addr), len);
}

int32_t sceKernelMprotect(const void *addr, uint64_t size, int32_t prot) {
  return ::Core::Memory::MemoryManager::GetInstance()->Protect(
      reinterpret_cast<uintptr_t>(addr), size, prot);
}

// Semaphore Implementation

struct InternalSema {
  std::counting_semaphore<0x7FFFFFFF> sem;
  int32_t max_count;

  InternalSema(int32_t init, int32_t max) : sem(init), max_count(max) {}
};

static std::map<OrbisKernelSema, std::unique_ptr<InternalSema>> g_semaphores;
static std::mutex g_sema_mutex;
static OrbisKernelSema g_next_sema = 1;

int32_t sceKernelCreateSema(OrbisKernelSema *sem, const char *name,
                            uint32_t attr, int32_t initCount, int32_t maxCount,
                            const void *pOptParam) {
  printf("[libkernel] sceKernelCreateSema called: %s (init: %d, max: %d)\n",
         name ? name : "unnamed", initCount, maxCount);

  std::lock_guard<std::mutex> lock(g_sema_mutex);
  OrbisKernelSema handle = g_next_sema++;
  g_semaphores[handle] = std::make_unique<InternalSema>(initCount, maxCount);

  if (sem)
    *sem = handle;
  return 0;
}

int32_t sceKernelWaitSema(OrbisKernelSema sem, int32_t needCount,
                          uint32_t *pTimeout) {
  InternalSema *s = nullptr;
  {
    std::lock_guard<std::mutex> lock(g_sema_mutex);
    auto it = g_semaphores.find(sem);
    if (it == g_semaphores.end())
      return -1;
    s = it->second.get();
  }

  // Basic implementation (doesn't handle needCount > 1 correctly with
  // std::counting_semaphore easily)
  for (int i = 0; i < needCount; ++i) {
    s->sem.acquire();
  }
  return 0;
}

int32_t sceKernelSignalSema(OrbisKernelSema sem, int32_t signalCount) {
  InternalSema *s = nullptr;
  {
    std::lock_guard<std::mutex> lock(g_sema_mutex);
    auto it = g_semaphores.find(sem);
    if (it == g_semaphores.end())
      return -1;
    s = it->second.get();
  }

  s->sem.release(signalCount);
  return 0;
}

// Registration function
void RegisterLibKernel(::Core::Kernel::ModuleManager *module_manager) {
  printf("[libkernel] Registering HLE functions...\n");

  // Core
  LIB_FUNCTION("D4yla3vx4tY", "libkernel", 1, "libkernel", sceKernelError);
  LIB_FUNCTION("Mv1zUObHvXI", "libkernel", 1, "libkernel",
               sceKernelGetSystemSwVersion);
  LIB_FUNCTION("YeU23Szo3BM", "libkernel", 1, "libkernel",
               sceKernelGetAllowedSdkVersionOnSystem);
  LIB_FUNCTION("9BcDykPmo1I", "libkernel", 1, "libkernel", __Error);

  // Macros to simplify registration
#undef LIB_FUNCTION
#define LIB_FUNCTION(nid, library, version, module, function)                  \
  module_manager->RegisterHLEExport(module, nid, #function,                    \
                                    reinterpret_cast<uint64_t>(function));

  // Threading Attributes
  LIB_FUNCTION("nsYoNRywwNg", "libkernel", 1, "libkernel", scePthreadAttrInit);
  LIB_FUNCTION("62KCwEMmzcM", "libkernel", 1, "libkernel",
               scePthreadAttrDestroy);
  LIB_FUNCTION("UTXzJbWhhTE", "libkernel", 1, "libkernel",
               scePthreadAttrSetstacksize);

  // Threading
  LIB_FUNCTION("6UgtwV+0zb4", "libkernel", 1, "libkernel", scePthreadCreate);
  LIB_FUNCTION("aI+OeCz8xrQ", "libkernel", 1, "libkernel", scePthreadSelf);
  LIB_FUNCTION("3kg7rT0NQIs", "libkernel", 1, "libkernel", scePthreadExit);
  LIB_FUNCTION("onNY9Byn-W8", "libkernel", 1, "libkernel", scePthreadJoin);

  // Mutexes
  LIB_FUNCTION("cmo1RIYva9o", "libkernel", 1, "libkernel", scePthreadMutexInit);
  LIB_FUNCTION("qH1gXoq71RY", "libkernel", 1, "libkernel",
               posix_pthread_mutex_init);
  LIB_FUNCTION("9UK1vLZQft4", "libkernel", 1, "libkernel",
               posix_pthread_mutex_lock);
  LIB_FUNCTION("tn3VlD0hG60", "libkernel", 1, "libkernel",
               posix_pthread_mutex_unlock);
  LIB_FUNCTION("2Of0f+3mhhE", "libkernel", 1, "libkernel",
               posix_pthread_mutex_destroy);
  LIB_FUNCTION("ttHNfU+qDBU", "libScePosix", 1, "libkernel",
               posix_pthread_mutex_init);
  LIB_FUNCTION("7H0iTOciTLo", "libScePosix", 1, "libkernel",
               posix_pthread_mutex_lock);
  LIB_FUNCTION("2Z+PpY6CaJg", "libScePosix", 1, "libkernel",
               posix_pthread_mutex_unlock);

  // Condition Variables
  LIB_FUNCTION("2Tb92quprl0", "libkernel", 1, "libkernel", scePthreadCondInit);
  LIB_FUNCTION("0TyVk4MSLt0", "libkernel", 1, "libkernel",
               posix_pthread_cond_init);
  LIB_FUNCTION("WKAXJ4XBPQ4", "libkernel", 1, "libkernel",
               posix_pthread_cond_wait);
  LIB_FUNCTION("kDh-NfxgMtE", "libkernel", 1, "libkernel",
               posix_pthread_cond_signal);
  LIB_FUNCTION("JGgj7Uvrl+A", "libkernel", 1, "libkernel",
               posix_pthread_cond_broadcast);
  LIB_FUNCTION("g+PZd2hiacg", "libkernel", 1, "libkernel",
               posix_pthread_cond_destroy);
  LIB_FUNCTION("mkx2fVhNMsg", "libScePosix", 1, "libkernel",
               posix_pthread_cond_broadcast);
  LIB_FUNCTION("2MOy+rUfuhQ", "libScePosix", 1, "libkernel",
               posix_pthread_cond_signal);

  // Semaphores
  LIB_FUNCTION("188x57JYp0g", "libkernel", 1, "libkernel", sceKernelCreateSema);
  LIB_FUNCTION("Zxa0VhQVTsk", "libkernel", 1, "libkernel", sceKernelWaitSema);
  LIB_FUNCTION("4czppHBiriw", "libkernel", 1, "libkernel", sceKernelSignalSema);

  // Event Flags
  LIB_FUNCTION("PZku4ZrXJqg", "libkernel", 1, "libkernel",
               sceKernelCancelEventFlag);
  LIB_FUNCTION("7uhBFWRAS60", "libkernel", 1, "libkernel",
               sceKernelClearEventFlag);
  LIB_FUNCTION("BpFoboUJoZU", "libkernel", 1, "libkernel",
               sceKernelCreateEventFlag);
  LIB_FUNCTION("8mql9OcQnd4", "libkernel", 1, "libkernel",
               sceKernelDeleteEventFlag);
  LIB_FUNCTION("9lvj5DjHZiA", "libkernel", 1, "libkernel",
               sceKernelPollEventFlag);
  LIB_FUNCTION("IOnSvHzqu6A", "libkernel", 1, "libkernel",
               sceKernelSetEventFlag);
  LIB_FUNCTION("JTvBflhYazQ", "libkernel", 1, "libkernel",
               sceKernelWaitEventFlag);

  // Memory Management
  LIB_FUNCTION("rTXw65xmLIA", "libkernel", 1, "libkernel",
               sceKernelAllocateDirectMemory);
  LIB_FUNCTION("B+vc2AO2Zrc", "libkernel", 1, "libkernel",
               sceKernelAllocateMainDirectMemory);
  LIB_FUNCTION("rVjRvHJ0X6c", "libkernel", 1, "libkernel",
               sceKernelVirtualQuery);
  LIB_FUNCTION("7oxv3PPCumo", "libkernel", 1, "libkernel",
               sceKernelReserveVirtualRange);
  LIB_FUNCTION("pO96TwzOm5E", "libkernel", 1, "libkernel",
               sceKernelGetDirectMemorySize);
  LIB_FUNCTION("NcaWUxfMNIQ", "libkernel", 1, "libkernel",
               sceKernelMapNamedDirectMemory);
  LIB_FUNCTION("L-Q3LEjIbgA", "libkernel", 1, "libkernel",
               sceKernelMapDirectMemory);
  LIB_FUNCTION("MBuItvba6z8", "libkernel", 1, "libkernel",
               sceKernelReleaseDirectMemory);
  LIB_FUNCTION("cQke9UuBQOk", "libkernel", 1, "libkernel", sceKernelMunmap);
  LIB_FUNCTION("vSMAm3cxYTY", "libkernel", 1, "libkernel", sceKernelMprotect);

  printf("[libkernel] Registration complete.\n");
}

} // namespace Kernel
} // namespace Libraries
} // namespace Core
