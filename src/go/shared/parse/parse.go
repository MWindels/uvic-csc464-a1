package parse

import "fmt"

func ScanInt() (int, bool) {
	var value int
	if _, err := fmt.Scanf("%d\n", &value); err == nil {
		return value, false
	}
	return 0, true
}