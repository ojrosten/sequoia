int main()
{
    runner.register_test<alpha_test>();
#ifndef _MSC_VER
    runner.register_test<beta_test>();
#endif
    runner.register_test<gamma_test>();

    code = runner.execute();
}
