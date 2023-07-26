#include "enfa.tcc"

#include <cstdlib>
#include <iostream>

int main()
{
    enfa<char> e;

    while (true) {
        //         std::cout << "1) transition 2) \u03b5-transition 3) start_state 4) finish_state 5) create_dot_file 6) check_membership 7) exit\n";
        int choice;
        std::cin >> choice;

        std::size_t q1, q2;
        char c;
        std::vector<char> str;

        switch (choice) {
        case 1:
            std::cin >> q1 >> c >> q2;
            e.add_transition(q1, c, q2);
            break;
        case 2:
            std::cin >> q1 >> q2;
            e.add_epsilon_transition(q1, q2);
            break;
        case 3:
            std::cin >> q1;
            e.mark_start_state(q1);
            break;
        case 4:
            std::cin >> q1;
            e.mark_finish_state(q1);
            break;
        case 5:
            e.create_dot_file("test.dot");
            if (std::system("dot -Tsvg -o test.svg test.dot"))
                exit(2);
            break;
        case 6:
            std::cin >> q1;
            str.resize(q1);
            for (std::size_t i = 0; i < q1; ++i)
                std::cin >> str[i];
            std::cout << (e.accept(str) ? "yes" : "no") << '\n';
            break;
        case 7:
            return 0;
        default:
            std::cout << "Bad command\n";
        }
    }
}
