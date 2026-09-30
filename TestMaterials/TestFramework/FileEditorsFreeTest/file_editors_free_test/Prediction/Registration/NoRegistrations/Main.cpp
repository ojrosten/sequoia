int main()
{
	try
	{
		sequoia::testing::test_runner runner{};
		runner.register_test<gamma_test>();

		code = runner.execute();
	}
}
