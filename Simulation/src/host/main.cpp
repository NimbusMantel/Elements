#include <iostream>

#include "host/elements.hpp"

int main(int argc, char* argv[]) {
	// Get the element simulation singleton

	Elements::Simulation* sim = Elements::Simulation::get();

	// Wait for user input, then exit program

	system("pause");

	return 0;
}