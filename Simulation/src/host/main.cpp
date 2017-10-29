#include <iostream>

#include "host/elements.hpp"

int main(int argc, char* argv[]) {
	// Get the element simulation singleton

	Elements::Simulation& sim = Elements::Simulation::get();

	// Simulation cell synchronisation test
	
	Elements::SimCell& c = sim(0, 0);
	Elements::ELE_ENM e = (Elements::ELE_ENM)(c.ele);
	c.ele = Elements::ELE_FIRE;

	// Wait for user input, then exit program

	system("pause");

	return 0;
}