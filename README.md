

**Scheduler v1.0.0**

A **Cross-Platform** Flexible Timer Scheduler for **C++20** applications **(Header-Only Class)**

**Author:** Amit Gefen

**License:** MIT License

<br>

**Overview**

- Provides a high-level interface for scheduling timers in **C++20** applications.
- Supports various callback mechanisms, including lambdas, functors, plain function pointers, and member functions.
- Leverages **Boost Asio** for efficient asynchronous timer management.
- Executes callbacks on a dedicated thread to avoid blocking the main thread.
- Implements graceful shutdown to ensure pending timers are completed before destruction.

<br>

**Features**

- Flexible Callback Options:
  - Schedule timers with lambdas, functors, function pointers, or member functions.
  - Pass optional arguments to the callbacks for custom data.
- Asynchronous Execution:
  - Callbacks execute on a dedicated thread to prevent blocking.
- Robust Error Handling:
  - Catches and logs potential errors during timer scheduling and execution.
- Thread Safety:
  - Designed for safe usage in multithreaded environments.
- Graceful Shutdown:
  - Ensures pending timers are completed before the scheduler is destroyed.

<br>

**Usage**

\- Include the header file:
```cpp
#include "Scheduler.h"
```
\- Create a Scheduler instance:
```cpp
Scheduler scheduler{};
```
\- Schedule timers using the ScheduleTimer method:
```cpp
scheduler.ScheduleTimer(1 /*timer_id*/, 2000 /*milliseconds*/, [](uint64_t timer_id) { 
  std::cout << "Timer " << timer_id << " expired!" << std::endl;
  });
```

<br>

**Example Usage**

See the **main.cpp** file for a comprehensive example demonstrating various scheduling scenarios.

<br>

**Dependencies**

Boost Asio (<https://www.boost.org/doc/libs/1_84_0/doc/html/boost_asio.html>)

