## About
This program prints a SHA-256 hash, as specified in [RFC 6234](https://datatracker.ietf.org/doc/html/rfc6234) (SHA 2), of stdin to stdout. It takes as an optional argument a hex encoding of a message digest against which to compare the hash. If the argument is provided and the argument matches the generated hash, the program prints `OK` and returns a 0 exit code. If the argument is provided and the argument does not match the generated hash, the program prints `BAD` and returns a nonzero exit code.

I wrote this to learn more about the SHA 2 algorithm and to improve the ergonomics of the `sha256sum` utility. It should go without saying, but just to be extra clear: do not use this program if security matters at all to you. 

## Usage
To generate the hash of a file, redirect the file to stdin.
```bash
$ ./a.out < main.c
263944a6441c8e6c53195ea1cdc7d62a52c76fb917de3d69b66496cd189dc4b5
```
To check the hash of a file against a known hash, pass the known hash as an argument.
```bash
$ ./a.out 263944a6441c8e6c53195ea1cdc7d62a52c76fb917de3d69b66496cd189dc4b5 < main.c
263944a6441c8e6c53195ea1cdc7d62a52c76fb917de3d69b66496cd189dc4b5 OK
```