#include <mutex>
#include <queue>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <functional>
#include <condition_variable>
#include "cpp/shared/parse.hpp"
#include "cpp/shared/semaphore.hpp"

std::mutex output_lock;

class cart;
class park;

class cart{
public:
	
	//Constructors/Destructor.
	cart(int i, int c) : unload_ready(0), lock(), is_full(), is_empty(), passenger_holder(0), id(i), capacity(c), passengers(0) {}
	cart(const cart&) = delete;
	cart(cart&&) = delete;
	~cart() = default;
	
	//Assignment Operators.
	cart& operator=(const cart&) = delete;
	cart& operator=(cart&&) = delete;
	
	//Simple Accessors.
	int get_id() const {return id;}
	int get_capacity() const {return capacity;}
	
	//Passenger-usable Functions.
	bool board(int pass_id);
	void ride();
	void unboard(int pass_id);
	
	mutable semaphore unload_ready;	//Unfortunately, has to be public for the park to access it.

private:
	
	//Car-only Functions.
	void load();
	void run();
	void unload();
	
	friend void car(cart& me, park& the_park);
	
	mutable std::mutex lock;
	mutable std::condition_variable is_full;
	mutable std::condition_variable is_empty;
	mutable semaphore passenger_holder;
	
	const int id;
	const int capacity;
	int passengers;

};

void cart::load(){
	std::unique_lock lk(lock);
	
	is_full.wait(lk, [=](){return passengers == capacity;});
}

void cart::run(){
	output_lock.lock();
	std::cout << "(Car " << id << ") Now running...\n";
	output_lock.unlock();
	
	std::this_thread::sleep_for(std::chrono::milliseconds(std::rand() % 5000));	
	unload_ready.wait();
	
	output_lock.lock();
	std::cout << "(Car " << id << ") Finished.\n";
	output_lock.unlock();
}

void cart::unload(){
	std::unique_lock lk(lock);
	
	for(int i = 0; i < passengers; ++i){
		passenger_holder.signal();
	}
	
	is_empty.wait(lk, [=](){return passengers == 0;});
}

bool cart::board(int pass_id){
	std::unique_lock lk(lock);
	
	if(passengers < capacity){
		if(++passengers == capacity){
			is_full.notify_one();
		}
		output_lock.lock();
		std::cout << "(Passenger " << pass_id << ") Boards car " << id << ".\n";
		output_lock.unlock();
		return true;
	}
	return false;
}

void cart::ride(){
	passenger_holder.wait();	//Wait until released.
}

void cart::unboard(int pass_id){
	std::unique_lock lk(lock);
	
	if(--passengers == 0){
		is_empty.notify_one();
	}
	output_lock.lock();
	std::cout << "(Passenger " << pass_id << ") Disembarks from car " << id << ".\n";
	output_lock.unlock();
}





class park{
public:
	
	//Constructors/Destructor.
	park(cart* cars, int n);
	park(const park&) = delete;
	park(park&&) = delete;
	~park() = default;
	
	//Assignment Operators.
	park& operator=(const park&) = delete;
	park& operator=(park&&) = delete;
	
	//Park Interaction Functions.
	cart* queue_for_car();

private:
	
	//Car-Only Interaction Functions.
	void start_car();
	void return_car();
	
	friend void car(cart& me, park& the_park);
	
	mutable std::mutex lock;
	mutable semaphore has_car_ready;
	
	std::queue<cart*> waiting_cars;
	std::queue<cart*> running_cars;
	cart* loading_car;
	cart* unloading_car;

};

park::park(cart* cars, int n) : lock(), has_car_ready(0), waiting_cars(), running_cars(), loading_car(NULL), unloading_car(NULL) {
	if(n > 0){
		loading_car = cars;
		for(int i = 0; i < cars[0].get_capacity(); ++i){
			has_car_ready.signal();
		}
		for(int i = 1; i < n; ++i){
			waiting_cars.push(cars + i);
		}
	}
}

void park::start_car(){
	std::unique_lock lk(lock);
	
	if(loading_car != NULL){
		if(unloading_car == NULL){
			unloading_car = loading_car;
			unloading_car->unload_ready.signal();
		}else{
			running_cars.push(loading_car);
		}
		
		if(!waiting_cars.empty()){
			loading_car = waiting_cars.front();
			waiting_cars.pop();
			for(int i = 0; i < loading_car->get_capacity(); ++i){
				has_car_ready.signal();
			}
		}else{
			loading_car = NULL;
		}
	}
}

void park::return_car(){
	std::unique_lock lk(lock);
	
	if(unloading_car != NULL){
		if(loading_car == NULL){
			loading_car = unloading_car;
			for(int i = 0; i < loading_car->get_capacity(); ++i){
				has_car_ready.signal();
			}
		}else{
			waiting_cars.push(unloading_car);
		}
		
		if(!running_cars.empty()){
			unloading_car = running_cars.front();
			running_cars.pop();
			unloading_car->unload_ready.signal();
		}else{
			unloading_car = NULL;
		}
	}
}

cart* park::queue_for_car(){
	has_car_ready.wait();
	
	std::unique_lock lk(lock);
	return loading_car;
}





void car(cart& me, park& the_park){
	while(true){
		me.load();
		the_park.start_car();
		me.run();
		me.unload();
		the_park.return_car();
	}
}

void passenger(int id, park& the_park){
	cart* ride = the_park.queue_for_car();
	ride->board(id);
	ride->ride();
	ride->unboard(id);
}

void test_scenario(int total_passengers, int total_cars, int total_seats){
	cart* the_cars = reinterpret_cast<cart*>(new char[total_cars * sizeof(cart)]);	//Allocates space without initializing.
	for(int i = 0; i < total_cars; ++i){
		new (the_cars + i) cart(i, total_seats);	//Initializes into a specific memory location.
	}
	park the_park(the_cars, total_cars);
	
	std::vector<std::thread> cars;
	std::vector<std::thread> passengers;
	for(int i = 0; i < total_cars; ++i){
		cars.push_back(std::thread(car, std::ref(the_cars[i]), std::ref(the_park)));
		(--cars.end())->detach();
	}
	for(int i = 0; i < total_passengers; ++i){
		passengers.push_back(std::thread(passenger, i, std::ref(the_park)));
	}
	
	for(auto i = passengers.begin(); i != passengers.end(); ++i){
		if(i->joinable()){
			i->join();
		}
	}
	
	delete [] the_cars;
}

int main(){
	std::srand(std::time(0));
	try{
		std::cout << "Please input how many passenger threads to run: ";
		int passengers = scan_int();
		if(passengers >= 0){
			std::cout << "Please input how many roller coaster car threads to run: ";
			int cars = scan_int();
			if(cars >= 0){
				std::cout << "Please input how many seats there are in the roller coaster cars: ";
				int seats = scan_int();
				if(seats < passengers){
					test_scenario(passengers, cars, seats);
				}else{
					throw std::invalid_argument("Please input a positive integer less than the number of passengers, and nothing else.");
				}
			}else{
				throw std::invalid_argument("Please input a single natural number, and nothing else.");
			}
		}else{
			throw std::invalid_argument("Please input a single natural number, and nothing else.");
		}
	}catch(const std::invalid_argument& ex){
		std::cout << ex.what();
	}
	return 0;
}