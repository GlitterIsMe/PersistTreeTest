CXX = g++
LDFLAGS = -pthread -lpmemobj -lpmem
DEFS = -DCZL_DEBUG -DEVERY_1G_PRINT -DDRAM_CANCEL_LOG -DCAL_ACCESS_COUNT
CXXFLAGS += $(DEFS)
AR = ar
ARFLAGS = rs
LIB_SOURCES = random.cc\
			persistent_skiplist_no_transaction.cc\
			persistent_skip_list.cc\
			log_batch.cc\
			log_batch_wrapper.cc\
			flush_nvm.cc\
			persistent_skiplist_wrapper.cc\

TEST_TARGET = motivation1_main\

LIBOBJECTS = $(LIB_SOURCES:.cc=.o)

TARGET_OBJS = $(LIB_SOURCES:.cc=)

LIBRARY = my_skip_list.a

.PHONY: clean
default:  all

all:  clean test

clean: 
	rm -f $(LIBRARY)
	rm -f $(LIBOBJECTS)
	rm -f $(TEST_TARGET)
	rm -f test

$(LIBOBJECTS): 
	for sou_file in $(TARGET_OBJS) ; do \
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -c $$sou_file.cc -o $$sou_file.o; \
	done;

$(LIBRARY) : $(LIBOBJECTS)
	rm -f $@
	$(AR) $(ARFLAGS) $@ $^

$(TEST_TARGET) :
	for test_file in $(TEST_TARGET) ; do \
	$(CXX) $(CXXFLAGS) $$test_file.cc -o $$test_file $(LIBRARY) $(LDFLAGS); \
	done;

test:
	$(CXX) $(CXXFLAGS) motivation1_main.cc $(LIB_SOURCES) -o test $(LDFLAGS);
	#$(CXX) $(CXXFLAGS) test_pmem_memcpy.cc $(LIB_SOURCES) -o test_pmem $(LDFLAGS);
#$(CXX) $(CXXFLAGS) batch_test_new.cc random.cc -o test $(LDFLAGS);
	
#$(CXX) $(CXXFLAGS) batch_test.cc random.cc -o test $(LDFLAGS);
	
#$(CXX) $(CXXFLAGS) persistent_skip_list_test.cc random.cc -o test $(LDFLAGS);