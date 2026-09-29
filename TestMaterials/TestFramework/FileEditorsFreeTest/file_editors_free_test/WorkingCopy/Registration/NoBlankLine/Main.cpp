int main()
{
    runner.register_test<alpha_test>();
    runner.register_test<beta_test>();
    code = runner.execute();
}
