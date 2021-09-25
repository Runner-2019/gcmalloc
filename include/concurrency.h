/*
  --------------------------------------
     ╭╩══╮╔══════╗╔══════╗╔══════╗ 
    ╭    ╠╣      ╠╣      ╠╣      ╣
    ╰⊙══⊙╯╚◎════◎╝╚◎════◎╝╚◎════◎╝
  --------------------------------------
  @date:   2021-九月-19
  @author: xiaomingZhang2020@outlook.com
  --------------------------------------
*/

#ifndef GCMALLOC_CONCURRENCY_H
#define GCMALLOC_CONCURRENCY_H
#include <mutex>
#include <pthread.h>

/* A wrapper of thread specific thread */
class Tsd {
public:
    Tsd() { create(); }

    ~Tsd() { pthread_key_delete(m_arena_key); }


    [[nodiscard]] void *get() const {
        return pthread_getspecific(m_arena_key);
    }

    void set(const void *value) const {
        pthread_setspecific(m_arena_key, value);
    }


private:
    void create() {
        pthread_key_create(&m_arena_key, nullptr);
    }

    pthread_key_t m_arena_key{};
};


class Mutex {
public:
    void lock() { m.lock(); }
    void unlock() { m.unlock(); }
    bool trylock() { return m.try_lock(); } // if lock success, return true
private:
    std::mutex m;
};

#endif //GCMALLOC_CONCURRENCY_H
