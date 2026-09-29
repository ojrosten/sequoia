int main()
{
  runner.register_test<utilities_free_test>();
  runner.register_test<utilities_free_test_extras>();
  runner.register_test<widget_test>();
  runner.execute();
}
