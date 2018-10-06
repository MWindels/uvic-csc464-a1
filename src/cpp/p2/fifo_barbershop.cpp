#include <mutex>
#include <vector>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <functional>
#include "cpp/shared/parse.hpp"
#include "cpp/shared/ts_queue.hpp"
#include "cpp/shared/semaphore.hpp"

std::mutex output_mutex;

struct customer_info{
	customer_info(int i = 0, semaphore* s = NULL) : id(i), sem(s) {}
	
	int id;
	semaphore* sem;
};

void customer(int id, ts_queue<customer_info>& queue){
	semaphore sem;
	customer_info info(id, &sem);
	std::this_thread::sleep_for(std::chrono::milliseconds(std::rand() % 3000));	//Walk to the barbershop...
	
	if(queue.enqueue(info)){	//Shop is not full, enter.
		output_mutex.lock();
		std::cout << "(Customer " << id << ") Enters.\n";	//This isn't perfect, one thread could get the lock first, even though the other go into the queue first.
		output_mutex.unlock();
		
		sem.wait();	//Wait until barber calls you up.
		//Get hair cut...
		sem.wait();	//Wait until barber is done.
	}else{
		output_mutex.lock();
		std::cout << "(Customer " << id << ") The shop is full!\n";		//Shop is full, balk and leave.
		output_mutex.unlock();
	}
}

void barber(ts_queue<customer_info>& queue){
	customer_info next;
	while(queue.dequeue(next)){	//Wait for a customer.
		next.sem->signal();	//Call customer up.
		
		output_mutex.lock();
		std::cout << "(Barber) Customer " << next.id << "!\n";
		output_mutex.unlock();
		std::this_thread::sleep_for(std::chrono::milliseconds(std::rand() % 3000));	//Cut their hair...
		output_mutex.lock();
		std::cout << "(Barber) All done, customer " << next.id << ".\n";
		output_mutex.unlock();
		
		next.sem->signal();	//Tell customer they're done.
	}
}

void test_scenario(int total_customers, int shop_capacity){
	ts_queue<customer_info> queue(shop_capacity);
	
	std::thread barber_thread(barber, std::ref(queue));
	std::vector<std::thread> customers(total_customers);
	for(int i = 0; i < total_customers; ++i){
		customers.push_back(std::thread(customer, i, std::ref(queue)));
	}
	
	for(auto i = customers.begin(); i != customers.end(); ++i){
		if(i->joinable()){
			i->join();
		}
	}
	
	queue.close();
	if(barber_thread.joinable()){
		barber_thread.join();
	}
}

int main(){
	std::srand(std::time(0));
	try{
		std::cout << "Please input how many customers to run: ";
		int customers = scan_int();
		if(customers >= 0){
			std::cout << "Please input how many chairs there are in the barbershop's waiting room: ";
			int capacity = scan_int();
			if(capacity >= 0){
				test_scenario(customers, capacity);
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