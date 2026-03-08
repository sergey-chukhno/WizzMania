#ifndef NATIVE_SHARED_MEMORY_H
#define NATIVE_SHARED_MEMORY_H

#include "GameIPC.h"
#include <iostream>
#include <string>

#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace wizz {

// ---------------------------------------------------------------------------
// NativeSharedMemory<T>
//
// A thin POSIX (Linux/macOS) / Win32 shared-memory wrapper that maps exactly
// sizeof(T) bytes.  Previously the class was not templated and hardcoded
// sizeof(GameIPCData), which silently truncated TicTacToeIPCData (a larger
// struct) and corrupted every field beyond the first 37 bytes.
// ---------------------------------------------------------------------------
template <typename T = GameIPCData> class NativeSharedMemory {
public:
  explicit NativeSharedMemory(const std::string &name) : m_name(name) {}
  ~NativeSharedMemory() { close(); }

  // Create (or recreate) the segment.  On POSIX we unlink first so that a fresh
  // segment with the right size is always created, even if a stale one from a
  // previous run persists.
  bool createAndMap() {
#ifdef _WIN32
    m_hMapFile =
        CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0,
                           static_cast<DWORD>(sizeof(T)), m_name.c_str());
    if (!m_hMapFile)
      return false;
    m_data = reinterpret_cast<T *>(
        MapViewOfFile(m_hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(T)));
    m_hMutex = CreateMutexA(NULL, FALSE, (m_name + "_mutex").c_str());
    return m_data != nullptr && m_hMutex != nullptr;
#else
    std::string shmName = "/" + m_name;

    // Unlink any stale segment from a previous run so we always start fresh
    // with exactly sizeof(T) bytes.
    shm_unlink(shmName.c_str());

    m_fd = shm_open(shmName.c_str(), O_CREAT | O_EXCL | O_RDWR, 0666);
    if (m_fd == -1) {
      std::cerr << "NativeSharedMemory::createAndMap: shm_open failed for "
                << shmName << ": " << strerror(errno) << std::endl;
      return false;
    }

    if (ftruncate(m_fd, static_cast<off_t>(sizeof(T))) == -1) {
      std::cerr << "NativeSharedMemory::createAndMap: ftruncate failed"
                << std::endl;
      ::close(m_fd);
      m_fd = -1;
      shm_unlink(shmName.c_str());
      return false;
    }

    m_data = reinterpret_cast<T *>(
        mmap(nullptr, sizeof(T), PROT_READ | PROT_WRITE, MAP_SHARED, m_fd, 0));
    if (m_data == MAP_FAILED) {
      std::cerr << "NativeSharedMemory::createAndMap: mmap failed" << std::endl;
      m_data = nullptr;
      ::close(m_fd);
      m_fd = -1;
      shm_unlink(shmName.c_str());
      return false;
    }

    // Semaphore for mutual exclusion (also re-created fresh)
    std::string semName = "/" + m_name + "_sem";
    sem_unlink(semName.c_str());
    m_sem = sem_open(semName.c_str(), O_CREAT | O_EXCL, 0666, 1);
    if (m_sem == SEM_FAILED) {
      std::cerr << "NativeSharedMemory::createAndMap: sem_open failed"
                << std::endl;
      m_sem = nullptr;
      munmap(m_data, sizeof(T));
      m_data = nullptr;
      ::close(m_fd);
      m_fd = -1;
      shm_unlink(shmName.c_str());
      return false;
    }

    m_ownsSegment = true;
    return true;
#endif
  }

  // Attach to an already-created segment (created by createAndMap in another
  // process).
  bool openAndMap() {
#ifdef _WIN32
    m_hMapFile = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, m_name.c_str());
    if (!m_hMapFile)
      return false;
    m_data = reinterpret_cast<T *>(
        MapViewOfFile(m_hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(T)));
    m_hMutex = OpenMutexA(MUTEX_ALL_ACCESS, FALSE, (m_name + "_mutex").c_str());
    return m_data != nullptr && m_hMutex != nullptr;
#else
    std::string shmName = "/" + m_name;
    m_fd = shm_open(shmName.c_str(), O_RDWR, 0666);
    if (m_fd == -1)
      return false;

    m_data = reinterpret_cast<T *>(
        mmap(nullptr, sizeof(T), PROT_READ | PROT_WRITE, MAP_SHARED, m_fd, 0));
    if (m_data == MAP_FAILED) {
      m_data = nullptr;
      ::close(m_fd);
      m_fd = -1;
      return false;
    }

    std::string semName = "/" + m_name + "_sem";
    m_sem = sem_open(semName.c_str(), 0);
    if (m_sem == SEM_FAILED) {
      m_sem = nullptr;
      munmap(m_data, sizeof(T));
      m_data = nullptr;
      ::close(m_fd);
      m_fd = -1;
      return false;
    }

    return true;
#endif
  }

  void lock() {
#ifdef _WIN32
    if (m_hMutex)
      WaitForSingleObject(m_hMutex, INFINITE);
#else
    if (m_sem)
      sem_wait(m_sem);
#endif
  }

  void unlock() {
#ifdef _WIN32
    if (m_hMutex)
      ReleaseMutex(m_hMutex);
#else
    if (m_sem)
      sem_post(m_sem);
#endif
  }

  T *data() { return m_data; }

  void close() {
#ifdef _WIN32
    if (m_data)
      UnmapViewOfFile(m_data);
    if (m_hMapFile)
      CloseHandle(m_hMapFile);
    if (m_hMutex)
      CloseHandle(m_hMutex);
    m_data = nullptr;
    m_hMapFile = NULL;
    m_hMutex = NULL;
#else
    if (m_data) {
      munmap(m_data, sizeof(T));
      m_data = nullptr;
    }
    if (m_fd != -1) {
      ::close(m_fd);
      m_fd = -1;
    }
    if (m_sem) {
      sem_close(m_sem);
      m_sem = nullptr;
    }
#endif
  }

  void unlink() {
#ifdef _WIN32
    // Windows unlinks automatically when handles are closed
#else
    close();
    std::string shmName = "/" + m_name;
    shm_unlink(shmName.c_str());
    std::string semName = "/" + m_name + "_sem";
    sem_unlink(semName.c_str());
#endif
  }

private:
  std::string m_name;
  T *m_data = nullptr;
  bool m_ownsSegment = false;

#ifdef _WIN32
  HANDLE m_hMapFile = NULL;
  HANDLE m_hMutex = NULL;
#else
  int m_fd = -1;
  sem_t *m_sem = nullptr;
#endif
};

// Backwards-compatible alias for existing code that didn't specify T
// (GameIPCData-based games like BrickBreaker).
using GameSharedMemory = NativeSharedMemory<GameIPCData>;

} // namespace wizz

#endif // NATIVE_SHARED_MEMORY_H
