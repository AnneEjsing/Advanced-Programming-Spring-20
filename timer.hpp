#include <iostream>
#include <memory>
#include <chrono>

//Resource Acquisition Is Initialization(RAII)
class Timer
{
public:
    Timer() { start_time_point = std::chrono::high_resolution_clock::now(); }
    ~Timer() { Stop(); }

    void Stop()
    {
        auto end_time_point = std::chrono::high_resolution_clock::now();
        auto start = std::chrono::time_point_cast<std::chrono::microseconds>(start_time_point).time_since_epoch().count();
        auto end = std::chrono::time_point_cast<std::chrono::microseconds>(end_time_point).time_since_epoch().count();
        auto duration = end - start;
        std::cout << duration << "us" << std::endl;
    }

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> start_time_point;
};