#ifndef SINGLETON_HH
#define SINGLETON_HH

#include <memory>

template<typename T>
class Singleton
{
	protected:
		Singleton () = default;
		~Singleton() = default;
	public:
		template<class... Args>
		static T& init(Args&&... args)
		{
			_singleton = std::unique_ptr<T>(new T(std::forward<Args>(args)...));
			return *_singleton.get();
		}
		static T& get()
		{
			if (!_singleton)
			{
				// ERROR: cannot use unitialized singleton
				throw std::runtime_error("ERROR: Access to unitialized object.");
			}
			return *_singleton.get();
		}
		static bool initialized()
		{
			return bool(_singleton);
		}
		static void kill()
		{
			_singleton.reset();
		}
	protected:
		static std::unique_ptr<T> _singleton;
};
template<typename T> std::unique_ptr<T> Singleton<T>::_singleton = nullptr;

#endif
