CXX=g++
CXXFLAGS=-pthread -Wall -Wno-switch -Wpedantic -std=c++11 -O3
ROOT_DIR=../../../source/cppclient
BASE_SRC_DIR=${ROOT_DIR}/client
INCLUDES=-I${BASE_SRC_DIR} -I${ROOT_DIR} -I/usr/include/eigen3 -I/usr/include/boost
SHARED_LIB_DIRS=-L${BASE_SRC_DIR}
SHARD_LIBS=-lTwsSocketClient
TARGET=emini_trader

$(TARGET)Static:
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(BASE_SRC_DIR)/*.cpp ./*.cpp -o$(TARGET) $(LDFLAGS)

$(TARGET):
	$(CXX) $(CXXFLAGS) $(INCLUDES) ./*.cpp -o$(TARGET) $(SHARED_LIB_DIRS) $(SHARD_LIBS) $(LDFLAGS) 

clean:
	rm -f $(TARGET) *.o

