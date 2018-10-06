package main

import (
	"fmt"
	"time"
	"sync"
	"math/rand"
	"go/shared/parse"	//Requires that your GOPATH include the directory which contains this repo.
)

func reader(id int, data *int, lock *sync.RWMutex, done chan<- bool) {
	var value int
	
	lock.RLock()
	time.Sleep(time.Duration(rand.Intn(10)) * time.Millisecond)
	value = *data
	lock.RUnlock()
	
	fmt.Printf("(Reader %d) Read %d\n", id, value)
	done <- true
}

func writer(id int, data *int, lock *sync.RWMutex, done chan<- bool) {
	var value int
	
	lock.Lock()
	time.Sleep(time.Duration(rand.Intn(10)) * time.Millisecond)
	value = *data + 1
	*data = value
	lock.Unlock()
	
	fmt.Printf("(Writer %d) Wrote %d\n", id, value)
	done <- true
}

func testScenario(totalReaders, totalWriters int) {
	data := 0
	lock := sync.RWMutex{}
	done := make(chan bool)
	
	for i, j := 0, 0; i < totalReaders || j < totalWriters; {
		if i < totalReaders && j < totalWriters {
			if rand.Intn(1 + totalReaders / (totalWriters + 1)) != 0 {
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
	fmt.Printf("Final value: %d\n", data)
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

//was non-alignment a pragmatic (realpolitik) descision, or was it planned?
//was tito pragmatic or ideologically motivated? (non-alignment and the tito-stalin splits would suggest pragmatic)
	//when did this change? (tito-stalin split?  earlier even?)
	//did this stay the same throughout his life? (yugoslavia did move away from non-alignment later according to a book in the library)