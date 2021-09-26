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
#include <pthread.h>

/* A wrapper of thread specific thread */
class Tsd {
public:
    Tsd() noexcept { pthread_key_create(&m_arena_key, nullptr); }

    ~Tsd() { pthread_key_delete(m_arena_key); }


    [[nodiscard]] void *get() const {
        return pthread_getspecific(m_arena_key);
    }

    void set(const void *value) const {
        pthread_setspecific(m_arena_key, value);
    }


private:
    pthread_key_t m_arena_key{};
};


class Mutex {
public:
    Mutex() { init(); }
    void lock() { pthread_mutex_lock(&m); }
    void unlock() { pthread_mutex_unlock(&m); }
    bool trylock() { return pthread_mutex_trylock(&m) == 0; } // if lock success, return true
    void init() { pthread_mutex_init(&m, nullptr); }
private:
    pthread_mutex_t m;
};

#endif //GCMALLOC_CONCURRENCY_H
