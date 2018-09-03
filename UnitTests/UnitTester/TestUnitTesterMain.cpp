#include "UnitTestUtils.hpp"

#include <stdexcept>
#include <functional>
#include <limits>
#include <cmath>


double testFn(double a, double b)
{
	return a + b;
}

void testThrow()
{
	throw std::runtime_error("useful message");
}


struct testException
{
	
};

void testThrow_2()
{
	throw testException();
}

int main()
{
	using namespace UnitTesting;
	
	int numFailures = 0,
	    numChecks   = 0;
	
	check_equality<int>(1, 2);
	++numFailures;
	++numChecks;
	
	check_equality<double>(1.0, 1.0);
	++numChecks;
	
	check_equality_within_tolerance(1.0, 1.1, 0.5);
	++numChecks;
	
	check_equality_within_tolerance(1.0, 1.1, 0.09);
	++numFailures;
	++numChecks;
		
	check_exception_thrown<std::runtime_error>(std::bind(testFn, 1.0, 1.0));
	++numChecks;
	
	check_exception_thrown<std::runtime_error>(testThrow);
	++numFailures;
	++numChecks;
	
	check_exception_thrown<std::out_of_range>(testThrow);
	++numFailures;
	++numChecks;
	
	check_exception_thrown<std::runtime_error>(testThrow_2);
	++numFailures;
	++numChecks;
	
	check_equality<std::string>("hello world", "hullo world");
	++numFailures;
	++numChecks;
	
	uint checkDiscrepancy = fabs(numChecks - UnitTestLogger::instance().getNumChecks());
	if(checkDiscrepancy)
	{
		std::cout << numChecks << " checks expected; " << UnitTestLogger::instance().getNumChecks() << " logged";	
	}
	
	uint checkFailures = fabs(numFailures - UnitTestLogger::instance().getNumFailures());
	if(checkFailures)
	{
		std::cout << numFailures << " failures expected; " << UnitTestLogger::instance().getNumFailures() << " logged";
	}
	
	return checkDiscrepancy + checkFailures;
}
