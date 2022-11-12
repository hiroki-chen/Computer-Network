/*
 Copyright (c) 2021 Haobin Chen

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef TIMER_HH
#define TIMER_HH

#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

namespace fake_tcp {
class Timer {
    std::atomic<bool> active { true };

public:
    template <class lambda>
    void setTimeout(lambda&& function, int delay);

    template <class lambda>
    void setInterval(lambda&& function, int interval);
    void stop();
};

template <class lambda>
void Timer::setTimeout(lambda&& function, int delay)
{
    active = true;
    std::thread t([=]() {
        if (!active.load())
            return;
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
        if (!active.load())
            return;
        function();
    });
    t.detach();
}

template <class lambda>
void Timer::setInterval(lambda&& function, int interval)
{
    active = true;
    std::thread t([=]() {
        while (active.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(interval));
            if (!active.load())
                return;
            function();
        }
    });
    t.join();
}

void Timer::stop()
{
    active = false;
}
} // namespace fake_tcp

#endif