/*
 * ThreadPool.cpp
 *
 *  Created on: Mar 23, 2021
 *      Author: maciek
 */

#include <alphagomoku/utils/ThreadPool.hpp>

#include <assert.h>

namespace ag
{
	ThreadPool::ThreadPool(size_t numberOfThreads) :
			threads(numberOfThreads)
	{
		for (size_t i = 0; i < threads.size(); i++)
			threads[i] = std::thread(thread_run, this);
	}
	ThreadPool::~ThreadPool()
	{
		{
			std::lock_guard lock(queue_mutex);
			is_running = false;
		}
		queue_cond.notify_all();
		for (size_t i = 0; i < threads.size(); i++)
			threads[i].join();
		ready_cond.notify_all();
	}

	void ThreadPool::addJob(Job *job)
	{
		if (job != nullptr)
		{
			std::unique_lock lock(queue_mutex);
			queue.push(job);
			queue_cond.notify_one();
		}
	}
	bool ThreadPool::isReady() const noexcept
	{
		std::lock_guard lock(queue_mutex);
		return is_ready();
	}
	void ThreadPool::waitForFinish()
	{
		std::unique_lock lock(queue_mutex);
		ready_cond.wait(lock, [this] // @suppress("Invalid arguments")
		{	return this->is_ready() or is_running == false;});
	}
	void ThreadPool::thread_run(ThreadPool *arg)
	{
		assert(arg != nullptr);

		std::unique_lock lock(arg->queue_mutex);
		while (arg->is_running)
		{
			arg->queue_cond.wait(lock, [arg]() // @suppress("Invalid arguments")
					{	return arg->queue.empty() == false or arg->is_running == false;});
			if (arg->is_running == false)
				break;
			assert(arg->queue.size() > 0);
			Job *job = arg->queue.front();
			arg->queue.pop();
			arg->number_of_working_threads++;
			lock.unlock();

			assert(job != nullptr);
			job->run();

			lock.lock();
			arg->number_of_working_threads--;
			if (arg->is_ready())
				arg->ready_cond.notify_all();
		}
	}
	bool ThreadPool::is_ready() const noexcept
	{
		return number_of_working_threads == 0 and queue.empty();
	}
} /* namespace ag */

