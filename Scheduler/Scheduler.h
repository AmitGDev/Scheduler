#ifndef AMITG_FC_SCHEDULER
#define AMITG_FC_SCHEDULER

/*
    Scheduler.hpp
    Copyright (c) 2024, Amit Gefen

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
    THE SOFTWARE.
*/

#include <functional>
#include <chrono>
#include <thread>
#include <future>
#include <syncstream>
#include <iostream>
#include <boost/asio.hpp>


// Scheduler for timer management.
// Callbacks execute on a dedicated thread (io_service thread). See Boost Asio threading guidelines:
// https://www.boost.org/doc/libs/1_84_0/doc/html/boost_asio/overview/core/threads.html

class Scheduler final
{
public:

    // Constructor
    //
    // - Starts the io_service_ using a dedicated thread (io_service_thread_).
    //   - The io_service_ is responsible for managing asynchronous operations within the Scheduler.
    // - Creates a io_service::work object (io_service_work_) to ensure the io_service_ keeps running until explicitly stopped.
    //
    // Throws:
    //   - Any standard exceptions that might occur during thread creation or io_service_ initialization.
    Scheduler() : io_service_(), io_service_work_(io_service_), io_service_thread_([this] { Service(); }) // (Runs the function Service() asynchronously)
    {
    }

    // Destructor
    //
    // - Gracefully stops the io_service_ by calling `stop()`.
    //   - This ensures any pending asynchronous operations are completed before the scheduler is destroyed.
    // - Waits for the dedicated io_service_ thread to finish execution using `join()`.
    // - Catches and logs any potential exceptions that occur during the stopping or joining process.
    ~Scheduler()
    {
        try { 
            io_service_.stop();
        } catch (const std::exception& e) {
            std::cerr << "error stopping io_service_: " << e.what() << std::endl;
        }

        // No need for io_service_thread_.join() as jthread handles that automatically.
    }

    // (1) ScheduleTimer(timer_id, duration, callback, callback_args...)
    //
    // Schedules a timer with the most flexible option, accepting any callable object (lambda, functor, etc.) as the callback.
    // - `timer_id`: A unique identifier for the timer.
    // - `duration`: The duration (in milliseconds) until the timer expires.
    // - `callback`: The callable object to be invoked when the timer expires.
    // - `callback_args...`: Optional arguments to be passed to the callback.
    //
    // Creates a shared pointer to a `boost::asio::deadline_timer`, sets up an asynchronous wait using the provided callback,
    // and handles potential errors during setup.
    template <typename Callback, typename... Args>
    void ScheduleTimer(const uint64_t timer_id, const uint32_t duration, const Callback& callback, Args... callback_args)
    {
        // Create a shared pointer to a boost::asio::deadline_timer:
        const auto timer = std::make_shared<boost::asio::deadline_timer>(io_service_, boost::posix_time::milliseconds(duration));

        try {
            // Set up asynchronous wait on the timer, invoking the provided callback when the timer expires:
            timer->async_wait([timer_id, timer, callback, callback_args...](const boost::system::error_code& e) {
                if (e) {
                    // Handle error
                    std::cerr << "error waiting on timer (id = " << timer_id << "): " << e.message() << std::endl;
                } else {
                    // Call the callback with the captured arguments
                    callback(timer_id, callback_args...);
                }
                });
        } catch (const std::exception& e) {
            std::cerr << "error scheduling timer (id = " << timer_id << "): " << e.what() << std::endl;
        }
    }

    // (2) ScheduleTimer(timer_id, duration, member_function, instance, member_function_args...)
    //
    // Schedules a timer that invokes a member function of an object.
    // - `timer_id`: A unique identifier for the timer.
    // - `duration`: The duration (in milliseconds) until the timer expires.
    // - `member_function`: A pointer to the member function to be invoked.
    // - `instance`: A pointer to the object on which to invoke the member function.
    // - `member_function_args...`: Optional arguments to be passed to the member function.
    //
    // Creates a lambda callback capturing the member function and instance, then delegates to the generic `ScheduleTimer` overload
    // with the lambda and any additional arguments.
    template <typename Callback, typename T, typename... Args>
    void ScheduleTimer(const uint64_t timer_id, const uint32_t duration, const Callback& member_function, T* instance, Args... member_function_args)
    {
        // The lambda callback shapes the *form* of the callback.
        // (Captures the member_function and instance, effectively binding them together.)
        auto callback = [member_function, instance](uint64_t timer_id, Args... lambda_args) {
            (instance->*member_function)(timer_id, std::forward<Args>(lambda_args)...);
            };

        // Schedules the timer using the lambda callback and forwards any additional arguments (member_function_args).
        ScheduleTimer(timer_id, duration, callback, std::forward<Args>(member_function_args)...); // <-- DELEGATE TO (1)
    }

private:

    // Service()
    //
    // - Runs in the context of a dedicated thread (io_service_thread_).
    // - Starts the Boost Asio io_service_ event loop, which is responsible for executing all scheduled asynchronous operations.
    // - This function blocks until the io_service_ is explicitly stopped or an error occurs.
    // - Catches and logs any exceptions that occur during the io_service_ execution.
    //
    // Note: This function should not be called directly.
    void Service()
    {
        try {
            io_service_.run(); // (Blocking)
        } catch (const std::exception& e) {
            std::cerr << "exception in service thread: " << e.what() << std::endl;
        }
    }

    // io_service_: The core object from Boost Asio responsible for managing asynchronous operations within the Scheduler.
    // - Handles the scheduling and execution of the timers.
    // - Functions as the central event loop for the Scheduler's asynchronous activities.
    boost::asio::io_service io_service_{};

    // io_service_work_: A work object associated with the io_service_ to prevent it from automatically exiting.
    // - Ensures the io_service_ event loop continues to run even when there are no pending asynchronous operations.
    // - This keeps the Scheduler active and ready to handle new tasks as they arrive.
    const boost::asio::io_service::work io_service_work_;

    // io_service_thread_: Thread for running io_service_ event loop, separate from the Scheduler's creation thread.
    // - This prevents blocking of the creating thread and ensures responsiveness.
    // - It enables concurrent handling of asynchronous operations alongside other tasks in the program.
    std::jthread io_service_thread_{};
};

#endif