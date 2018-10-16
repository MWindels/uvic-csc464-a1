package main

import (
	"fmt"
	"time"
	"sync"
	"math/rand"
	"go/shared/parse"
)

type cart struct {
	
	//Synchronization Members.
	enterCar chan bool
	leaveCar chan bool
	notifyEnter chan bool
	notifyExit chan bool
	unloadReady chan bool	//Buffered with cap = 1.
	
	//Const Members.
	id int
	capacity int
	
}

type park struct {
	
	//Synchronization Members.
	hasCarReady chan cart	//Buffered with cap = max capacity of all cars.
	waitingCars chan cart	//Buffered with cap = total number of cars.
	runningCars chan cart	//Buffered with cap = total number of cars.
	
	//Mutable Members.
	carLock sync.Mutex
	loadingCar cart
	loadingExists bool
	unloadingCar cart
	unloadingExists bool
	
}



//----------Cart Functions----------

func initCart(i, c int) cart {
	ct := cart{
		enterCar: make(chan bool),
		leaveCar: make(chan bool),
		notifyEnter: make(chan bool),
		notifyExit: make(chan bool),
		unloadReady: make(chan bool, 1),
		id: i,
		capacity: c,
	}
	return ct
}

func (c cart) load() {
	for i := 0; i < c.capacity; i++ {
		<- c.enterCar
	}
	for i := 0; i < c.capacity; i++ {
		c.notifyEnter <- true
	}
}

func (c cart) run() {
	fmt.Printf("(Car %d) Now running...\n", c.id)
	time.Sleep(time.Duration(rand.Intn(10)) * time.Millisecond)
	<- c.unloadReady
	fmt.Printf("(Car %d) Finished.\n", c.id)
}

func (c cart) unload() {
	for i := 0; i < c.capacity; i++ {
		c.notifyExit <- true
	}
	for i := 0; i < c.capacity; i++ {
		<- c.leaveCar
	}
}

func (c cart) board(passId int) {
	c.enterCar <- true
	fmt.Printf("(Passenger %d) Boards car %d.\n", passId, c.id)
}

func (c cart) ride() {
	<- c.notifyEnter	//Get in to the car.
	<- c.notifyExit		//Get out of the car.
}

func (c cart) unboard(passId int) {
	fmt.Printf("(Passenger %d) Disembarks from car %d.\n", passId, c.id)
	c.leaveCar <- true
}



//----------Park Functions----------

func initPark(pk *park, cars []cart) {
	maxCap := 0
	for _, car := range cars {
		if maxCap < car.capacity {
			maxCap = car.capacity
		}
	}
	
	*pk = park{
		hasCarReady: make(chan cart, maxCap),
		waitingCars: make(chan cart, len(cars)),
		runningCars: make(chan cart, len(cars)),
		carLock: sync.Mutex{},
		loadingCar: cart{},
		loadingExists: false,
		unloadingCar: cart{},
		unloadingExists: false,
	}
	if len(cars) > 0 {
		pk.loadingCar = cars[0]
		pk.loadingExists = true
		for i := 0; i < pk.loadingCar.capacity; i++ {
			pk.hasCarReady <- pk.loadingCar
		}
		for _, car := range cars[1:] {
			pk.waitingCars <- car
		}
	}
}

func (p *park) startCar() {
	p.carLock.Lock()
	defer p.carLock.Unlock()
	
	if p.loadingExists {
		if !p.unloadingExists {
			p.unloadingCar = p.loadingCar
			p.unloadingExists = true
			p.unloadingCar.unloadReady <- true
		}else{
			p.runningCars <- p.loadingCar
		}
		
		select{
		case p.loadingCar = <- p.waitingCars:
			p.loadingExists = true
			for i := 0; i < p.loadingCar.capacity; i++ {
				p.hasCarReady <- p.loadingCar
			}
		default:
			p.loadingExists = false
		}
	}
}

func (p *park) returnCar() {
	p.carLock.Lock()
	defer p.carLock.Unlock()
	
	if p.unloadingExists {
		if !p.loadingExists {
			p.loadingCar = p.unloadingCar
			p.loadingExists = true
			for i := 0; i < p.loadingCar.capacity; i++ {
				p.hasCarReady <- p.loadingCar
			}
		}else{
			p.waitingCars <- p.unloadingCar
		}
		
		select{
		case p.unloadingCar = <- p.runningCars:
			p.unloadingExists = true
			p.unloadingCar.unloadReady <- true
		default:
			p.unloadingExists = false
		}
	}
}

func (p *park) queueForCar() cart {
	return <- p.hasCarReady
}



//----------Thread Functions----------

func car(me cart, thePark *park) {
	for {
		me.load()
		thePark.startCar()
		me.run()
		me.unload()
		thePark.returnCar()
	}
}

func passenger(id int, thePark *park, done chan<- bool) {
	ride := thePark.queueForCar()
	ride.board(id)
	ride.ride()
	ride.unboard(id)
	
	done <- true
}

func testScenario(totalPassengers, totalCars, totalSeats int) {
	var thePark park
	var theCars []cart
	for i := 0; i < totalCars; i++ {
		theCars = append(theCars, initCart(i, totalSeats))
	}
	done := make(chan bool)
	initPark(&thePark, theCars)
	
	//start := time.Now()
	
	for i := 0; i < totalCars; i++ {
		go car(theCars[i], &thePark)
	}
	for i := 0; i < totalPassengers; i++ {
		go passenger(i, &thePark, done)
	}
	
	for i := 0; i < totalPassengers; i++ {
		<- done
	}
	
	//fmt.Printf("%d\n", time.Since(start).Nanoseconds())
}

func main() {
	rand.Seed(time.Now().UTC().UnixNano())
	
	fmt.Print("Please input how many passenger threads to run: ")
	if passengers, passengersErr := parse.ScanInt(); !passengersErr && passengers >= 0 {
		fmt.Print("Please input how many roller coaster car threads to run: ")
		if cars, carsErr := parse.ScanInt(); !carsErr && cars >= 0 {
			fmt.Print("Please input how many seats there are in the roller coaster cars: ")
			if seats, seatsErr := parse.ScanInt(); !seatsErr && seats >= 0 && seats <= passengers {
				testScenario(passengers, cars, seats)
			}else{
				fmt.Println("Please input a positive integer less than of equal to the number of passengers, and nothing else.")
			}
		}else{
			fmt.Println("Please input a single natural number, and nothing else.")
		}
	}else{
		fmt.Println("Please input a single natural number, and nothing else.")
	}
}