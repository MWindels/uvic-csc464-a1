package main

import (
	"fmt"
	"time"
	"sync"
	"math/rand"
	"go/shared/parse"	//Requires that your GOPATH include the directory which contains this repo.
)

func reader(id int, data *int, lock *sync.RWMutex, done chan<- bool) {
	//start := time.Now()
	lock.RLock()
	//totalTime := time.Since(start)
	
	fmt.Printf("(Reader %d) Begins reading...\n", id)
	time.Sleep(time.Duration(rand.Intn(10)) * time.Millisecond)
	value := *data
	value++
	fmt.Printf("(Reader %d) Read %d.\n", id, *data)
	
	lock.RUnlock()
	
	//fmt.Printf("%d\n", totalTime.Nanoseconds())
	
	done <- true
}

func writer(id int, data *int, lock *sync.RWMutex, done chan<- bool) {
	//start := time.Now()
	lock.Lock()
	//totalTime := time.Since(start)
	
	fmt.Printf("(Writer %d) Begins writing...\n", id)
	time.Sleep(time.Duration(rand.Intn(10)) * time.Millisecond)
	*data = *data + 1
	fmt.Printf("(Writer %d) Wrote %d.\n", id, *data)
	
	lock.Unlock()
	
	//fmt.Printf("%d\n", totalTime.Nanoseconds())
	
	done <- true
}

func testScenario(totalReaders, totalWriters int) {
	data := 0
	lock := sync.RWMutex{}
	done := make(chan bool)
	
	for i, j := 0, 0; i < totalReaders || j < totalWriters; {
		time.Sleep(time.Duration(rand.Intn(5)) * time.Millisecond)
		if i < totalReaders && j < totalWriters {
			if rand.Intn(2) == 0 {
				go reader(i, &data, &lock, done)
				i++
			}else{
				go writer(j, &data, &lock, done)
				j++
			}
		}else if i < totalReaders {
			go reader(i, &data, &lock, done)
			i++
		}else if j < totalWriters {
			go writer(j, &data, &lock, done)
			j++
		}
	}
	
	for i := 0; i < totalReaders + totalWriters; i++ {
		<- done
	}
	//fmt.Printf("Final value: %d\n", data)
}

func main() {
	rand.Seed(time.Now().UTC().UnixNano())
	
	fmt.Print("Please input how many reader threads to run: ")
	if readers, readersErr := parse.ScanInt(); !readersErr && readers >= 0 {
		fmt.Print("Please input how many writer threads to run: ")
		if writers, writersErr := parse.ScanInt(); !writersErr && writers >= 0 {
			testScenario(readers, writers)
		}else{
			fmt.Println("Please input a single natural number, and nothing else.")
		}
	}else{
		fmt.Println("Please input a single natural number, and nothing else.")
	}
}