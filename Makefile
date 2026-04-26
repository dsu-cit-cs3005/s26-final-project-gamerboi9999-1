CXX = g++
CXXFLAGS = -std=c++20 -Wall -g -fPIC
LDFLAGS = -ldl

TARGET = RobotWarz
TESTER = test_robot

OBJS = main.o Arena.o RobotBase.o

all: $(TARGET) $(TESTER)

$(TARGET): $(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)

$(TESTER): test_robot.o RobotBase.o
	$(CXX) -o $@ $^ $(LDFLAGS)

main.o: main.cpp Arena.h RobotBase.h RadarObj.h
Arena.o: Arena.cpp Arena.h RobotBase.h RadarObj.h
RobotBase.o: RobotBase.cpp RobotBase.h RadarObj.h
test_robot.o: test_robot.cpp RobotBase.h RadarObj.h

clean:
	rm -f $(OBJS) $(TARGET) $(TESTER) test_robot.o robots/*.so

.PHONY: all clean
