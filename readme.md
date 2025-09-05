## About
This program prints a SHA-256 hash, as specified in [RFC 6234](https://datatracker.ietf.org/doc/html/rfc6234) (SHA 2), of stdin to stdout. It takes as an optional argument a hex encoding of a message digest against which to compare the hash. If the argument is provided and the argument matches the generated hash, the program prints `OK` and returns a 0 exit code. If the argument is provided and the argument does not match the generated hash, the program prints `BAD` and returns a nonzero exit code.

I wrote this to learn more about the SHA 2 algorithm and to improve the ergonomics of the `sha256sum` utility. It should go without saying, but just to be extra clear: do not use this program if security matters at all to you. 

## Usage
To generate the hash of a file, redirect the file to stdin.
```bash
$ ./a.out < main.c
ac2a06ceefbdc16a783c62e8d7cbfa65eebb62cbb6db91c70dcdc2bb5e8dfd48
```
To check the hash of a file against a known hash, pass the known hash as an argument.
```bash
$ ./a.out ac2a06ceefbdc16a783c62e8d7cbfa65eebb62cbb6db91c70dcdc2bb5e8dfd48 < main.c
ac2a06ceefbdc16a783c62e8d7cbfa65eebb62cbb6db91c70dcdc2bb5e8dfd48 OK
```