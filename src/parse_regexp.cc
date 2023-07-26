#include "enfa.tcc"

#include <iostream>
#include <memory>
#include <string>

#ifdef DEBUG
void dbg(std::string s, enfa<char> const& e)
{
    std::cout << s << '\n';
    e.create_dot_file((s + ".dot").c_str());
    if (std::system((std::string("neato -Tsvg -o '") + s + ".svg' '" + s + ".dot'").c_str()))
        std::cout << s << " file failed\n";
}
#endif

enfa<char> parse_regexp(std::string s);

static enfa<char> parse_regexp_atom(std::string s, std::size_t& l)
{
#ifdef DEBUG
    std::string s_out;
#endif

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
#ifdef DEBUG
        s_out = s.substr(l, r - l);
#endif
        l = r + 1;
    } else {
        e = enfa<char>::letter_enfa(s[l]);
#ifdef DEBUG
        s_out = s.substr(l, 1);
#endif
        l++;
    }
    if (s[l] == '*') {
        e = enfa<char>::kleene_star_enfa(e);
#ifdef DEBUG
        s_out += "*";
#endif
        l++;
    }
#ifdef DEBUG
    dbg(s_out, e);
#endif
    return e;
}

static enfa<char> parse_regexp_concat(std::string s, std::size_t& i)
{
    if (s.length() == 0)
        return enfa<char>::empty_expr_enfa();
#ifdef DEBUG
    std::size_t i_init = i;
#endif
    enfa<char> e = parse_regexp_atom(s, i);
    while (i < s.length() && s[i] != '|')
        e = enfa<char>::concat_enfa(e, parse_regexp_atom(s, i));
#ifdef DEBUG
    s = s.substr(i_init, i - i_init - 1);
    dbg(s, e);
#endif
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
#ifdef DEBUG
    dbg(s, e);
#endif
    return e;
};
