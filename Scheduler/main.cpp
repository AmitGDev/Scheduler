// main.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <syncstream>
#include <iostream>
#include "Scheduler.h"


namespace // (Anonymous namespace)
{

    void TestGenericCallback()
    {
        std::cout << "* test lambda & functor callbacks" << std::endl;

        // Lambda callback
        auto lambda_callback = [](uint64_t timer_id, int value) {
            std::osyncstream sync_stream(std::cout);
            sync_stream << "lambda callback for timer " << timer_id << " expired. int value: " << value << std::endl;
            };

        // Functor callback
        struct MyFunctor {
            void operator()(uint64_t timer_id, std::string str, int value) const {
                std::osyncstream sync_stream(std::cout);
                sync_stream << "functor callback for timer " << timer_id << " expired. string data: " << str << " int value: " << value << std::endl;
            }
        };

        Scheduler scheduler{};

        // Using lambda callback
        scheduler.ScheduleTimer(1, 2000, lambda_callback, 42); // <--

        // Using functor callback
        MyFunctor functor;
        scheduler.ScheduleTimer(2, 4000, functor, std::string("test functor string"), 2024); // <--

        // Sleep for a while to let the timers expire
        std::this_thread::sleep_for(std::chrono::seconds(6));
    }


    // Function Callback__

    void OnTimer(uint64_t timer_id)
    {
        std::osyncstream sync_stream(std::cout);
        sync_stream << "timer " << timer_id << " expired" << std::endl;
    }


    void TestFunctionCallback2Timers()
    {
        std::cout << "* test \"traditional\" function callback" << std::endl;

        Scheduler scheduler{};

        scheduler.ScheduleTimer(1, 2000, OnTimer); // <-- 
        scheduler.ScheduleTimer(2, 4000, OnTimer); // <-- 

        // Sleep for a while to let the timers expire
        std::this_thread::sleep_for(std::chrono::seconds(6));
    }

    // __Function Callback


    void TestMemberFunctionCallback_PlusExtraParameter_PlusReschedule()
    {
        std::cout << "* test member function & instance callback" << std::endl;

        class Model final
        {
        public:

            void Test()
            {
                std::cout << "* test re-activate the timer 5 times" << std::endl;

                scheduler_.ScheduleTimer(1, 1000, &Model::OnTimer, this, 5); // <-- 

                // Sleep for a while to let the timer expire
                std::this_thread::sleep_for(std::chrono::seconds(6));
            }

        protected:
            Scheduler scheduler_{};

            void OnTimer(uint64_t timer_id, uint8_t n) // n - Extra parameter
            {
                if (n > 0) {
                    std::osyncstream sync_stream(std::cout);
                    sync_stream << "timer " << timer_id << " expired" << std::endl;

                    scheduler_.ScheduleTimer(timer_id, 1000, &Model::OnTimer, this, n - 1); // <--
                }
            }
        };

        Model model{};
        model.Test();
    }


    void TestEndCases()
    {
        std::cout << "* test end cases" << std::endl;

    }

}


// Main
int main()
{
    TestGenericCallback();
    TestFunctionCallback2Timers();
    TestMemberFunctionCallback_PlusExtraParameter_PlusReschedule();
 //   TestEndCases();
}

