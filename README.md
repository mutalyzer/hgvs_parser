# HGVS variant description parser in C

## Building

```
make
```

Alternatively, for POSIX systems (assumes `unistd.h` to be present) using
an ANSI-compatible terminal ouput will be colored when printing to a
`tty`:

```
make OPTIONS='ANSI'
```

## Testing

To run the tests (assumes valgrind to be present):

```
make check
```

## Use

Run:

```
./a.out 'NG_012232.1(NM_004006.1):c.183_186+48del'
```
