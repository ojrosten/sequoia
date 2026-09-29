int main()
{
    runner.register_test<alpha_test>();
#ifndef _MSC_VER
    runner.register_test<beta_test>();
#endif

    code = runner.execute();
}
