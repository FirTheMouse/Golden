#include<util/util.hpp>
#include<core/type.hpp>

int main() {
    g_ptr<Type> t = make<Type>();
    t->add_column(4);
    t->add_column(8);
    print(t->type_to_string(4));
    return 0;
}