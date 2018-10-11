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

std::mutex output_mutex;

std::list<int>::iterator find(int x, std::list<int>& ctnr){
	for(auto i = ctnr.begin(); i != ctnr.end(); ++i){
		if(*i == x){
			return i;
		}
	}
	return ctnr.end();
}

void searcher(int id, std::list<int>& ctnr, std::shared_mutex& delete_lock){
	std::shared_lock<std::shared_mutex> del_lk(delete_lock);
	
	if(find(id, ctnr) != ctnr.end()){
		std::cout << "(Searcher " << id << ") Found element {" << id << "}.\n";
	}else{
		std::cout << "(Searcher " << id << ") Did not find element {" << id << "}!\n";
	}
}

void inserter(int id, std::list<int>& ctnr, std::shared_mutex& delete_lock, std::mutex& insert_lock){
	std::shared_lock<std::shared_mutex> del_lk(delete_lock);
	std::unique_lock ins_lk(insert_lock);
	
	ctnr.push_back(id);
	std::cout << "(Inserter " << id << ") Added element {" << id << "}.\n";
}

void deleter(int id, std::list<int>& ctnr, std::shared_mutex& delete_lock){
	std::unique_lock<std::shared_mutex> del_lk(delete_lock);
	
	std::list<int>::iterator elem = find(id, ctnr);
	if(elem != ctnr.end()){
		ctnr.erase(elem);
		std::cout << "(Deleter " << id << ") Removed element {" << id << "}.\n";
	}else{
		std::cout << "(Deleter " << id << ") Did not find element {" << id << "}!\n";
	}
}

void test_scenario(int total_searchers, int total_inserters, int total_deleters){
	std::list<int> ctnr;
	std::shared_mutex delete_lock;
	std::mutex insert_lock;
	
	std::vector<std::thread> searchers;
	std::vector<std::thread> inserters;
	std::vector<std::thread> deleters;
	
	for(int i = 0, j = 0, k = 0; i + j + k < total_searchers + total_inserters + total_deleters; ){
		std::this_thread::sleep_for(std::chrono::milliseconds(std::rand() % 100));
		if(i < total_searchers && j < total_inserters && k < total_deleters){
			if(std::rand() % 3 == 0){
				searchers.push_back(std::thread(searcher, i++, std::ref(ctnr), std::ref(delete_lock)));
			}else{
				if(std::rand() % 2 == 0){
					inserters.push_back(std::thread(inserter, j++, std::ref(ctnr), std::ref(delete_lock), std::ref(insert_lock)));
				}else{
					deleters.push_back(std::thread(deleter, k++, std::ref(ctnr), std::ref(delete_lock)));
				}
			}
		}else if(i < total_searchers && j < total_inserters){
			if(std::rand() % 2 == 0){
				searchers.push_back(std::thread(searcher, i++, std::ref(ctnr), std::ref(delete_lock)));
			}else{
				inserters.push_back(std::thread(inserter, j++, std::ref(ctnr), std::ref(delete_lock), std::ref(insert_lock)));
			}
		}else if(i < total_searchers && k < total_deleters){
			if(std::rand() % 2 == 0){
				searchers.push_back(std::thread(searcher, i++, std::ref(ctnr), std::ref(delete_lock)));
			}else{
				deleters.push_back(std::thread(deleter, k++, std::ref(ctnr), std::ref(delete_lock)));
			}
		}else if(j < total_inserters && k < total_deleters){
			if(std::rand() % 2 == 0){
				inserters.push_back(std::thread(inserter, j++, std::ref(ctnr), std::ref(delete_lock), std::ref(insert_lock)));
			}else{
				deleters.push_back(std::thread(deleter, k++, std::ref(ctnr), std::ref(delete_lock)));
			}
		}else if(i < total_searchers){
			searchers.push_back(std::thread(searcher, i++, std::ref(ctnr), std::ref(delete_lock)));
		}else if(j < total_inserters){
			inserters.push_back(std::thread(inserter, j++, std::ref(ctnr), std::ref(delete_lock), std::ref(insert_lock)));
		}else if(k < total_deleters){
			deleters.push_back(std::thread(deleter, k++, std::ref(ctnr), std::ref(delete_lock)));
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