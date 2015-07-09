#
# Platform/Configuration independent makefile for building 64 bit AuthModule library
#

SrcDir := .
OutputDir := ./sampleAuth64
ObjDir := $(OutputDir)
IncludeDir := $(SrcDir)/../include/

AuthModuleLib := ${OutputDir}/AuthModule.so
AuthModuleSrcs := ${SrcDir}/AuthModule.cpp 
AuthModuleObjs := $(patsubst %.cpp,$(ObjDir)/%.o,$(notdir $(AuthModuleSrcs)))

SHARED := -shared
AuthModule_CC = $(CC)
AuthModule_CXX := $(CXX)
AuthModule_LDFLAGS := $(LDFLAGS) $(DBGFLAGS)
AuthModule_CXXFLAGS := $(CXXFLAGS) $(DBGFLAGS) -m64 -fPIC -DUNIX -D_REENTRANT

AuthModule_CXXFLAGS := $(AuthModule_CXXFLAGS) \
                -I$(SrcDir) \
		-I$(IncludeDir) 
		
-include $(AuthModuleObjs:.o=.d)

$(OutputDir)/%.o: $(SrcDir)/%.cpp 
	mkdir -p $(ObjDir)
	$(AuthModule_CXX) $(AuthModule_CXXFLAGS) $< -c -o $@

.PHONY: AuthModule AuthModule_clean

AuthModule: $(AuthModuleLib)

AuthModule_clean:
	-rm -rf $(AuthModuleLib) $(AuthModuleObjDir) 

$(AuthModuleLib): $(AuthModuleObjs)
	$(AuthModule_CXX) $(SHARED) -o $(AuthModuleLib) $(AuthModuleObjs)
