#include <mutex>
#include <thread>
#include <vector>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <shared_mutex>
#include "cpp/shared/parse.hpp"

typedef std::chrono::steady_clock testing_clock;

std::mutex output_mutex;	//This protects std::cout while the readers/writers are working.

void reader(int id, int* data, std::shared_mutex* lock){
	//testing_clock::time_point start = testing_clock::now();
	lock->lock_shared();
	//testing_clock::time_point end = testing_clock::now();
	
	output_mutex.lock();
	std::cout << "(Reader " << id << ") Begins reading...\n";
	output_mutex.unlock();
	
	std::this_thread::sleep_for(std::chrono::milliseconds(std::rand() % 10));
	
	int value = *data;
	++value;
	
	output_mutex.lock();
	std::cout << "(Reader " << id << ") Read " << *data << ".\n";
	output_mutex.unlock();
	
	lock->unlock_shared();
	
	/*output_mutex.lock();
	std::cout << std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() << "\n";
	output_mutex.unlock();*/
}

void writer(int id, int* data, std::shared_mutex* lock){
	//testing_clock::time_point start = testing_clock::now();
	lock->lock();
	//testing_clock::time_point end = testing_clock::now();
	
	output_mutex.lock();
	std::cout << "(Writer " << id << ") Begins writing...\n";
	output_mutex.unlock();
	
	std::this_thread::sleep_for(std::chrono::milliseconds(std::rand() % 10));
	
	*data = *data + 1;
	
	output_mutex.lock();
	std::cout << "(Writer " << id << ") Wrote " << *data << ".\n";
	output_mutex.unlock();
	
	lock->unlock();
	
	/*output_mutex.lock();
	std::cout << std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() << "\n";
	output_mutex.unlock();*/
}

void test_scenario(int total_readers, int total_writers){
	int data = 0;
	std::shared_mutex lock;
	
	std::vector<std::thread> readers(total_readers);
	std::vector<std::thread> writers(total_writers);
	
	for(int i = 0, j = 0; i < total_readers || j < total_writers;){
		std::this_thread::sleep_for(std::chrono::milliseconds(std::rand() % 5));
		if(i < total_readers && j < total_writers){
			if(std::rand() % 2 == 0){
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
	
	//std::cout << "Final value: " << data << "\n";
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