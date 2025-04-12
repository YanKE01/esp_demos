#include <stdio.h>
#include <iostream>
#include <vector>
#include <boost/signals2/signal.hpp>
#include <boost/signals2/shared_connection_block.hpp>

struct Hello {
    void operator()() const
    {
        std::cout << "Hello" << std::endl;
    }
};

struct World {
    void operator()() const
    {
        std::cout << "World" << std::endl;
    }
};

void printf_args(float a, float b)
{
    std::cout << "a: " << a << ", b: " << b << std::endl;
}

void printf_add_args(float a, float b)
{
    std::cout << "a + b: " << a + b << std::endl;
}

float add_args(float a, float b)
{
    std::cout << "a + b: " << std::endl;
    return a + b;
}

float sub_args(float a, float b)
{
    std::cout << "a - b: " << std::endl;
    return a - b;
}

struct collect_results {
    using result_type = std::vector<float>;

    template<typename T>
    result_type operator()(T first, T last) const
    {
        result_type results;
        for (; first != last; ++first) {
            results.push_back(*first);
        }
        return results;
    }
};

extern "C" void app_main(void)
{
    // void slot
    boost::signals2::signal<void ()> sig;
    Hello hello;
    World world;
    boost::signals2::connection c1 = sig.connect(1, hello);
    boost::signals2::connection c2 = sig.connect(0, world);
    sig();

    // float slot
    boost::signals2::signal<void (float, float)> sig2;
    sig2.connect(&printf_args);
    sig2.connect(&printf_add_args);
    sig2(1.0f, 2.0f);

    // float slot with return value
    boost::signals2::signal<float (float, float), collect_results> sig3;
    sig3.connect(&add_args);
    sig3.connect(&sub_args);
    std::vector<float> results = sig3(1.0f, 2.0f);

    std::cout << "sig3(1.0f, 2.0f):\n";
    for (float r : results) {
        std::cout << r << std::endl;
    }

    // block slot
    sig();
    {
        boost::signals2::shared_connection_block block(c1);
        sig();
    }
    sig();
}
