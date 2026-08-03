package main

import (
	"bufio"
	"fmt"
	"os"
	"strconv"
	"strings"
)

//Locates and replaces the first occurrence of a string in the heap
//of a process

//Usage: ./read_write_heap PID search_string replace_by_string
//Where:
//- PID is the pid of the target process
//- search_string is the ASCII string you are looking to overwrite
//- replace_by_string is the ASCII string you want to replace
//  search_string with

func usage() {
	fmt.Println("Usage ./read_write_heap pid search_string replace_string run with sudo")
}

func main() {

	pid, err := strconv.Atoi(os.Args[1])
	search_string := os.Args[2]
	replace_string := os.Args[3]

	if err != nil || pid <= 0 {
		usage()
		return
	}

	if len(os.Args) != 4 {
		usage()
		return
	}

	if search_string == "" {
		usage()
		return
	}

	maps_filename := fmt.Sprintf("%s%s%s", "/proc/", pid, "/maps")
	fmt.Printf("[+] maps : %s", maps_filename)
	mem_filename := fmt.Sprintf("%s%s%s", "/proc/", pid, "/mem")
	fmt.Printf("[+] maps : %s", mem_filename)

	file, err := os.Open(maps_filename)
	if err != nil {
		fmt.Printf("Error when opening a file %v", err)
		return
	}
	defer file.Close()

	scanner := bufio.NewScanner(file)

	for scanner.Scan() {
		words := strings.Fields(scanner.Text())

		if words[len(words)-1] == "[heap]" {
			addr := words[0]
			perm := words[1]
			//offset := words[2]
			//device := words[3]
			//inode := words[4]
			//pathname := words[5]

			if perm[0] != 'r' || perm[1] != 'w' {
				fmt.Println("Wrong permissions")
				return
			}

			addr_split := strings.Split(addr, "-")
			if len(addr_split) != 2 {
				fmt.Println("wrong number of addresses")
				return
			}

			addr_start, err := strconv.ParseInt(addr_split[0], 16, 64)
			addr_end, err := strconv.ParseInt(addr_split[1], 16, 64)

			mem_file, err := os.OpenFile(mem_filename, os.O_RDWR, 0644)
			if err != nil {
				fmt.Printf("Error opening mem file %v\n", err)
				return
			}
			defer mem_file.Close()

			size := addr_end - addr_start
			heap := make([]byte, size)

			_, err = mem_file.ReadAt(heap, addr_start)
			if err != nil {
				fmt.Printf("Error reading mem file %v\n", err)
				return
			}

		}

	}

}
