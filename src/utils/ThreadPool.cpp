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
		return number_of_working_threads == 0 and queue.empty();
	}
	void ThreadPool::waitForFinish()
	{
		std::unique_lock lock(queue_mutex);
		ready_cond.wait(lock, [this] // @suppress("Invalid arguments")
		{	return (number_of_working_threads == 0 and queue.empty()) or not is_running;});
	}
	void ThreadPool::thread_run(ThreadPool *arg)
	{
		assert(arg != nullptr);
		while (arg->is_running)
		{
			std::unique_lock lock(arg->queue_mutex);
			arg->queue_cond.wait(lock, [arg]() // @suppress("Invalid arguments")
					{	return not arg->queue.empty() or not arg->is_running;});
			if (arg->is_running == false)
				break;

			Job *job = arg->queue.front();
			arg->queue.pop();
			arg->number_of_working_threads++;
			lock.unlock();

			job->run();

			lock.lock();
			arg->number_of_working_threads--;
			if (arg->number_of_working_threads == 0)
				arg->ready_cond.notify_all();
		}
	}
} /* namespace ag */

