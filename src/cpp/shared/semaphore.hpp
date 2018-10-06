#ifndef SEMAPHORE_H_INCLUDED
#define SEMAPHORE_H_INCLUDED

#include <mutex>
#include <condition_variable>

class semaphore{
public:
	
	//Constructors/Destructor.
	semaphore(int i = 0) : lock(), waiter(), value(i) {}
	semaphore(const semaphore&) = delete;
	semaphore(semaphore&&) = delete;
	~semaphore() = default;
	
	//Assignment Operators.
	semaphore& operator=(const semaphore&) = delete;
	semaphore& operator=(semaphore&&) = delete;
	
	//Semaphore Operations.
	void wait();
	void signal();

private:
	
	mutable std::mutex lock;
	mutable std::condition_variable waiter;
	int value;

};

#endif