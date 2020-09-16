#ifndef MUTEX_H
#define MUTEX_H

typedef int mutex_t;

int mutex_lock(mutex_t *mutex);
int mutex_unlock(mutex_t *mutex);
int mutex_init(mutex_t *mutex);

#endif //MUTEX_H
