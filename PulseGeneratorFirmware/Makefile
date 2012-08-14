SOURCES = pulseStateMachine.o
TEST_SOURCES = pulseStateMachine_test.o


test: run_tests
	./run_tests

run_tests: $(SOURCES) $(TEST_SOURCES)
	$(CXX) $(CXXFLAGS) -o run_tests $^

pulseStateMachine.o pulseStateMachine_test.o : pulseStateMachine.h
