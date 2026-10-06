int main()
{
	try
	{
		runner.register_test<alpha_test>();

		code = runner.execute();
	}
}
