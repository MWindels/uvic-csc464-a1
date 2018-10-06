#include <mutex>
#include <thread>
#include <vector>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <shared_mutex>
#include "cpp/shared/parse.hpp"

std::mutex output_mutex;	//This protects std::cout while the readers/writers are working.

/*class rw_mutex{
public:
	//Constructors/Destructor
	rw_mutex() = default;
	rw_mutex(const rw_mutex&) = delete;
	rw_mutex(rw_mutex&& other) = delete;
	~rw_mutex() = default;
	
	//Assignment Operators
	rw_mutex& operator=(const rw_mutex&) = delete;
	rw_mutex& operator=(rw_mutex&&) = delete;
	
	//Read Lock/Unlock
	void r_lock();
	void r_unlock();
	
	//Write Lock/Unlock
	void lock();
	void unlock();

private:
	//Lightswitch Components
	int readers;
	std::mutex no_readers;
	std::mutex self_lock;
	
	//Turnstile
	std::mutex turnstile;

};

void rw_mutex::r_lock(){
	turnstile.lock();
	turnstile.unlock();
	
	self_lock.lock();
	if(readers++ == 0){
		no_readers.lock();
	}
	self_lock.unlock();
}

void rw_mutex::r_unlock(){
	self_lock.lock();
	if(--readers == 0){
		no_readers.unlock();
	}
	self_lock.unlock();
}

void rw_mutex::lock(){
	turnstile.lock();
	no_readers.lock();
}

void rw_mutex::unlock(){
	turnstile.unlock();
	no_readers.unlock();
}*/

void reader(int id, int* data, /*rw_mutex* lock*/ std::shared_mutex* lock){
	int value;
	
	//lock->r_lock();
	lock->lock_shared();
	std::this_thread::sleep_for(std::chrono::milliseconds(std::rand() % 10));
	value = *data;
	lock->unlock_shared();
	//lock->r_unlock();
	
	output_mutex.lock();
	std::cout << "(Reader " << id << ") Read " << value << "\n";
	output_mutex.unlock();
}

void writer(int id, int* data, /*rw_mutex* lock*/ std::shared_mutex* lock){
	int value;
	
	//lock->lock();
	lock->lock();
	std::this_thread::sleep_for(std::chrono::milliseconds(std::rand() % 10));
	value = *data + 1;
	*data = value;
	lock->unlock();
	//lock->unlock();
	
	output_mutex.lock();
	std::cout << "(Writer " << id << ") Wrote " << value << "\n";
	output_mutex.unlock();
}

void test_scenario(int total_readers, int total_writers){
	int data = 0;
	//rw_mutex lock;
	std::shared_mutex lock;
	
	std::vector<std::thread> readers(total_readers);
	std::vector<std::thread> writers(total_writers);
	
	for(int i = 0, j = 0; i < total_readers || j < total_writers;){
		if(i < total_readers && j < total_writers){
			if(std::rand() % (1 + total_readers / (total_writers + 1))){
				readers.push_back(std::thread(reader, i++, &data, &lock));
			}else{
				writers.push_back(std::thread(writer, j++, &data, &lock));
			}
		}else if(i < total_readers){
			readers.push_back(std::thread(reader, i++, &data, &lock));
		}else if(j < total_writers){
			writers.push_back(std::thread(writer, j++, &data, &lock));
		}
	}
	
	for(auto i = readers.begin(); i != readers.end(); ++i){
		if(i->joinable()){
			i->join();
		}
	}
	for(auto i = writers.begin(); i != writers.end(); ++i){
		if(i->joinable()){
			i->join();
		}
	}
	
	std::cout << "Final value: " << data << "\n";
}

int main(){
	std::srand(std::time(0));
	try{
		std::cout << "Please input how many reader threads to run: ";
		int readers = scan_int();
		if(readers >= 0){
			std::cout << "Please input how many writer threads to run: ";
			int writers = scan_int();
			if(writers >= 0){
				test_scenario(readers, writers);
			}else{
				throw std::invalid_argument("Read a negative value from std::cin.");
			}
		}else{
			throw std::invalid_argument("Read a negative value from std::cin.");
		}
	}catch(const std::invalid_argument& ex){
		std::cout << "Please input a single natural number, and nothing else.\n";
	}
	return 0;
}