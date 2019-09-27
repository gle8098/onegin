main: 
	gcc -o onegin main.c onegin.c -I .

tests:
	gcc -o onegin_test tests.c onegin.c -I .

run_tests: tests
	./onegin_test
