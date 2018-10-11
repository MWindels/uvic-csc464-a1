package main

import (
	"fmt"
	"time"
	"sync"
	"math/rand"
	"container/list"
	"go/shared/parse"
)

func find(x int, ctnr *list.List) *(list.Element) {
	for i := ctnr.Front(); i != nil; i = i.Next() {
		if i.Value == x {
			return i
		}
	}
	return nil
}

func searcher(id int, ctnr *list.List, deleteLock *sync.RWMutex, done chan<- bool) {
	defer func() {done <- true}()
	
	deleteLock.RLock()
	defer deleteLock.RUnlock()
	
	if find(id, ctnr) != nil {
		fmt.Printf("(Searcher %d) Found element {%d}.\n", id, id)
	}else{
		fmt.Printf("(Searcher %d) Did not find element {%d}!\n", id, id)
	}
}

func inserter(id int, ctnr *list.List, deleteLock *sync.RWMutex, insertLock *sync.Mutex, done chan<- bool) {
	defer func() {done <- true}()
	
	deleteLock.RLock()
	defer deleteLock.RUnlock()
	
	insertLock.Lock()
	defer insertLock.Unlock()
	
	ctnr.PushBack(id)
	fmt.Printf("(Inserter %d) Added element {%d}.\n", id, id)
}

func deleter(id int, ctnr *list.List, deleteLock *sync.RWMutex, done chan<- bool) {
	defer func() {done <- true}()
	
	deleteLock.Lock()
	defer deleteLock.Unlock()
	
	if elem := find(id, ctnr); elem != nil {
		ctnr.Remove(elem)
		fmt.Printf("(Deleter %d) Removed element {%d}.\n", id, id)
	}else{
		fmt.Printf("(Deleter %d) Did not find element {%d}!\n", id, id)
	}
}

func testScenario(totalSearchers, totalInserters, totalDeleters int) {
	ctnr := list.New()
	deleteLock := sync.RWMutex{}	//Used to exclude deletions from each other, and every other operation, while allowing the other operations to proceed in parallel.
	insertLock := sync.Mutex{}		//Used to exclude more than one inserter from inserting at the same time.
	done := make(chan bool)
	
	for i, j, k := 0, 0, 0; i + j + k < totalSearchers + totalInserters + totalDeleters; {
		time.Sleep(time.Duration(rand.Intn(100)) * time.Millisecond)
		if i < totalSearchers && j < totalInserters && k < totalDeleters {
			if rand.Intn(3) == 0 {
				go searcher(i, ctnr, &deleteLock, done)
				i++
			}else{
				if rand.Intn(2) == 0 {
					go inserter(j, ctnr, &deleteLock, &insertLock, done)
					j++
				}else{
					go deleter(k, ctnr, &deleteLock, done)
					k++
				}
			}
		}else if i < totalSearchers && j < totalInserters {
			if rand.Intn(2) == 0 {
				go searcher(i, ctnr, &deleteLock, done)
				i++
			}else{
				go inserter(j, ctnr, &deleteLock, &insertLock, done)
				j++
			}
		}else if j < totalInserters && k < totalDeleters {
			if rand.Intn(2) == 0 {
				go inserter(j, ctnr, &deleteLock, &insertLock, done)
				j++
			}else{
				go deleter(k, ctnr, &deleteLock, done)
				k++
			}
		}else if i < totalSearchers && k < totalDeleters {
			if rand.Intn(2) == 0 {
				go searcher(i, ctnr, &deleteLock, done)
				i++
			}else{
				go deleter(k, ctnr, &deleteLock, done)
				k++
			}
		}else if i < totalSearchers {
			go searcher(i, ctnr, &deleteLock, done)
			i++
		}else if j < totalInserters {
			go inserter(j, ctnr, &deleteLock, &insertLock, done)
			j++
		}else if k < totalDeleters {
			go deleter(k, ctnr, &deleteLock, done)
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