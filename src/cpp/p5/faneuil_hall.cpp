#include <mutex>
#include <thread>
#include <chrono>
#include <vector>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <functional>
#include "cpp/shared/parse.hpp"
#include "cpp/shared/semaphore.hpp"

typedef std::chrono::steady_clock testing_clock;

std::mutex output_mutex;

class hall{
public:
	
	//Constructors/Destructor.
	hall() : try_enter(), checked_in(0), swear_oath(0), certification(0), try_leave(), notify_leave(0), entered(0) {}
	hall(const hall&) = delete;
	hall(hall&&) = delete;
	~hall() = default;
	
	//Assignment Operators.
	hall& operator=(const hall&) = delete;
	hall& operator=(hall&&) = delete;
	
	//Immigrant Functions.
	void enter_immigrant(int id);
	void check_in(int id);
	void swear(int id);
	void leave_immigrant(int id);
	
	//Judge Functions.
	void enter_judge(int prev_immigrants);
	void confirm();
	int leave_judge();
	
	//Spectator Functions.
	void enter_spectator(int id);
	void spectate(int id);
	void leave_spectator(int id);

private:
	
	//Synchronization Members.
	std::mutex try_enter;
	semaphore checked_in;
	semaphore swear_oath;
	semaphore certification;
	std::mutex try_leave;
	semaphore notify_leave;
	
	//Mutable Members.
	int entered;

};



//----------Immigrant Functions----------

void hall::enter_immigrant(int id){
	//testing_clock::time_point start = testing_clock::now();
	std::unique_lock lk(try_enter);
	
	/*output_mutex.lock();
	std::cout << std::chrono::duration_cast<std::chrono::nanoseconds>(testing_clock::now() - start).count() << "\n";
	output_mutex.unlock();*/
	
	++entered;
	
	output_mutex.lock();
	std::cout << "(Immigrant " << id << ") Arrives.\n";
	output_mutex.unlock();
}

void hall::check_in(int id){
	output_mutex.lock();
	std::cout << "(Immigrant " << id << ") Checks in.\n";
	output_mutex.unlock();
	
	checked_in.signal();
}

void hall::swear(int id){
	swear_oath.wait();
	
	output_mutex.lock();
	std::cout << "(Immigrant " << id << ") Swears their oath, and gets their certificate.\n";
	output_mutex.unlock();
	
	certification.signal();
}

void hall::leave_immigrant(int id){
	std::unique_lock lk(try_leave);
	
	output_mutex.lock();
	std::cout << "(Immigrant " << id << ") Leaves.\n";
	output_mutex.unlock();
	
	notify_leave.signal();
}



//----------Judge Functions----------

void hall::enter_judge(int prev_immigrants){
	for(int i = 0; i < prev_immigrants; ++i){
		notify_leave.wait();
	}
	
	try_enter.lock();
	try_leave.lock();
	
	output_mutex.lock();
	std::cout << "(The Judge) Arrives.\n";
	output_mutex.unlock();
}

void hall::confirm(){
	for(int i = 0; i < entered; ++i){
		checked_in.wait();
	}
	
	output_mutex.lock();
	std::cout << "(The Judge) Begins the confirmation process.\n";
	output_mutex.unlock();
	
	//testing_clock::time_point start = testing_clock::now();
	for(int i = 0; i < entered; ++i){
		swear_oath.signal();
		certification.wait();
	}
	/*output_mutex.lock();
	std::cout << std::chrono::duration_cast<std::chrono::nanoseconds>(testing_clock::now() - start).count() << "\n";
	output_mutex.unlock();*/
}

int hall::leave_judge(){
	output_mutex.lock();
	std::cout << "(The Judge) Leaves.\n";
	output_mutex.unlock();
	
	int prev_immigrants = entered;
	entered = 0;
	try_leave.unlock();
	try_enter.unlock();
	
	return prev_immigrants;
}



//----------Spectator Functions----------

void hall::enter_spectator(int id){
	//testing_clock::time_point start = testing_clock::now();
	std::unique_lock lk(try_enter);
	
	/*output_mutex.lock();
	std::cout << std::chrono::duration_cast<std::chrono::nanoseconds>(testing_clock::now() - start).count() << "\n";
	output_mutex.unlock();*/
	
	output_mutex.lock();
	std::cout << "(Spectator " << id << ") Arrives.\n";
	output_mutex.unlock();
}

void hall::spectate(int id){
	output_mutex.lock();
	std::cout << "(Spectator " << id << ") Spectates.\n";
	output_mutex.unlock();
	
	std::this_thread::sleep_for(std::chrono::milliseconds(std::rand() % 100));
}

void hall::leave_spectator(int id){
	output_mutex.lock();
	std::cout << "(Spectator " << id << ") Leaves.\n";
	output_mutex.unlock();
}



//----------Thread Functions----------

void immigrant(int id, hall& fh){
	std::this_thread::sleep_for(std::chrono::milliseconds(std::rand() % 2000));	//In transit.
	fh.enter_immigrant(id);
	std::this_thread::sleep_for(std::chrono::milliseconds(std::rand() % 200));	//Find way to check-in.
	fh.check_in(id);
	fh.swear(id);
	fh.leave_immigrant(id);
}

void judge(hall& fh){
	int prev_immigrants = 0;
	while(true){
		std::this_thread::sleep_for(std::chrono::milliseconds(std::rand() % 10));	//In transit.
		fh.enter_judge(prev_immigrants);
		fh.confirm();
		prev_immigrants = fh.leave_judge();
	}
}

void spectator(int id, hall& fh){
	std::this_thread::sleep_for(std::chrono::milliseconds(std::rand() % 2000));	//In transit.
	fh.enter_spectator(id);
	fh.spectate(id);
	fh.leave_spectator(id);
}

void test_scenario(int total_immigrants, int total_spectators){
	hall fh;
	
	std::thread the_judge(judge, std::ref(fh));
	the_judge.detach();
	
	std::vector<std::thread> immigrants;
	std::vector<std::thread> spectators;
	
	for(int i = 0, j = 0; i + j < total_immigrants + total_spectators; ){
		std::this_thread::sleep_for(std::chrono::milliseconds(std::rand() % 10));
		if(i < total_immigrants && j < total_spectators){
			if(std::rand() % 2 == 0){
				immigrants.push_back(std::thread(immigrant, i++, std::ref(fh)));
			}else{
				spectators.push_back(std::thread(spectator, j++, std::ref(fh)));
			}
		}else if(i < total_immigrants){
			immigrants.push_back(std::thread(immigrant, i++, std::ref(fh)));
		}else if(j < total_spectators){
			spectators.push_back(std::thread(spectator, j++, std::ref(fh)));
		}
	}
	
	for(auto i = immigrants.begin(); i != immigrants.end(); ++i){
		if(i->joinable()){
			i->join();
		}
	}
	for(auto i = spectators.begin(); i != spectators.end(); ++i){
		if(i->joinable()){
			i->join();
		}
	}
}

int main(){
	std::srand(std::time(0));
	try{
		std::cout << "Please input how many immigrant threads to run: ";
		int immigrants = scan_int();
		if(immigrants >= 0){
			std::cout << "Please input how many spectator threads to run: ";
			int spectators = scan_int();
			if(spectators >= 0){
				test_scenario(immigrants, spectators);
			}else{
				throw std::invalid_argument("Read a value less than zero from std::cin.");
			}
		}else{
			throw std::invalid_argument("Read a value less than zero from std::cin.");
		}
	}catch(const std::invalid_argument& ex){
		std::cout << "Please input a single natural number, and nothing else.";
	}
	return 0;
}