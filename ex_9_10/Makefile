MODULE = processing_acc

SRC_PATH_COMMON = ../npu_common
SRCS_COMMON = $(SRC_PATH_COMMON)/DmaChannel.cpp $(SRC_PATH_COMMON)/EthernetLink.cpp $(SRC_PATH_COMMON)/IoModule.cpp $(SRC_PATH_COMMON)/IpPacket.cpp $(SRC_PATH_COMMON)/memory.cpp $(SRC_PATH_COMMON)/MemoryManager.cpp $(SRC_PATH_COMMON)/PcapImporter.cpp $(SRC_PATH_COMMON)/RAM.cpp $(SRC_PATH_COMMON)/SimpleBusAT.cpp $(SRC_PATH_COMMON)/report.cpp $(SRC_PATH_COMMON)/globals.cpp $(SRC_PATH_COMMON)/RoutingTable.cpp $(SRC_PATH_COMMON)/Cpu_proc.cpp $(SRC_PATH_COMMON)/argvparser.cpp

SRCS_LOCAL = Cpu.cpp main.cpp Accelerator.cpp

SRCS =$(SRCS_COMMON) $(SRCS_LOCAL)

EXTRA_LIBS = -lpcap

include ../npu_common/build.mk
