#Simple make file
#kdupreez@hotmail.com

APP=video-device-io
CXX=g++
SRC_MAIN=src
OUT_DIR=bin
CPP_STD=c++14

#extra include paths
INC1=
INC2=
INC=$(INC1) $(INC2)
INC_PARAMS=$(foreach d, $(INC), -I$d)

#additional sources files
S1=
S2=
S3=
S4=
S5=
S6=
S7=
S8=
S9=
SRC_EXT=$(S1) $(S2) $(S3) $(S4) $(S5) $(S6) $(S7) $(S8) $(S9)

#lib paths
LP1=
LP2=
LIB_PATH=$(LP1) $(LP2)
LIB_PATH_PARAMS=$(foreach d, $(LIB_PATH), -L$d)

#link libs
L1=
L2=
L3=
L4=
	#opencv	
L5=opencv_core
L6=opencv_highgui
L7=opencv_imgproc
L8=opencv_imgcodecs
L9=opencv_calib3d
	#opencv cuda
L10=opencv_cudaimgproc
L11=opencv_cudaarithm
L12=opencv_cudafeatures2d
L13=opencv_cudawarping
L14=opencv_cudafilters

LIBS=$(L1) $(L2) $(L3) $(L4) $(L5) $(L6) $(L7) $(L8) $(L9) $(L10) $(L11) $(L12) $(L13) $(L14)
LIBS_PARAMS=$(foreach d, $(LIBS), -l$d)

#compile
$(APP): $(SRC_MAIN)/$(APP).cpp 
	test -d bin || mkdir -p bin
	$(CXX) -std=$(CPP_STD) $(SRC_MAIN)/$(APP).cpp $(SRC_EXT) -o $(OUT_DIR)/$(APP) $(INC_PARAMS) $(LIB_PATH_PARAMS) $(LIBS_PARAMS) -g -O0

clean:
	rm bin/$(APP)
	
