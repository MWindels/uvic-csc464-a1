#ifndef TS_QUEUE_H_INCLUDED
#define TS_QUEUE_H_INCLUDED

#include <mutex>
#include <queue>
#include <cstdlib>
#include <condition_variable>

/*
 * This object represents a thread-safe queue.
 */
template <class T>
class ts_queue {
public:
	
	using value_type = T;
	
	//Constructors/Destructor
	ts_queue(long max = -1) : lock(), not_empty(), queue(), is_closed(false), maximum(max) {}
	ts_queue(const ts_queue&) = delete;
	ts_queue(ts_queue&&) = delete;
	~ts_queue() {clear(); close();}
	
	//Assignment Operators
	ts_queue& operator=(const ts_queue&) = delete;
	ts_queue& operator=(ts_queue&&) = delete;
	
	//Queue Operations
	bool empty() const {std::unique_lock lk(lock);  return queue.empty();}		//Returns whether or not the queue is empty.
	bool enqueue(const value_type&);											//Adds an element to the end of the queue.
	bool dequeue(value_type&);													//Removes an element from the front of the queue.  Blocks if queue is not closed, doesn't otherwise.
	void clear();																//Removes everything from the queue.
	
	//Clean-up Operations
	bool closed() const {std::unique_lock lk(lock);  return is_closed;}					//Returns whether or not the queue is closed.
	void close() {std::unique_lock lk(lock); is_closed = true; not_empty.notify_all();}	//Closes the queue.  Prevents enqueues, makes dequeues non-blocking.
	

private:
	
	mutable std::mutex lock;
	mutable std::condition_variable not_empty;
	std::queue<value_type> queue;
	
	mutable bool is_closed;
	long maximum;

};

template <class T>
bool ts_queue<T>::enqueue(const value_type& elem){
	std::unique_lock lk(lock);
	
	if(is_closed || (maximum >= 0 && queue.size() == std::size_t(maximum))){
		return false;
	}
	
	queue.push(elem);
	not_empty.notify_one();
	return true;
}

template <class T>
bool ts_queue<T>::dequeue(value_type& ret){
	std::unique_lock lk(lock);
	
	if(is_closed && queue.empty()){
		return false;		//In case it was closed and emptied before waiting.
	}
	not_empty.wait(lk, [=](){return !queue.empty() || is_closed;});
	if(is_closed && queue.empty()){
		return false;		//In case it was closed and emptied while waiting.
	}
	
	ret = queue.front();
	queue.pop();
	return true;
}

template <class T>
void ts_queue<T>::clear(){
	std::unique_lock lk(lock);
	
	while(!queue.empty()){
		queue.pop();
	}
}

#endif