#include <list>
#include <vector>
#include <thread>
#include <iostream>
#include <functional>
#include "cpp/shared/parse.hpp"
#include "cpp/shared/semaphore.hpp"

struct shared_lists{
	
	std::list<std::list<int>> sieve_data;	//These lists are used specifically so that iterators are not invalidated after insertions.
	std::list<semaphore> sieve_sems;

};

void generate(int n, std::list<std::list<int>>::iterator output, std::list<semaphore>::iterator output_notify){
	for(int i = 2; i <= n; ++i){
		output->push_back(i);
		output_notify->signal();
	}
	output->push_back(0);
	output_notify->signal();
}

void sieve(std::list<std::list<int>>::iterator input, std::list<semaphore>::iterator input_notify, shared_lists& shared_data, std::vector<int>& primes){
	input_notify->wait();
	int prime = *(input->begin());
	
	primes.push_back(prime);	//Doesn't need a mutex, no two threads will ever excute this at the same time.  The thread which created this one already added its prime before spinning off this thread.
	
	shared_data.sieve_data.push_back(std::list<int>());	//Likewise with this little block, and for exactly the same reasons.
	shared_data.sieve_sems.emplace_back(0);
	std::list<std::list<int>>::iterator output = --(shared_data.sieve_data.end());
	std::list<semaphore>::iterator output_notify = --(shared_data.sieve_sems.end());
	
	std::thread next;
	bool has_next = false;
	std::list<int>::iterator value = input->begin();
	input_notify->wait();
	while(*(++value) != 0){
		if(*value % prime != 0){
			if(!has_next){
				has_next = true;
				next = std::thread(sieve, output, output_notify, std::ref(shared_data), std::ref(primes));
			}
			output->push_back(*value);
			output_notify->signal();
		}
		input_notify->wait();
	}
	
	if(has_next){
		output->push_back(0);
		output_notify->signal();
		if(next.joinable()){
			next.join();
		}
	}
}

std::vector<int> find_primes_up_to(int n){
	shared_lists data;
	std::vector<int> primes;
	
	data.sieve_data.push_back(std::list<int>());
	data.sieve_sems.emplace_back(0);
	
	std::thread generator(generate, n, data.sieve_data.begin(), data.sieve_sems.begin());
	std::thread first_sieve(sieve, data.sieve_data.begin(), data.sieve_sems.begin(), std::ref(data), std::ref(primes));
	
	if(generator.joinable()){
		generator.join();
	}
	if(first_sieve.joinable()){
		first_sieve.join();
	}
	
	return primes;
}

void test_scenario(int n){
	std::vector<int> primes = find_primes_up_to(n);
	
	std::cout << "The prime numbers from two to " << n << "\n";
	for(auto i = primes.begin(); i != primes.end(); ++i){
		auto next = (++i)--;
		if(next != primes.end()){
			std::cout << *i << ", ";
		}else{
			std::cout << *i << "\n";
		}
	}
}

int main(){
	try{
		std::cout << "Please input which number to print the primes up to: ";
		int max = scan_int();
		if(max >= 2){
			test_scenario(max);
		}else{
			throw std::invalid_argument("Read a value less than two from std::cin.");
		}
	}catch(const std::invalid_argument& ex){
		std::cout << "Please input a single integer larger than or equal to two, and nothing else.";
	}
	return 0;
}