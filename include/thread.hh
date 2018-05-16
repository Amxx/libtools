#ifndef SYNC_HH
#define SYNC_HH

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace madag::sync
{

	/***************************************************************************/
	/*                                  Utils                                  */
	/***************************************************************************/
	class notifiable
	{
		public:
			void notify()
			{
				std::unique_lock<decltype(m_mutex)> lock(m_mutex);
				m_wake = true;
				m_condition.notify_one();
			}
			void wait()
			{
				std::unique_lock<decltype(m_mutex)> lock(m_mutex);
				while (!m_wake) m_condition.wait(lock);
				m_wake = false;
			}
			bool try_wait()
			{
				std::unique_lock<decltype(m_mutex)> lock(m_mutex);
				bool result = m_wake;
				m_wake = false;
				return result;
			}

		private:
			std::mutex              m_mutex;
			std::condition_variable m_condition;
			bool                    m_wake = false;
	};

	/***************************************************************************/
	/*                             Generic thread                              */
	/***************************************************************************/
	template<class... Args>
	class PolymorphicThread
	{
		public:
			virtual ~PolymorphicThread() = default;
			virtual void start (Args&&... args)
			{
				m_thread = std::thread(&PolymorphicThread::run, this, std::forward<Args>(args)...);
			}
			virtual void join  () { m_thread.join();            }
			virtual void detach() { m_thread.detach();          }
			virtual bool active() { return m_thread.joinable(); }

		protected:
			virtual void run(Args... args) = 0;

		private:
			std::thread m_thread;
	};

	/***************************************************************************/
	/*                Pulsers (ticking threads, interruptable)                 */
	/***************************************************************************/
	template<class... Args>
	class PulserBase : public PolymorphicThread<Args...>
	{
		public:
			PulserBase()
			: m_interrupted(false)
			{
			}
			virtual void interrupt() { m_interrupted = true; }

		protected:
			virtual void tick() = 0;

		protected:
			std::atomic<bool> m_interrupted;
	};

	/**
	 * Pulses regularly (clock)
	 */
	template<class I>
	class ClockPulser : public PulserBase<>
	{
		public:
			using interval = I;

		public:
			ClockPulser(const interval& _interval)
			: m_interval(_interval)
			{
			}

		private:
			void run() final
			{
				while (true)
				{
					std::this_thread::sleep_for(m_interval);
					if (m_interrupted) { break; }
					tick();
				}
			}

		private:
			interval m_interval;
	};

	/**
	 * Pulses with an interval planning (interruptable)
	 */
	/*
	template<class I>
	class IntervalPulser : public PulserBase<>
	{
		public:
			using interval = I;

		public:
			IntervalPulser(const std::vector<interval>& _intervals)
			: m_intervals(_intervals)
			{
			}

		private:
			void run() final
			{
				for (interval it : m_intervals)
				{
					std::this_thread::sleep_for(it);
					if (m_interrupted) { break; }
					tick();
				}
			}

		private:
			std::vector<interval> m_intervals;
	};
	*/

	/**
	* Pulses with an epoch planning (interruptable)
	*/
	/*
	template<class C>
	class EpochPulser : public PulserBase<>
	{
		public:
			using clock     = C;
			using timepoint = std::chrono::time_point<clock>;

		public:
			EpochPulser(const std::vector<timepoint>& _timepoints)
			: m_timepoints(_timepoints)
			{
			}

		private:
			void run() final
			{
				for (timepoint target : m_timepoints)
				{
					std::cerr << std::chrono::duration<double>(target - clock::now()).count() << std::endl;
					std::this_thread::sleep_for(target - clock::now());
					if (m_interrupted) { break; }
					tick();
				}
			}

		private:
			std::vector<timepoint> m_timepoints;
	};
	*/

	/**
	 * Pulses based on notification (killed by interrupt)
	 */
	class NotifiedPulser : public madag::sync::PulserBase<>
	{
		public:
			~NotifiedPulser()
			{
				kill();
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
		private:
			void run()
			{
				for (;;)
				{
					m_notifiablelock.wait();
					if (m_interrupted) { break; }
					tick();
				}
			}
		public:
			void wakeup()
			{
				m_notifiablelock.notify();
			}
			void kill()
			{
				interrupt();
				wakeup(); // Needed for the thread to break
			}
		protected:
			notifiable m_notifiablelock;
	};

	/**
	 * Pulses based on notification with minimal delay between ticks
	 */
	template<class I>
	class DelayedNotifiedPulser : public NotifiedPulser
	{
		public:
			using interval = I;

		public:
			DelayedNotifiedPulser(const interval& _delay)
			: m_delay(_delay)
			{}
		private:
			void run()
			{
				for (;;)
				{
					m_notifiablelock.wait();
					if (m_interrupted) { break; }
					tick();
					std::this_thread::sleep_for(m_delay);
				}
			}
		private:
			interval m_delay;
	};

	/**
	 * Pulses once with lambda (interruptable)
	 */
	template<class I>
	class SelfDeletingTimer : public PulserBase<>
	{
		public:
			using callable = std::function<void(void)>;
			using interval = I;
			using instance = SelfDeletingTimer*;

		private:
			SelfDeletingTimer(const interval& _interval, callable&& _callback)
			: m_interval(_interval)
			, m_callback(_callback)
			{
			}
			void run() final
			{
				std::this_thread::sleep_for(m_interval);
				if (!m_interrupted) tick();
				delete this;
			}
			void tick()
			{
				m_callback();
			}
		public:
			void start()
			{
				PolymorphicThread::start();
				PolymorphicThread::detach();
			}
			static
			SelfDeletingTimer* factory(const interval& _interval, callable&& _callback)
			{
				return new SelfDeletingTimer(_interval, std::forward<callable&&>(_callback));
			}
		private:
			interval m_interval;
			callable m_callback;
	};

}

#endif
