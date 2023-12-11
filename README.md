# Lama interpreter and static bytecode analyzer

## Iterative bytecode interpreter and static analyze of the [Lama](https://github.com/PLTools/Lama.git) language

Implemented by Miroshnikov Vladislav

## Build 
In the project root directory execute:
```bash
make 
```

## Run
In the project root directory run compile version:
```bash
./lama-vm interpret <path_to_bc_file>
```

Example for lama bubble-sort:
```bash
./lama-vm interpret Sort.bc
```

To generate lama bytecode execute:
```bash
lamac -b <path_to_lama_file>
```

Example for lama bubble-sort:
```bash
lamac -b Sort.lama
```

## Performance comparison

* 22.4s - Lama recursive interpreter
* 12.7s - Iterative bytecode interpreter

Example of time measurement for iterative bytecode interpreter:
```bash
time ./lama-vm interpret Sort.bc
```

Example of time measurement for Lama recursive interpreter (need to pass empty stdin file):
```bash
touch empty
time lamac -i Sort.lama < empty
```