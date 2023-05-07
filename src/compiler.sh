gcc ./kernel/main.c ./our_commons/*.c -l pthread -l readline -l commons
echo "---------------------------------------------"
gcc ./memory/main.c ./our_commons/*.c -l pthread -l readline -l commons
echo "---------------------------------------------"
gcc ./cpu/main.c ./our_commons/*.c -l pthread -l readline -l commons
echo "---------------------------------------------"
gcc ./console/main.c ./our_commons/*.c -l pthread -l readline -l commons
echo "---------------------------------------------"
gcc ./filesystem/main.c ./our_commons/*.c -l pthread -l readline -l commons
echo "---------------------------------------------"

