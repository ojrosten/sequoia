int main()
{
  runner.register_test<alpha_test>(state);
  runner.register_test<gamma_test>();

  code = runner.execute();
}
