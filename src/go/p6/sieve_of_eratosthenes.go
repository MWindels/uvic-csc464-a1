package main

import (
	"fmt"
	"time"
	"go/shared/parse"
)

func generate(n int, output chan<- int) {
	for i := 2; i <= n; i++ {
		output <- i
	}
	close(output)
}

func sieve(input <-chan int, primes chan<- int) {
	prime := <- input
	primes <- prime
	
	output := make(chan int)
	
	hasNext := false
	for value := range input {
		if value % prime != 0 {
			if !hasNext {
				go sieve(output, primes)
				hasNext = true
			}
			output <- value
		}
	}
	
	if hasNext {
		close(output)
	}else{
		close(primes)
	}
}

func findPrimesUpTo(n int) []int {
	primes := make([]int, 0)
	primesCh := make(chan int)
	
	generateCh := make(chan int)
	go generate(n, generateCh)
	go sieve(generateCh, primesCh)
	
	for prime := range primesCh {
		primes = append(primes, prime)
	}
	
	return primes
}

func testScenario(n int) {
	//start := time.Now()
	primes := findPrimesUpTo(n)
	//fmt.Printf("%d\n", time.Since(start).Nanoseconds())
	
	fmt.Printf("The prime numbers from two to %d:\n", n)
	for i, prime := range primes {
		if i < len(primes) - 1 {
			fmt.Printf("%d, ", prime)
		}else{
			fmt.Printf("%d\n", prime)
		}
	}
}

func main() {
	fmt.Print("Please input which number to print the primes up to: ")
	if max, maxErr := parse.ScanInt(); !maxErr && max >= 2 {
		testScenario(max)
	}else{
		fmt.Println("Please input a single integer larger than or equal to two, and nothing else.")
	}
}