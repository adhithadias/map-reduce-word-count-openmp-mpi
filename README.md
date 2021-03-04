## Running the program

```bash
gcc serial.c -o serial -fopenmp
./serial 2>&1 | tee serial.txt
sort serial.txt > serial_sorted.txt

gcc parallel.c -o parallel -fopenmp
./parallel 2>&1 | tee parallel.txt
sort parallel.txt > parallel_sorted.txt
```