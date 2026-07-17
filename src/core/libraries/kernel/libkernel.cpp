#include <condition_variable>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <map>
#include <memory>
#include <mutex>
#include <semaphore>
#include <string>
#include <vector>

#include "common/io_file.h"
#include "core/file_sys/fs.h"
#include "core/libraries/kernel/equeue.h"
#include "core/libraries/kernel/process.h"
#include "core/libraries/kernel/time.h"
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

// FileSystem static pointers (set during init)
static Core::FileSys::MntPoints *g_mnt_points = nullptr;
static Core::FileSys::HandleTable *g_handle_table = nullptr;

void SetFileSysPointers(void *mnt, void *htab) {
  g_mnt_points = static_cast<Core::FileSys::MntPoints *>(mnt);
  g_handle_table = static_cast<Core::FileSys::HandleTable *>(htab);
  printf("[libkernel] FileSystem pointers set (mnt=%p, htab=%p)\n", mnt, htab);
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

// FileSystem HLE Implementation

int32_t sceKernelOpen(const char *path, int32_t flags, uint16_t mode) {
  if (!path || !g_mnt_points || !g_handle_table) {
    printf("[libkernel] sceKernelOpen failed: null pointer\n");
    return -1;
  }

  printf("[libkernel] sceKernelOpen: %s flags=0x%x mode=0x%x\n", path, flags,
         mode);

  bool read = (flags & 0x3) == ORBIS_KERNEL_O_RDONLY;
  bool write = (flags & 0x3) == ORBIS_KERNEL_O_WRONLY;
  bool rdwr = (flags & 0x3) == ORBIS_KERNEL_O_RDWR;
  bool create = (flags & ORBIS_KERNEL_O_CREAT) != 0;
  bool trunc = (flags & ORBIS_KERNEL_O_TRUNC) != 0;
  bool append = (flags & ORBIS_KERNEL_O_APPEND) != 0;
  bool directory = (flags & ORBIS_KERNEL_O_DIRECTORY) != 0;

  // Translate guest path to host path
  bool read_only = false;
  auto host_path = g_mnt_points->GetHostPath(path, &read_only);

  if (host_path.empty()) {
    printf("[libkernel] sceKernelOpen: no mount point for '%s'\n", path);
    *__Error() = 2; // ENOENT
    return -1;
  }

  bool exists = std::filesystem::exists(host_path);

  if (create && !exists) {
    if (read_only) {
      *__Error() = 30; // EROFS
      return -1;
    }
    // Create the file
    Common::IO::File creator(host_path, Common::IO::FileAccessMode::Create);
  } else if (!exists && !create) {
    printf("[libkernel] sceKernelOpen: file not found '%s'\n", path);
    *__Error() = 2; // ENOENT
    return -1;
  }

  if (directory || std::filesystem::is_directory(host_path)) {
    // Directory open - create handle but don't open file
    int fd = g_handle_table->CreateHandle();
    auto *file = g_handle_table->GetFile(fd);
    if (file) {
      file->is_opened = true;
      file->type = Core::FileSys::FileType::Directory;
      file->m_host_name = host_path;
      file->m_guest_name = path;
    }
    printf("[libkernel] sceKernelOpen: opened directory fd=%d\n", fd);
    return fd;
  }

  // Regular file
  int fd = g_handle_table->CreateHandle();
  auto *file = g_handle_table->GetFile(fd);
  if (!file) {
    return -1;
  }

  file->m_host_name = host_path;
  file->m_guest_name = path;
  file->type = Core::FileSys::FileType::Regular;
  file->f = std::make_unique<Common::IO::File>();

  Common::IO::FileAccessMode access_mode = Common::IO::FileAccessMode::Read;
  if (read) {
    access_mode = Common::IO::FileAccessMode::Read;
  } else if (write) {
    access_mode = append ? Common::IO::FileAccessMode::Append
                         : Common::IO::FileAccessMode::Write;
  } else if (rdwr) {
    access_mode = append ? Common::IO::FileAccessMode::ReadAppend
                         : Common::IO::FileAccessMode::ReadWrite;
  }

  if (trunc) {
    access_mode = Common::IO::FileAccessMode::Create; // w+b truncates
  }

  int err = file->f->Open(host_path, access_mode);
  if (err != 0) {
    printf("[libkernel] sceKernelOpen: failed to open '%s' err=%d\n", path,
           err);
    g_handle_table->DeleteHandle(fd);
    *__Error() = err;
    return -1;
  }

  file->is_opened = true;
  printf("[libkernel] sceKernelOpen: success fd=%d\n", fd);
  return fd;
}

int32_t sceKernelClose(int32_t fd) {
  if (!g_handle_table)
    return -1;
  printf("[libkernel] sceKernelClose: fd=%d\n", fd);

  auto *file = g_handle_table->GetFile(fd);
  if (!file || !file->is_opened) {
    *__Error() = 9; // EBADF
    return -1;
  }

  if (file->f) {
    file->f->Close();
  }
  file->is_opened = false;
  g_handle_table->DeleteHandle(fd);
  return 0;
}

int64_t sceKernelRead(int32_t fd, void *buf, uint64_t nbytes) {
  if (!g_handle_table || !buf)
    return -1;

  auto *file = g_handle_table->GetFile(fd);
  if (!file || !file->is_opened) {
    *__Error() = 9; // EBADF
    return -1;
  }

  if (file->type == Core::FileSys::FileType::Device) {
    // Stdin reads return 0 (EOF)
    return 0;
  }

  if (!file->f || !file->f->IsOpen()) {
    *__Error() = 9;
    return -1;
  }

  size_t bytes_read = file->f->Read(buf, static_cast<size_t>(nbytes));
  return static_cast<int64_t>(bytes_read);
}

int64_t sceKernelWrite(int32_t fd, const void *buf, uint64_t nbytes) {
  if (!g_handle_table || !buf)
    return -1;

  auto *file = g_handle_table->GetFile(fd);
  if (!file || !file->is_opened) {
    *__Error() = 9;
    return -1;
  }

  // stdout / stderr → redirect to host console
  if (file->type == Core::FileSys::FileType::Device) {
    if (file->m_guest_name == "/dev/stdout" ||
        file->m_guest_name == "/dev/stderr") {
      fwrite(buf, 1, static_cast<size_t>(nbytes),
             file->m_guest_name == "/dev/stdout" ? stdout : stderr);
      return static_cast<int64_t>(nbytes);
    }
    return static_cast<int64_t>(nbytes); // Consume silently for other devices
  }

  if (!file->f || !file->f->IsOpen()) {
    *__Error() = 9;
    return -1;
  }

  size_t bytes_written = file->f->Write(buf, static_cast<size_t>(nbytes));
  return static_cast<int64_t>(bytes_written);
}

int64_t sceKernelLseek(int32_t fd, int64_t offset, int32_t whence) {
  if (!g_handle_table)
    return -1;

  auto *file = g_handle_table->GetFile(fd);
  if (!file || !file->is_opened || !file->f) {
    *__Error() = 9;
    return -1;
  }

  return file->f->Seek(offset, whence);
}

static void FillStat(OrbisKernelStat *sb, const std::filesystem::path &path) {
  std::memset(sb, 0, sizeof(OrbisKernelStat));
  std::error_code ec;
  auto status = std::filesystem::status(path, ec);
  if (ec)
    return;

  if (std::filesystem::is_regular_file(status)) {
    sb->st_mode = 0100644; // S_IFREG | 0644
    sb->st_size = static_cast<int64_t>(std::filesystem::file_size(path, ec));
  } else if (std::filesystem::is_directory(status)) {
    sb->st_mode = 0040755; // S_IFDIR | 0755
    sb->st_size = 0;
  }

  sb->st_blksize = 512;
  sb->st_blocks = (sb->st_size + 511) / 512;
  sb->st_nlink = 1;

  // File times
  auto ftime = std::filesystem::last_write_time(path, ec);
  if (!ec) {
    auto sys_time = std::chrono::clock_cast<std::chrono::system_clock>(ftime);
    auto secs = std::chrono::duration_cast<std::chrono::seconds>(
                    sys_time.time_since_epoch())
                    .count();
    sb->st_mtime = secs;
    sb->st_atime = secs;
    sb->st_ctime = secs;
  }
}

int32_t sceKernelStat(const char *path, OrbisKernelStat *sb) {
  if (!path || !sb || !g_mnt_points)
    return -1;
  printf("[libkernel] sceKernelStat: %s\n", path);

  auto host_path = g_mnt_points->GetHostPath(path);
  if (host_path.empty() || !std::filesystem::exists(host_path)) {
    *__Error() = 2; // ENOENT
    return -1;
  }

  FillStat(sb, host_path);
  return 0;
}

int32_t sceKernelFstat(int32_t fd, OrbisKernelStat *sb) {
  if (!sb || !g_handle_table)
    return -1;
  printf("[libkernel] sceKernelFstat: fd=%d\n", fd);

  auto *file = g_handle_table->GetFile(fd);
  if (!file || !file->is_opened) {
    *__Error() = 9; // EBADF
    return -1;
  }

  if (file->type == Core::FileSys::FileType::Device) {
    std::memset(sb, 0, sizeof(OrbisKernelStat));
    sb->st_mode = 0020666; // S_IFCHR
    return 0;
  }

  FillStat(sb, file->m_host_name);
  return 0;
}

// --- Critical Boot Stubs ---

static u64 g_stack_chk_guard = 0xDEADBEEF54321ABCULL;

static void stack_chk_fail() {
  printf("[Kernel] FATAL: __stack_chk_fail called!\n");
  // In a real scenario this would abort, but for now just log
}

const char *PS4_SYSV_ABI sceKernelGetFsSandboxRandomWord() { return "sys"; }

int32_t PS4_SYSV_ABI _sigprocmask() { return 0; }

int32_t PS4_SYSV_ABI posix_getpagesize() { return 16384; } // 16KB PS4 pages

// sysconf — returns system config values
static uint64_t PS4_SYSV_ABI posix_sysconf(int32_t name) {
  switch (name) {
  case 47: // _SC_PAGESIZE
    return 16384;
  case 58: // _SC_THREAD_STACK_MIN
    return 16384;
  case 68: // _SC_NPROCESSORS_ONLN
    return 8; // PS4 has 8 cores
  default:
    printf("[Kernel] posix_sysconf: unhandled name=%d\n", name);
    return 0;
  }
}

// Heap trace info (libc)
struct HeapInfoInfo {
  uint64_t size;
  uint32_t flag;
  uint32_t getSegmentInfo;
  uint64_t *mspace_atomic_id_mask;
  uint64_t *mstate_table;
};

static uint64_t g_mspace_atomic_id_mask = 0;
static uint64_t g_mstate_table[64] = {0};

void PS4_SYSV_ABI sceLibcHeapGetTraceInfo(HeapInfoInfo *info) {
  if (!info) return;
  info->mspace_atomic_id_mask = &g_mspace_atomic_id_mask;
  info->mstate_table = g_mstate_table;
  info->getSegmentInfo = 0;
}

// Entry params
static int32_t g_argc = 1;
static const char *g_argv_data[] = {"eboot.bin", nullptr};
static const char **g_argv = g_argv_data;

int32_t PS4_SYSV_ABI getargc() { return g_argc; }
const char **PS4_SYSV_ABI getargv() { return g_argv; }

// Registration function
void RegisterLibKernel(::Core::Kernel::ModuleManager *module_manager) {
  printf("[LibKernel] Registering functions...\n");

  RegisterProcess(module_manager);
  RegisterTime(module_manager);
  RegisterEventQueue(module_manager);

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

  // FileSystem
  LIB_FUNCTION("1G3lF1Gg1k8", "libkernel", 1, "libkernel", sceKernelOpen);
  LIB_FUNCTION("UK2Tl2DWUns", "libkernel", 1, "libkernel", sceKernelClose);
  LIB_FUNCTION("Cg4srZ6TKbU", "libkernel", 1, "libkernel", sceKernelRead);
  LIB_FUNCTION("4wSze92BhLI", "libkernel", 1, "libkernel", sceKernelWrite);
  LIB_FUNCTION("oib76F-12fk", "libkernel", 1, "libkernel", sceKernelLseek);
  LIB_FUNCTION("eV9wAD2riIA", "libkernel", 1, "libkernel", sceKernelStat);
  LIB_FUNCTION("kBwCPsYX-m4", "libkernel", 1, "libkernel", sceKernelFstat);

  // Critical boot functions
  LIB_FUNCTION("JGfTMBOdUJo", "libkernel", 1, "libkernel",
               sceKernelGetFsSandboxRandomWord);
  LIB_FUNCTION("6xVpy0Fdq+I", "libkernel", 1, "libkernel", _sigprocmask);
  LIB_FUNCTION("Ou3iL1abvng", "libkernel", 1, "libkernel", stack_chk_fail);
  LIB_FUNCTION("k+AXqu2-eBc", "libkernel", 1, "libkernel", posix_getpagesize);
  LIB_FUNCTION("k+AXqu2-eBc", "libScePosix", 1, "libkernel",
               posix_getpagesize);
  LIB_FUNCTION("NWtTN10cJzE", "libSceLibcInternalExt", 1,
               "libSceLibcInternal", sceLibcHeapGetTraceInfo);
  LIB_FUNCTION("mkawd0NA9ts", "libkernel", 1, "libkernel", posix_sysconf);
  LIB_FUNCTION("mkawd0NA9ts", "libScePosix", 1, "libkernel", posix_sysconf);
  LIB_FUNCTION("iKJMWrAumPE", "libkernel", 1, "libkernel", getargc);
  LIB_FUNCTION("FJmglmTMdr4", "libkernel", 1, "libkernel", getargv);

  // Stack guard object
  module_manager->RegisterHLEExport("libkernel", "f7uOxY9mM1U",
                                    "__stack_chk_guard",
                                    reinterpret_cast<uint64_t>(
                                        &g_stack_chk_guard));

  printf("[libkernel] Registration complete.\n");
}

} // namespace Kernel
} // namespace Libraries
} // namespace Core
