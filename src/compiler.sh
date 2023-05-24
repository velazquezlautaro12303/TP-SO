rm ./kernel/a.out ./cpu/a.out ./console/a.out ./memory/a.out ./filesystem/a.out ./log.log

> log.log

gcc ./kernel/main.c ./our_commons/*.c -l pthread -l readline -l commons -o ./kernel/a.out
echo "---------------------------------------------"
gcc ./memory/main.c ./our_commons/*.c -l pthread -l readline -l commons -o ./memory/a.out
echo "---------------------------------------------"
gcc ./cpu/main.c ./our_commons/*.c -l pthread -l readline -l commons -o ./cpu/a.out
echo "---------------------------------------------"
gcc ./console/main.c ./our_commons/*.c -l pthread -l readline -l commons -o ./console/a.out
echo "---------------------------------------------"
gcc ./filesystem/main.c ./our_commons/*.c -l pthread -l readline -l commons -o ./filesystem/a.out
echo "---------------------------------------------"

