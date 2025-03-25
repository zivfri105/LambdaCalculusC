# Lambda Calculus Interpreter in C

A lambda calculus interpreter written in C, featuring:

- Lambda expression parsing
- Beta reduction
- Variable expansion and contraction
- Variable definition and storage
- Interactive REPL using GNU `readline`

## Features

- Parse lambda expressions using standard notation: `(\x. x)`
- Perform beta reductions (`br`)
- Define and store variables (`def`)
- Expand and contract expressions (`ex`, `con`)
- Load definitions from a file (`load`)
- View all currently defined variables (`show`)
- REPL supports line editing and command history

## Example Commands

```text
def id (\x. x)
def tru (\x. (\y. x))
def fls (\x. (\y. y))
br 10 (id tru)
ex 5 tru
con 10 tru
show
load default
```

## Build Instructions

### Dependencies

- GCC
- GNU `readline`
- `make`

To install dependencies on Arch Linux:

```sh
sudo pacman -S base-devel readline
```

Written in C using readline, argp, and custom data structures for tokenization and evaluation of lambda calculus expressions.