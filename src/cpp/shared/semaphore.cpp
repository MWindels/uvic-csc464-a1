#include "cpp/shared/semaphore.hpp"

void semaphore::wait(){
	std::unique_lock lk(lock);
	
	if(value == 0){
		waiter.wait(lk, [=](){return value > 0;});
	}
	--value;
}

void semaphore::signal(){
	std::unique_lock lk(lock);
	
	++value;
	waiter.notify_one();
}