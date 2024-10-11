package main

import (
	"bufio"
	"fmt"
	"os"
	"strconv"
	"strings"
	"unicode"
)

func main() {
	if len(os.Args) != 3 {
		fmt.Fprintf(
			os.Stderr,
			"Usage: %v enc|dec <value: 0-255>\n", os.Args[0],
		)
		os.Exit(1)
	}

	operation := ""

	switch op := os.Args[1]; op {
	case "enc", "dec":
		operation = op
	default:
		fmt.Fprintln(os.Stderr, "Invalid operation:", op)
		os.Exit(1)

	}

	key, err := strconv.ParseUint(os.Args[2], 10, 8)
	if err != nil || !(0 <= key && key <= 255) {
		fmt.Fprintln(os.Stderr, "Invalid number")
		os.Exit(1)
	}

	scn := bufio.NewScanner(os.Stdin)
	if !scn.Scan() && scn.Err() != nil {
		fmt.Fprintln(os.Stderr, "Error reading input")
		os.Exit(1)
	}

	if operation == "enc" {
		// Encode the space trimmed string
		res := xorEncode([]byte(strings.Trim(scn.Text(), " \n\t")), byte(key))
		fmt.Printf("[ENC %v] => %v\n", key, res)
	} else {
		res := xorDecode(scn.Text(), byte(key))
		if res == nil {
			fmt.Fprintln(os.Stderr, "Decoding failed, invalid input.")
			os.Exit(1)
		}
		fmt.Printf("[DEC %v] => %v\n", key, string(res))
	}
}

// Encode the message in hex format.
func xorEncode(message []byte, key byte) string {
	var ret strings.Builder
	ret.Grow(2 * len(message))

	for _, b := range message {
		enc := b ^ key
		ret.WriteString(fmt.Sprintf("%x", enc))
	}

	return ret.String()
}

// Decode the hex encoded message.
func xorDecode(message string, key byte) []byte {
	ret := make([]byte, 0, len(message)/2+1)
	digits := make([]byte, 0, 2)

	hex_decode := func() {
		dec := key ^ (digits[0]*16 + digits[1])
		ret = append(ret, dec)
		digits = digits[:0]
	}

	for _, c := range message {
		if unicode.IsSpace(c) {
			continue
		}

		c := unicode.ToLower(c)
		var b byte

		switch {
		case '0' <= c && c <= '9':
			b = byte(c) - '0'
		case 'a' <= c && c <= 'f':
			b = 10 + byte(c) - 'a'

		default:
			return nil

		}

		switch len(digits) {
		case 0, 1:
			digits = append(digits, b)
		case 2:
			hex_decode()
			digits = append(digits, b)
		}
	}

	if len(digits) == 2 {
		hex_decode()
	}

	return ret

}
