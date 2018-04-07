#include <iostream>
#include <string>
#include "DatabaseAccess.h"
#include "MemoryAccess.h"
#include "AlbumManager.h"

#define MEMORY_MODE 0
#define SQL_MODE 1
#define MODE SQL_MODE

int getCommandNumberFromUser()
{
	std::string message("\nPlease enter any command(use number): ");
	std::string numericStr("0123456789");
	
	std::cout << message << std::endl;
	std::string input;
	std::getline(std::cin, input);
	
	while (std::cin.fail() || std::cin.eof() || input.find_first_not_of(numericStr) != std::string::npos) {

		std::cout << "Please enter a number only!" << std::endl;

		if (input.find_first_not_of(numericStr) == std::string::npos) {
			std::cin.clear();
		}

		std::cout << std::endl << message << std::endl;
		std::getline(std::cin, input);
	}
	
	return std::atoi(input.c_str());
}

int main(void)
{
	// initialization data access
	IDataAccess* dataAccess = nullptr;

	switch(MODE)
	{
	case MEMORY_MODE:
		dataAccess = new MemoryAccess;
		break;
	case SQL_MODE:
		dataAccess = new DatabaseAccess;
		break;
	default:
		dataAccess = new MemoryAccess;
		break;
	}

	// initialize album manager
	AlbumManager albumManager(*dataAccess);
	time_t now = time(nullptr);

	std::string albumName;
	std::cout << "Welcome to Gallery!" << std::endl;
	std::cout << "  Made by Barr Kirel" << std::endl;
	std::cout << "  " <<std::put_time(localtime(&now), "%d/%m/%Y %H:%M:%S") << std::endl;
	std::cout << "===================" << std::endl;
	std::cout << "Type " << HELP << " to a list of all supported commands" << std::endl;
	
	do {
		int commandNumber = getCommandNumberFromUser();
		
		try	{
			albumManager.executeCommand(static_cast<CommandType>(commandNumber));
		} catch (std::exception& e) {	
			std::cout << e.what() << std::endl;
		}
	} 
	while (true);
	delete dataAccess;
}


