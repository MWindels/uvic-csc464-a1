package main

import (
	"fmt"
	"time"
	"sync"
	"math/rand"
	"container/list"
	"go/shared/parse"
)

type container struct {
	
	//Synchronization Members.
	deleteLock sync.RWMutex	//Used to exclude deletions from each other, and every other operation, while allowing the other operations to proceed in parallel.
	insertLock sync.Mutex	//Used to exclude more than one inserter from inserting at the same time.
	sizeLock sync.Mutex		//Used to determine the size of the list without torn reads.
	
	//Mutable Member.
	ctnr *list.List
	
}

func (c *container) find(x int) *(list.Element) {
	c.sizeLock.Lock()
	size := c.ctnr.Len()
	c.sizeLock.Unlock()
	
	for i, item := 0, c.ctnr.Front(); i < size; i++ {
		if item.Value == x {
			return item
		}
		item = item.Next()
	}
	return nil
}

func searcher(id int, c *container, done chan<- bool) {
	defer func() {done <- true}()
	
	time.Sleep(time.Duration(rand.Intn(1)) * time.Millisecond)
	//start := time.Now()
	
	c.deleteLock.RLock()
	defer c.deleteLock.RUnlock()
	
	if c.find(id) != nil {
		fmt.Printf("(Searcher %d) Found element {%d}.\n", id, id)
	}else{
		fmt.Printf("(Searcher %d) Did not find element {%d}!\n", id, id)
	}
	
	//fmt.Printf("%d\n", time.Since(start).Nanoseconds())
}

func inserter(id int, c *container, done chan<- bool) {
	defer func() {done <- true}()
	
	time.Sleep(time.Duration(rand.Intn(1)) * time.Millisecond)
	//start := time.Now()
	
	c.deleteLock.RLock()
	defer c.deleteLock.RUnlock()
	
	c.insertLock.Lock()
	defer c.insertLock.Unlock()
	
	func (){
		c.sizeLock.Lock()
		defer c.sizeLock.Unlock()
		
		c.ctnr.PushBack(id)
	}()
	
	//fmt.Printf("%d\n", time.Since(start).Nanoseconds())
	
	fmt.Printf("(Inserter %d) Added element {%d}.\n", id, id)
}

func deleter(id int, c *container, done chan<- bool) {
	defer func() {done <- true}()
	
	time.Sleep(time.Duration(rand.Intn(1)) * time.Millisecond)
	//start := time.Now()
	
	c.deleteLock.Lock()
	defer c.deleteLock.Unlock()
	
	if elem := c.find(id); elem != nil {
		c.ctnr.Remove(elem)
		fmt.Printf("(Deleter %d) Removed element {%d}.\n", id, id)
	}else{
		fmt.Printf("(Deleter %d) Did not find element {%d}!\n", id, id)
	}
	
	//fmt.Printf("%d\n", time.Since(start).Nanoseconds())
}

func testScenario(totalSearchers, totalInserters, totalDeleters int) {
	theContainer := container{}
	theContainer.ctnr = list.New()
	done := make(chan bool)
	
	for i, j, k := 0, 0, 0; i + j + k < totalSearchers + totalInserters + totalDeleters; {
		//time.Sleep(time.Duration(rand.Intn(5)) * time.Millisecond)
		time.Sleep(time.Duration(rand.Intn(1)) * time.Millisecond)
		if i < totalSearchers && j < totalInserters && k < totalDeleters {
			if rand.Intn(3) == 0 {
				go searcher(i, &theContainer, done)
				i++
			}else{
				if rand.Intn(2) == 0 {
					go inserter(j, &theContainer, done)
					j++
				}else{
					go deleter(k, &theContainer, done)
					k++
				}
			}
		}else if i < totalSearchers && j < totalInserters {
			if rand.Intn(2) == 0 {
				go searcher(i, &theContainer, done)
				i++
			}else{
				go inserter(j, &theContainer, done)
				j++
			}
		}else if j < totalInserters && k < totalDeleters {
			if rand.Intn(2) == 0 {
				go inserter(j, &theContainer, done)
				j++
			}else{
				go deleter(k, &theContainer, done)
				k++
			}
		}else if i < totalSearchers && k < totalDeleters {
			if rand.Intn(2) == 0 {
				go searcher(i, &theContainer, done)
				i++
			}else{
				go deleter(k, &theContainer, done)
				k++
			}
		}else if i < totalSearchers {
			go searcher(i, &theContainer, done)
			i++
		}else if j < totalInserters {
			go inserter(j, &theContainer, done)
			j++
		}else if k < totalDeleters {
			go deleter(k, &theContainer, done)
			k++
		}
	}
	
	for i := 0; i < totalSearchers + totalInserters + totalDeleters; i++ {
		<- done
	}
}

func main() {
	rand.Seed(time.Now().UTC().UnixNano())
	
	fmt.Print("Please input how many searcher threads to run: ")
	if searchers, searchersErr := parse.ScanInt(); !searchersErr && searchers >= 0 {
		fmt.Print("Please input how many inserter threads to run: ")
		if inserters, insertersErr := parse.ScanInt(); !insertersErr && inserters >= 0 {
			fmt.Print("Please input how many deleter threads to run: ")
			if deleters, deletersErr := parse.ScanInt(); !deletersErr && deleters >= 0 {
				testScenario(searchers, inserters, deleters)
			}else{
				fmt.Println("Please input a single natural number, and nothing else.")
			}
		}else{
			fmt.Println("Please input a single natural number, and nothing else.")
		}
	}else{
		fmt.Println("Please input a single natural number, and nothing else.")
	}
}