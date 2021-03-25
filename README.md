## Running the program

```bash
gcc serial.c -o serial -fopenmp -std=gnu99
./serial 2>&1 | tee serial.txt
sort serial.txt > serial_sorted.txt

gcc parallel.c -o parallel -fopenmp -std=gnu99
./parallel 2>&1 | tee parallel.txt
sort parallel.txt > parallel_sorted.txt
```
