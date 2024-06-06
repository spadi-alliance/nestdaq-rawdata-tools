TARGET1 = dlm_flag_checker
TARGET2 = hbf_num_checker
TARGET3 = rawdata_dumper
TARGET4 = show_file_info
TARGET5 = scaler_checker

OBJECT1 = $(TARGET1).o
OBJECT2 = $(TARGET2).o
OBJECT3 = $(TARGET3).o
OBJECT4 = $(TARGET4).o
OBJECT5 = $(TARGET5).o

.SUFFIXES: .cc .o

all:
	make $(TARGET1)
	make $(TARGET2)
	make $(TARGET3)
	make $(TARGET4)
	make $(TARGET5)

$(TARGET1): $(OBJECT1)
	g++ -std=c++17 -Werror -Wall -O2 $^ -o $@ 
$(TARGET2): $(OBJECT2)
	g++ -std=c++17 -Werror -Wall -O2 $^ -o $@ 
$(TARGET3): $(OBJECT3)
	g++ -std=c++17 -Werror -Wall -O2 $^ -o $@ 
$(TARGET4): $(OBJECT4)
	g++ -std=c++17 -Werror -Wall -O2 $^ -o $@ 
$(TARGET5): $(OBJECT5)
	g++ -std=c++17 -Werror -Wall -O2 $^ -o $@ 

clean:
	rm -f $(TARGET1) $(TARGET2) $(TARGET3) $(TARGET4) $(TARGET5) *.o *~ 
