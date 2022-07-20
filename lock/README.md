**为了充分复用代码，将信号量、互斥锁、条件变量三种线程同步机制封装成3个类，实现在locker.h中。**
# 相关知识＿RAII及三种进程同步机制

## 1.RAII(Resource Acquisition is Initalization)资源获取即初始化

- 在构造函数中申请分配资源,在析构函数中释放资源.因为C++的语言机制保证:当一个对象创建时,自动调用构造函数,当对象超出作用域的时候会自动调用析构函数.所以,在RAII的指导下,应该**使用类管理资源,将资源和对象的生命周期绑定.**

## 2.信号量

在Linux 上，信号量API有两组。一组是System V IPC信号量，另外一组是POSIX信号量。这两组接口很相似，但不保证能互换。这两种信号量的语义完全相同。

### 2.1System V IPC信号量

- 当多个进程同时访问系统上的某个资源的时候，比如同时写一个数据库的某条记录，或者同时修改某个文件，就需要考虑进程的同步问题，以确保任一时刻只有一个进程可以拥有对资源的独占式访问。

- 通常，程序对共享资源的访问的代码只是很短的一段，但就是这一段代码引发了进程之间的竞态条件。我们称这段代码为关键代码段，或者**临界区**。对进程同步，也就是确保任一时刻只有一个进程能进入关键代码段。

- 要编写具有通用目的的代码，以确保关键代码段的独占式访问是非常困难的。有两个名为Dekker算法和 Peterson算法的解决方案，它们试图从语言本身(不需要内核支持）解决并发问题。但它们依赖于忙等待，即进程要持续不断地等待某个内存位置状态的改变。这种方式下CPU利用率太低，显然是不可取的。

- **Dijkstra提出的信号量(Semaphore）概念是并发编程领域迈出的重要一步。**

- **信号量是一种特殊的变量，它只能取自然数值并且只支持两种操作:等待（wait）和信号(signal)。**不过在Linux/UNIX中，“等待”和“信号”都已经具有特殊的含义，所以对信号量的这两种操作更常用的称呼是**P、V操作**。这两个字母来自于荷兰语单词passeren（传递，就好像进人临界区）和vrijgeven(释放，就好像退出临界区)。假设有信号量SV，则对它的P、V操作含义如下:

  - > P(SV),如果SV的值大于0,就将它减1.如果SV的值为0,则挂起进程的执行.
    >
    > V(SV),如果有其他进程因为等待SV而挂起,则唤醒之;如果没有,将SV加1.

信号量的取值可以是任何自然数。但最常用的、最简单的信号量是二进制信号量，它只能取0和1这两个值。

Linux信号量的API都定义在sys/sem.h头文件中，主要包含3个系统调用:**semget,semop和 semctl**。它们都被设计为操作一组信号量，即信号量集.

### 2.2 POSIX信号量

POSIX信号量函数的名字都以sem_开头。常用的POSIX信号量函数是下面5个:

> include < semaphore.h>
> int sem_init ( sem_t* sem, int pshared,unsigned int value );
>
> int sem_destroy( sem_t* sem ) ;
> int sem_wait ( sem_t*sem ) ;*
>
> int sem_trywait ( sem_t* sem ) ;
> int sem_post ( sem_t* sem );
>
> 这些函数的第一个参数sem指向被操作的信号量。
> -----sem_init函数用于初始化一个未命名的信号量。
>
> ​	---pshared参数指定信号量的类型。如果其值为0，就表示这个信号量是当前进程的局部信号量，否则该信号量就可以在多个进程之间共享。value参数指定信号量的初始值。初始化一个已经被初始化的信号量将导致不可预期的结果。
> -----sem_destroy函数用于销毁信号量，以释放其占用的内核资源。如果销毁一个正被其他线程等待的信号量，则将导致不可预期的结果。
> -----sem_wait函数以原子操作的方式将信号量的值减1。如果信号量的值为0，则sem_wait将被阻塞，直到这个信号量具有非0值。
> -----sem_trywait与sem_wait函数相似，不过它始终立即返回，而不论被操作的信号量是否具有非0值，相当于sem_wait的非阻塞版本。当信号量的值非0时，sem_trywait对信号量执行减Ⅰ操作。当信号量的值为0时，它将返回-1并设置errno为EAGAIN.
> -----sem_post函数以原子操作的方式将信号量的值加1。当信号量的值大于0时，其他正在调用sem_wait等待信号量的线程将被唤醒。上面这些函数成功时返回0，失败则返回-1并设置errno。

## 3.互斥锁

互斥锁（也称互斥量〉可以用于保护关键代码段，以确保其独占式的访问，这有点像个二进制信号量。当进入关键代码段时，我们需要获得互斥锁并将其加锁,这等价于二进制信号量的Р操作;当离开关键代码段时，我们需要对互斥锁解锁，以唤醒其他等待该互斥锁的线程，这等价于二进制信号量的V操作。

POSIX互斥锁的相关函数主要有如下5个:

> include <pthread.h>
> int pthread_mutex_init ( pthread_mutex_t* mutex,const pthread_mutexattr_t* mutexattr ) ;
> int pthread_mutex_destroy( pthread_mutex_t* mutex );
> int pthread_mutex_lock ( pthread_mutex_t* mutex );
>
> int pthread_mutex_trylocki(pthread_mutex_t* mutex );
>
> int pthread_mutex_unlock( pthread_mutex_t* mutex );
> 这些函数的第一个参数mutex指向要操作的目标互斥锁，互斥锁的类型是pthread_mutex_t结构体。
>
> -----pthread_mutex_init函数用于初始化互斥锁。
>
> ​	---mutexattr参数指定互斥锁的属性。如果将它设置为NULL，则表示使用默认属性。除了这个函数外，还可以使用如下方式来初始化一个互斥锁:pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;宏PTHREAD_MUTEX_INITIALIZER实际上只是把互斥锁的各个字段都初始化为0。
>
> -----pthread_mutex_destroy函数用于销毁互斥锁，以释放其占用的内核资源。销毁一个已经加锁的互斥锁将导致不可预期的后果。
> -----pthread_mutex_lock 函数以原子操作的方式给一个互斥锁加锁。如果目标互斥锁已经故锁上，则pthread_mutex_lock调用将阻塞，直到该互斥锁的占有者将其解锁。
> -----pthread_mutex_trylock 与 pthread_mutex_lock函数类似，不过它始终立即返回，而不论被操作的互斥锁是否已经被加锁，相当于pthread_mutex_lock的非阻塞版本。当目标互论被操作的互斥锁是否已经被加锁，相当于pthread_mutex_lock 的非阻塞版本。当目标互斥锁未被加锁时，pthread_mutex_trylock对互斥锁执行加锁操作。当互斥锁已经被加锁时,pthread_mutex_trylock将返回错误码EBUSY。需要注意的是，这里讨论的pthread_mutexlock 和pthread_mutex_trylock 的行为是针对普通锁而言的。对于其他类型的锁而言，这两个加锁函数会有不同的行为。
> -----pthread_mutex_unlock 函数以原子操作的方式给一个互斥锁解锁。如果此时有其他线程正在等待这个互斥锁，则这些线程中的某一个将获得它。
>
> 上面这些函数成功时返回0，失败则返回错误码。

## 4.条件变量

**如果说互斥锁是用于同步线程对共享数据的访问的话，那么条件变量则是用于在线程之间同步共享数据的值.**条件变量提供了一种线程间的通知机制:**当某个共享数据达到某个值的时候，唤醒等待这个共享数据的线程。**

条件变量的相关函数主要有如下5个:

> #include <pthread.h>
> int pthread_cond_init( pthread_cond_t* cond，const pthread_condattr_t* cond_attr);
>
> int pthread_cond_destroy ( pthread_cond_t * cond);
> int pthread_cond_broadcast( pthread_cond_t* cond );
>
> int pthread_cond_signal( pthread_cond_t* cond ) ;
> int pthread_cond_wait( pthread_cond_t* cond,pthread_mutex_t* mutex ) ;
> 这些函数的第一个参数cond指向要操作的目标条件变量，条件变量的类型是pthread_cond_t结构体。
> -----pthread_cond_init函数用于初始化条件变量。
>
> ​	---cond_attr参数指定条件变量的属性。如果将它设置为NULL，则表示使用默认属性。除了pthread_cond_init函数外，我们还可以使用如下方式来初始化一个条件变量:pthread_cond_t cond = PTHREAD_COND_INITIALIZER;宏PTHREAD_COND_INITIALIZER实际上只是把条件变量的各个字段都初始化为0。
>
> -----pthread_cond_destroy函数用于销毁条件变量，以释放其占用的内核资源。销毁一个正在被等待的条件变量将失败并返回EBUSY。
> -----pthread_cond_broadcast函数以广播的方式唤醒所有等待目标条件变量的线程。
>
> -----pthreadcond_signal函数用于唤醒一个等待目标条件变量的线程。至于哪个线程将被唤醒，则取决于线程的优先级和调度策略。有时候我们可能想唤醒一个指定的线程，但pthread没有对该需求提供解决方法。不过我们可以间接地实现该需求:定义一个能够唯一表示目标线程的全局变量，在唤醒等待条件变量的线程前先设置该变量为目标线程，然后采用广播方式唤醒所有等待条件变量的线程，这些线程被唤醒后都检查该变量以判断被唤醒的是否是自己，如果是就开始执行后续代码，如果不是则返回继续等待。
> -----pthread_cond_wait函数用于等待目标条件变量。mutex 参数是用于保护条件变量的互斥锁，以确保pthread_cond_wait操作的原子性。在调用pthread_cond_wait前，必须确保互斥锁mutex 已经加锁，否则将导致不可预期的结果。pthread_cond_wait 函数执行时，首先把调用线程放入条件变量的等待队列中，然后将互斥锁mutex解锁。可见，从pthread_cond_wait开始执行到其调用线程被放入条件变量的等待队列之间的这段时间内，pthread_cond_signal和pthread_cond_broadcast等函数不会修改条件变量。换言之，pthread_cond_wait 函数不会错过目标条件变量的任何变化”。当pthread_cond_wait 函数成功返回时，互斥锁mutex将再次被锁上。
> 上面这些函数成功时返回0，失败则返回错误码。

