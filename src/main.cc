#include "enfa.tcc"

#include <iostream>
#include <string>

enfa<char> parse_regexp(std::string s);

int main()
{
    std::string s;

    std::cout << "Enter regex\n";
    std::cin >> s;

    enfa<char> e = parse_regexp(s);

    std::cout << s << '\n';
    e.create_dot_file((s + ".dot").c_str());
    if (std::system((std::string("neato -Tsvg -o '") + s + ".svg' '" + s + ".dot'").c_str()))
        std::cout << s << " file failed\n";

    std::cout << "Check\n";
    while (true) {
        std::cin >> s;
        std::vector<char> s_vec(s.length());
        for (std::size_t i = 0; i < s.length(); ++i)
            s_vec[i] = s[i];
        std::cout << "'" << s << "': " << (e.accept(s_vec) ? "yes" : "no") << '\n';
    }
}
