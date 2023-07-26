#include "enfa.tcc"

#include <iostream>
#include <memory>
#include <string>

enfa<char> parse_regexp(std::string s);

static enfa<char> parse_regexp_atom(std::string s, std::size_t& l)
{
    enfa<char> e;
    if (s[l] == '(') {
        int count = 1;
        l++;
        std::size_t r = l;
        for (; r < s.length(); ++r) {
            if (s[r] == ')')
                --count;
            else if (s[r] == '(')
                ++count;
            if (count == 0)
                break;
        }
        e = parse_regexp(s.substr(l, r - l));
        l = r + 1;
    } else {
        e = enfa<char>::letter_enfa(s[l]);
        l++;
    }
    if (s[l] == '*') {
        e = enfa<char>::kleene_star_enfa(e);
        l++;
    }
    return e;
}

static enfa<char> parse_regexp_concat(std::string s, std::size_t& i)
{
    if (s.length() == 0)
        return enfa<char>::empty_expr_enfa();

    std::size_t i_init = i;

    enfa<char> e = parse_regexp_atom(s, i);
    while (i < s.length() && s[i] != '|')
        e = enfa<char>::concat_enfa(e, parse_regexp_atom(s, i));

    s = s.substr(i_init, i - i_init);

    return e;
}

enfa<char> parse_regexp(std::string s)
{
    if (s.length() == 0)
        return enfa<char>::empty_expr_enfa();

    std::size_t i = 0;
    enfa<char> e = parse_regexp_concat(s, i);
    ++i;
    while (i < s.length()) {
        e = enfa<char>::union_enfa(e, parse_regexp_concat(s, i));
        ++i;
    }
    return e;
}
