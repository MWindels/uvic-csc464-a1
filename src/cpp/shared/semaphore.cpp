#include "cpp/shared/semaphore.hpp"

void semaphore::wait(){
	std::unique_lock lk(lock);
	
	--value;
	waiter.wait(lk, [=](){return value >= 0;});
}

void semaphore::signal(){
	std::unique_lock lk(lock);
	
	++value;
	waiter.notify_one();
}