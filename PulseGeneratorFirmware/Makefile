SOURCES = pulseStateMachine.o
TEST_SOURCES = pulseStateMachine_test.o

CXXFLAGS += -Wall -Werror

test: run_tests
	./run_tests

clean:
	rm -rf $(SOURCES)
	rm -rf $(TEST_SOURCES)
	rm -rf run_tests

run_tests: $(SOURCES) $(TEST_SOURCES)
	$(CXX) $(CXXFLAGS) -o run_tests $^

pulseStateMachine.o pulseStateMachine_test.o : pulseStateMachine.h
