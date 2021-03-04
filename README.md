## Running the program

```bash
gcc serial.c -o serial -fopenmp
<<<<<<< HEAD
./serial 2>&1 | tee serial.txt
sort serial.txt > serial_sorted.txt

gcc parallel.c -o parallel -fopenmp
./parallel 2>&1 | tee parallel.txt
sort parallel.txt > parallel_sorted.txt
=======
./entry
>>>>>>> efdc11fe9964de1ff44605c78dea8c132fddbc25
```