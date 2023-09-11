//==============================================================================
/**
@file       CallBackTimer.h
@brief      A callback timer that calls a given function on a set interval
@copyright  (c) 2020, Momoko Tomoko
**/
//==============================================================================

#pragma once

#include <atomic>
#include <mutex>
#include <vector>
#include <functional> // for function
#include <algorithm> // for min
#include <set>

/**
	@brief Timer that triggers on a specified interval
**/
class CallBackTimer
{
public:
	CallBackTimer()
	{
		lock();
	}

	~CallBackTimer()
	{
		stop();
	}

	/**
		@brief Unlocks mutex and stops/joins the thread
	**/
	void stop()
	{
		running = false;
		if (locked)
		{
			unlock();
		}
		if (thd.joinable())
			thd.join();
	}

	/**
		@brief starts the callback loop with an interval

		@param[in] interval the milliseconds to wait
		@param[in] func the function to trigger
	**/
	void start(uint32_t intervalMilliseconds, std::function<void(void)> func)
	{
		// can't be already running
		if (running == true)
		{
			return;
		}

		running = true;

		// start the timer thread
		thd = std::thread([this, intervalMilliseconds, func]()
			{
				while (running)
				{
					func();
					// wait
					if (timerMutex.try_lock_for(std::chrono::milliseconds(intervalMilliseconds)))
					{
						unlock();
					}
					if (!locked)
					{
						lock();
					}
				}
			});
	};

	/**
		@brief starts the callback loop with given minute-of-the-hour trigger times and ability to wake

		@param[in] triggerMinuteOfTheHour set of the minutes of the hour to trigger on (0-59)
		@param[in] func the function to trigger, should return true on success
	**/
	void start(std::set<int> triggerMinutesOfTheHour, std::function<bool(void)> func)
	{
		// can't be already running
		if (running == true)
		{
			return;
		}

		running = true;

		// start the timer thread
		thd = std::thread([this, triggerMinutesOfTheHour, func]()
			{
				while (running)
				{
					// get current time
					time_t now = time(0);
					// find the closest trigger time
					time_t nextTriggerTime = 0;
					bool isFirst = true;
					for (const auto& triggerMinute : triggerMinutesOfTheHour)
					{
						if (triggerMinute < 0 || triggerMinute >= 60)
							continue;

						struct tm newTime {};
						localtime_s(&newTime, &now);
						newTime.tm_sec = 0;
						newTime.tm_min = triggerMinute;
						time_t triggerTime = mktime(&newTime);

						// if the new time is behind us, it means the next trigger minute is in an hour
						if (difftime(triggerTime, now) < 1)
						{
							triggerTime += 3600; // increment by an hour
						}

						// store the closest trigger time
						if (isFirst || (triggerTime < nextTriggerTime))
						{
							nextTriggerTime = triggerTime;
							isFirst = false;
						}
					}

					// call the desired function
					int status = func();

					int waitTime = 0;
					if (status == false) // function failed, retry in 5s
					{
						waitTime = 5;
					}
					else
					{
						// function may be slow, recompute current time, diff maybe be negative which in that case we immeadiatly unlock mutex
						now = time(0);
						waitTime = (int)(difftime(nextTriggerTime, now));
					}

					// wait
					if (timerMutex.try_lock_for(std::chrono::seconds(waitTime)))
					{
						unlock();
					}
					if (!locked)
					{
						lock();
					}
				}
			});
	}

	/**
		@brief checks if thread is running

		@return true if thread is running
	**/
	bool is_running() const noexcept
	{
		return (running && thd.joinable());
	}

	/**
		@brief force a wakeup even if trigger time not met
	**/
	void wake()
	{
		if (is_running() && locked)
		{
			unlock();
		}
	}
private:
	std::atomic_bool running = false;
	std::thread thd;

	std::timed_mutex timerMutex;
	std::atomic_bool locked = false;

	void lock()
	{
		timerMutex.lock();
		locked = true;
	}

	void unlock()
	{
		locked = false;
		timerMutex.unlock();
	}
};