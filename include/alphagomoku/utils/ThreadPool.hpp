/*
 * ThreadPool.hpp
 *
 *  Created on: Mar 22, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_UTILS_THREADPOOL_HPP_
#define ALPHAGOMOKU_UTILS_THREADPOOL_HPP_

#include <vector>
#include <queue>
#include <thread>
#include <condition_variable>
#include <mutex>

namespace ag
{
//	class Job
//	{
//		public:
//			virtual ~Job() = default;
//			virtual void run() = 0;
//	};
//
//	class ThreadPool
//	{
//		private:
//			std::vector<std::thread> threads;
//			std::queue<Job*> queue;
//			mutable std::mutex queue_mutex;
//			std::condition_variable queue_cond;
//			std::condition_variable ready_cond;
//
//			int number_of_working_threads = 0;
//			bool is_running = true;
//		public:
//			ThreadPool(size_t numberOfThreads);
//			ThreadPool(const ThreadPool &other) = delete;
//			ThreadPool& operator=(const ThreadPool &other) = delete;
//			ThreadPool(ThreadPool &&other) = default;
//			ThreadPool& operator=(ThreadPool &&other) = default;
//			~ThreadPool();
//
//			void addJob(Job *job);
//			bool isReady() const noexcept;
//			int size() const noexcept;
//			void waitForFinish();
//
//		private:
//			static void thread_run(ThreadPool *arg);
//			bool is_ready() const noexcept;
//	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_UTILS_THREADPOOL_HPP_ */
