#include<linking/one.hpp>
#include<linking/two.hpp>

one::one() {

}

void one::take_two() {
    twos.push(make<two>());
    twos[0]->say_o();
}