int main()
{
    runner.register_test<alpha_test>();
    // runner.register_test<gamma_test>();
    my_runner.register_test<gamma_test>();
    runner.register_test<gamma_test>();

    code = runner.execute();
}
