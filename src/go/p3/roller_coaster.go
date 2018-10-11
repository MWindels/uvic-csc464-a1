package main

import (
	"fmt"
	"time"
	"math/rand"
	"go/shared/parse"
)

type carChannels struct {
	freeSeatTokens chan int
	passengerEmbark chan bool
	passengerDisembark chan bool
}

func load(id int, seats int, chs carChannels) {
	for i := 0; i < seats; i++ {
		<- chs.passengerEmbark		//Accept passengers into the car.
	}
}

func run(id int) {
	fmt.Printf("(Car %d) Now running...\n", id)
	time.Sleep(time.Duration(rand.Intn(5000)) * time.Millisecond)	//Run the coaster.
	fmt.Printf("(Car %d) Finished.\n", id)
}

func unload(seats int, chs carChannels) {
	for i := 0; i < seats; i++ {
		chs.passengerDisembark <- true	//Tell passengers to disembark.
	}
}

func car(id int, seats int, chs carChannels) {
	for {
		load(id, seats, chs)
		run(id)
		unload(seats, chs)
	}
}

func board(id int, chs carChannels) int {
	carId := <- chs.freeSeatTokens	//Attempt to take a free seat in the car.
	fmt.Printf("(Passenger %d) Boards car %d.\n", id, carId)
	chs.passengerEmbark <- true	//Board the car.
	return carId
}

func unboard(id int, carId int, chs carChannels) {
	<- chs.passengerDisembark		//Disembark from the car.
	fmt.Printf("(Passenger %d) Disembarks from car %d.\n", id, carId)
	chs.freeSeatTokens <- carId		//Leave your seat.
	
}

func passenger(id int, chs carChannels, done chan bool) {
	carId := board(id, chs)
	unboard(id, carId, chs)
	
	done <- true
}

func testScenario(totalPassengers, totalSeats int) {
	carChs := carChannels{freeSeatTokens: make(chan int, totalSeats), passengerEmbark: make(chan bool), passengerDisembark: make(chan bool)}
	done := make(chan bool)
	
	go car(0, totalSeats, carChs)
	for i := 0; i < totalSeats; i++ {
		carChs.freeSeatTokens <- 0
	}
	
	for i := 0; i < totalPassengers; i++ {
		go passenger(i, carChs, done)
	}
	
	for i := 0; i < totalPassengers; i++ {
		<- done
	}
}

func main() {
	rand.Seed(time.Now().UTC().UnixNano())
	
	fmt.Print("Please input how many passenger threads to run: ")
	if passengers, passengersErr := parse.ScanInt(); !passengersErr && passengers >= 0 {
		fmt.Print("Please input how many seats there are in the roller coaster cars: ")
		if seats, seatsErr := parse.ScanInt(); !seatsErr && seats >= 0 && seats < passengers {
			testScenario(passengers, seats)
		}else{
			fmt.Println("Please input a positive integer less than the number of passengers, and nothing else.")
		}
	}else{
		fmt.Println("Please input a single natural number, and nothing else.")
	}
}