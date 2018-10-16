#include <list>
#include <thread>
#include <chrono>
#include <vector>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <functional>
#include <shared_mutex>
#include "cpp/shared/parse.hpp"

typedef std::chrono::steady_clock testing_clock;

std::mutex output_mutex;

struct container{
	
	//Constructors/Destructor.
	container() : delete_lock(), insert_lock(), size_lock(), ctnr() {}
	container(const container&) = delete;
	container(container&&) = delete;
	~container() = default;
	
	//Assignment Operators.
	container& operator=(const container&) = delete;
	container& operator=(container&&) = delete;
	
	//Container Functions.
	std::list<int>::iterator find(int x);
	
	//Synchronization Members.
	std::shared_mutex delete_lock;
	std::mutex insert_lock;
	std::mutex size_lock;
	
	//Mutable Members.
	std::list<int> ctnr;

};

std::list<int>::iterator container::find(int x){
	size_lock.lock();
	int size = ctnr.size();
	size_lock.unlock();
	
	auto item = ctnr.begin();
	for(int i = 0; i < size; ++i){
		if(*item == x){
			return item;
		}
		++item;
	}
	throw std::range_error("The element is not in the list.");
}

void searcher(int id, container& c){
	std::this_thread::sleep_for(std::chrono::milliseconds(1));
	//testing_clock::time_point start = testing_clock::now();
	
	std::shared_lock<std::shared_mutex> del_lk(c.delete_lock);
	
	try{
		c.find(id);
		
		output_mutex.lock();
		std::cout << "(Searcher " << id << ") Found element {" << id << "}.\n";
		output_mutex.unlock();
	}catch(const std::range_error& ex){
		output_mutex.lock();
		std::cout << "(Searcher " << id << ") Did not find element {" << id << "}!\n";
		output_mutex.unlock();
	}
	
	/*output_mutex.lock();
	std::cout << std::chrono::duration_cast<std::chrono::nanoseconds>(testing_clock::now() - start).count() << "\n";
	output_mutex.unlock();*/
}

void inserter(int id, container& c){
	std::this_thread::sleep_for(std::chrono::milliseconds(1));
	//testing_clock::time_point start = testing_clock::now();
	
	std::shared_lock<std::shared_mutex> del_lk(c.delete_lock);
	std::unique_lock ins_lk(c.insert_lock);
	
	c.size_lock.lock();
	c.ctnr.push_back(id);
	c.size_lock.unlock();
	
	/*output_mutex.lock();
	std::cout << std::chrono::duration_cast<std::chrono::nanoseconds>(testing_clock::now() - start).count() << "\n";
	output_mutex.unlock();*/
	
	output_mutex.lock();
	std::cout << "(Inserter " << id << ") Added element {" << id << "}.\n";
	output_mutex.unlock();
}

void deleter(int id, container& c){
	std::this_thread::sleep_for(std::chrono::milliseconds(1));
	//testing_clock::time_point start = testing_clock::now();
	
	std::unique_lock<std::shared_mutex> del_lk(c.delete_lock);
	
	try{
		std::list<int>::iterator elem = c.find(id);
		c.ctnr.erase(elem);
		
		output_mutex.lock();
		std::cout << "(Deleter " << id << ") Removed element {" << id << "}.\n";
		output_mutex.unlock();
	}catch(const std::range_error& ex){
		output_mutex.lock();
		std::cout << "(Deleter " << id << ") Did not find element {" << id << "}!\n";
		output_mutex.unlock();
	}
	
	/*output_mutex.lock();
	std::cout << std::chrono::duration_cast<std::chrono::nanoseconds>(testing_clock::now() - start).count() << "\n";
	output_mutex.unlock();*/
}

void test_scenario(int total_searchers, int total_inserters, int total_deleters){
	container the_container;
	
	std::vector<std::thread> searchers;
	std::vector<std::thread> inserters;
	std::vector<std::thread> deleters;
	
	for(int i = 0, j = 0, k = 0; i + j + k < total_searchers + total_inserters + total_deleters; ){
		//std::this_thread::sleep_for(std::chrono::milliseconds(std::rand() % 5));
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		if(i < total_searchers && j < total_inserters && k < total_deleters){
			if(std::rand() % 3 == 0){
				searchers.push_back(std::thread(searcher, i++, std::ref(the_container)));
			}else{
				if(std::rand() % 2 == 0){
					inserters.push_back(std::thread(inserter, j++, std::ref(the_container)));
				}else{
					deleters.push_back(std::thread(deleter, k++, std::ref(the_container)));
				}
			}
		}else if(i < total_searchers && j < total_inserters){
			if(std::rand() % 2 == 0){
				searchers.push_back(std::thread(searcher, i++, std::ref(the_container)));
			}else{
				inserters.push_back(std::thread(inserter, j++, std::ref(the_container)));
			}
		}else if(i < total_searchers && k < total_deleters){
			if(std::rand() % 2 == 0){
				searchers.push_back(std::thread(searcher, i++, std::ref(the_container)));
			}else{
				deleters.push_back(std::thread(deleter, k++, std::ref(the_container)));
			}
		}else if(j < total_inserters && k < total_deleters){
			if(std::rand() % 2 == 0){
				inserters.push_back(std::thread(inserter, j++, std::ref(the_container)));
			}else{
				deleters.push_back(std::thread(deleter, k++, std::ref(the_container)));
			}
		}else if(i < total_searchers){
			searchers.push_back(std::thread(searcher, i++, std::ref(the_container)));
		}else if(j < total_inserters){
			inserters.push_back(std::thread(inserter, j++, std::ref(the_container)));
		}else if(k < total_deleters){
			deleters.push_back(std::thread(deleter, k++, std::ref(the_container)));
		}
	}
	
	for(auto i = searchers.begin(); i != searchers.end(); ++i){
		if(i->joinable()){
			i->join();
		}
	}
	for(auto i = inserters.begin(); i != inserters.end(); ++i){
		if(i->joinable()){
			i->join();
		}
	}
	for(auto i = deleters.begin(); i != deleters.end(); ++i){
		if(i->joinable()){
			i->join();
		}
	}
}

int main(){
	std::srand(std::time(0));
	try{
		std::cout << "Please input how many searcher threads to run: ";
		int searchers = scan_int();
		if(searchers >= 0){
			std::cout << "Please input how many inserter threads to run: ";
			int inserters = scan_int();
			if(inserters >= 0){
				std::cout << "Please input how many deleter threads to run: ";
				int deleters = scan_int();
				if(deleters >= 0){
					test_scenario(searchers, inserters, deleters);
				}else{
					throw std::invalid_argument("Read a value less than zero from std::cin.");
				}
			}else{
				throw std::invalid_argument("Read a value less than zero from std::cin.");
			}
		}else{
			throw std::invalid_argument("Read a value less than zero from std::cin.");
		}
	}catch(const std::invalid_argument& ex){
		std::cout << "Please input a single integer larger than or equal to two, and nothing else.";
	}
	return 0;
}