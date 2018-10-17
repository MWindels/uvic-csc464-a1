package main

import (
	"fmt"
	"time"
	"math/rand"
	"go/shared/parse"
)

type hall struct {
	
	//Synchronization Members.
	tryEnter chan bool	//Buffered channel with cap = 1.  Also protects the entered member.
	notifyCheckIn chan bool	//Buffered channel with cap = total number of immigrants.
	swearOath chan bool
	certification chan bool
	tryLeave chan bool	//Buffered channel with cap = 1.
	notifyLeave chan bool	//Buffered channel with cap = total number of immigrants.
	
	//Mutable Members.
	entered int		//The number of immigrants who've entered.
	
}

func initHall(totalImmigrants int) hall {
	hl := hall{
		tryEnter: make(chan bool, 1),
		notifyCheckIn: make(chan bool, totalImmigrants),
		swearOath: make(chan bool),
		certification: make(chan bool),
		tryLeave: make(chan bool, 1),
		notifyLeave: make(chan bool, totalImmigrants),
		entered: 0,
	}
	hl.tryEnter <- true
	hl.tryLeave <- true
	return hl
}



//----------Immigrant Functions----------

func (h *hall) enterImmigrant(id int) {
	//start := time.Now()
	<- h.tryEnter
	//fmt.Printf("%d\n", time.Since(start).Nanoseconds())
	h.entered++
	fmt.Printf("(Immigrant %d) Arrives.\n", id)
	h.tryEnter <- true
}

func (h hall) checkIn(id int) {
	fmt.Printf("(Immigrant %d) Checks in.\n", id)
	h.notifyCheckIn <- true
}

func (h hall) swear(id int) {
	<- h.swearOath
	fmt.Printf("(Immigrant %d) Swears their oath, and gets their certificate.\n", id)
	<- h.certification
}

func (h hall) leaveImmigrant(id int) {
	<- h.tryLeave
	fmt.Printf("(Immigrant %d) Leaves.\n", id)
	h.tryLeave <- true
	
	h.notifyLeave <- true
}



//----------Judge Functions----------

func (h hall) enterJudge(prevImmigrants int) {
	for i := 0; i < prevImmigrants; i++ {
		<- h.notifyLeave
	}
	
	<- h.tryEnter
	<- h.tryLeave
	fmt.Printf("(The Judge) Arrives.\n")
}

func (h hall) confirm() {
	for i := 0; i < h.entered; i++ {
		<- h.notifyCheckIn
	}
	fmt.Printf("(The Judge) Begins the confirmation process.\n")
	
	//start := time.Now()
	for i := 0; i < h.entered; i++ {
		h.swearOath <- true
		h.certification <- true
	}
	//fmt.Printf("%d\n", time.Since(start).Nanoseconds())
}

func (h *hall) leaveJudge() int {
	fmt.Printf("(The Judge) Leaves.\n")
	prevImmigrants := h.entered
	h.entered = 0
	h.tryLeave <- true
	h.tryEnter <- true
	
	return prevImmigrants
}



//----------Spectator Functions----------

func (h hall) enterSpectator(id int) {
	//start := time.Now()
	<- h.tryEnter
	//fmt.Printf("%d\n", time.Since(start).Nanoseconds())
	fmt.Printf("(Spectator %d) Arrives.\n", id)
	h.tryEnter <- true
}

func (h hall) spectate(id int) {
	fmt.Printf("(Spectator %d) Spectates.\n", id)
	time.Sleep(time.Duration(rand.Intn(100)) * time.Millisecond)
}

func (h hall) leaveSpectator(id int) {
	fmt.Printf("(Spectator %d) Leaves.\n", id)
}



//----------Thread Functions----------

func immigrant(id int, fh *hall, done chan bool) {
	defer func(){done <- true}()
	
	time.Sleep(time.Duration(rand.Intn(2000)) * time.Millisecond)	//In transit.
	fh.enterImmigrant(id)
	time.Sleep(time.Duration(rand.Intn(200)) * time.Millisecond)	//Find way to check-in.
	fh.checkIn(id)
	fh.swear(id)
	fh.leaveImmigrant(id)
}

func judge(fh *hall) {
	prevImmigrants := 0
	for {
		time.Sleep(time.Duration(rand.Intn(10)) * time.Millisecond)	//In transit.
		fh.enterJudge(prevImmigrants)
		fh.confirm()
		prevImmigrants = fh.leaveJudge()
	}
}

func spectator(id int, fh *hall, done chan bool) {
	defer func(){done <- true}()
	
	time.Sleep(time.Duration(rand.Intn(2000)) * time.Millisecond)	//In transit.
	fh.enterSpectator(id)
	fh.spectate(id)
	fh.leaveSpectator(id)
}

func testScenario(totalImmigrants, totalSpectators int) {
	fh := initHall(totalImmigrants)
	done := make(chan bool)
	
	go judge(&fh)
	for i, j := 0, 0; i + j < totalImmigrants + totalSpectators; {
		time.Sleep(time.Duration(rand.Intn(10)) * time.Millisecond)
		if i < totalImmigrants && j < totalSpectators {
			if rand.Intn(2) == 0 {
				go immigrant(i, &fh, done)
				i++
			}else{
				go spectator(j, &fh, done)
				j++
			}
		}else if i < totalImmigrants {
			go immigrant(i, &fh, done)
			i++
		}else if j < totalSpectators {
			go spectator(j, &fh, done)
			j++
		}
	}
	
	for i := 0; i < totalImmigrants + totalSpectators; i++ {
		<- done
	}
}

func main() {
	rand.Seed(time.Now().UTC().UnixNano())
	
	fmt.Print("Please input how many immigrant threads to run: ")
	if immigrants, immigrantsErr := parse.ScanInt(); !immigrantsErr && immigrants >= 0 {
		fmt.Print("Please input how many spectator threads to run: ")
		if spectators, spectatorsErr := parse.ScanInt(); !spectatorsErr && spectators >= 0 {
			testScenario(immigrants, spectators)
		}else{
			fmt.Println("Please input a single natural number, and nothing else.")
		}
	}else{
		fmt.Println("Please input a single natural number, and nothing else.")
	}
}