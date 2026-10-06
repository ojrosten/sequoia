int main()
{
    runner.register_test<alpha_test>();
    runner.register_test<beta_test>();
    runner.register_test<gamma_test>();

    code = runner.execute();
}
