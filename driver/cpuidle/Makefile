obj-m += st_module.o
st_module-objs := main.o ../src/st_lib.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -f ../src/.st_lib.o.cmd
	rm -f ../src/*.o
