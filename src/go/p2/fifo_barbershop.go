package main

import (
	"fmt"
	"time"
	"math/rand"
	"go/shared/parse"
)

func customer(id int, queue chan<- int, notify <-chan bool, done chan<- bool) {
	time.Sleep(time.Duration(rand.Intn(10000)) * time.Millisecond)		//Walk to the barbershop...
	
	select {
	case queue <- id:	//Shop is not full, enter.
		fmt.Printf("(Customer %d) Enters.\n", id)
		<- notify	//Wait until barber calls you up.
		//Get hair cut...
		<- notify	//Wait until barber is done.
	default:
		fmt.Printf("(Customer %d) The shop is full!\n", id)		//Shop is full, balk and leave.
	}
	
	done <- true
}

func barber(queue <-chan int, notifiers [](chan bool)) {
	for {
		customer := <- queue			//Wait for a customer.
		notifiers[customer] <- true		//Call customer up.
		
		fmt.Printf("(Barber) Customer %d!\n", customer)
		time.Sleep(time.Duration(rand.Intn(3000)) * time.Millisecond)	//Cut their hair...
		fmt.Printf("(Barber) All done, customer %d.\n", customer)
		
		notifiers[customer] <- true		//Tell customer they're done.
	}
}

func testScenario(totalCustomers, shopCapacity int) {
	queue := make(chan int, shopCapacity)
	notifiers := make([](chan bool), totalCustomers, totalCustomers)
	for i := 0; i < totalCustomers; i++ {
		notifiers[i] = make(chan bool)
	}
	done := make(chan bool)
	
	go barber(queue, notifiers)
	for i := 0; i < totalCustomers; i++ {
		go customer(i, queue, notifiers[i], done)
	}
	
	for i := 0; i < totalCustomers; i++ {
		<- done
	}
}

func main() {
	rand.Seed(time.Now().UTC().UnixNano())
	
	fmt.Print("Please input how many customers to run: ")
	if customers, customersErr := parse.ScanInt(); !customersErr && customers >= 0 {
		fmt.Print("Please input how many chairs there are in the barbershop's waiting room: ")
		if capacity, capacityErr := parse.ScanInt(); !capacityErr && capacity >= 0 {
			testScenario(customers, capacity)
		}else{
			fmt.Println("Please input a single natural number, and nothing else.")
		}
	}else{
		fmt.Println("Please input a single natural number, and nothing else.")
	}
}