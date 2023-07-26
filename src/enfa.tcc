#ifndef ENFA_TCC
#define ENFA_TCC

#include <fstream>
#include <set>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <vector>

template<typename Alpha, class Alpha_Hash = std::hash<Alpha>>
class enfa {
private:
    std::size_t Q; // state space

    std::vector<Alpha> Sigma; // alphabet
    std::unordered_map<Alpha, std::size_t, Alpha_Hash> Sigma_index;

    std::size_t q0, qf;

    std::vector<std::vector<std::set<std::size_t>>> delta;

private:
    void add_state(std::size_t q)
    {
        if (q >= Q) {
            for (std::size_t i = Q; i <= q; ++i)
                delta.push_back(std::vector<std::set<std::size_t>>(Sigma.size()));
            Q = q + 1;
        }
    }
    void add_letter(Alpha a)
    {
        if (Sigma_index.count(a) == 0) {
            Sigma_index[a] = Sigma.size();
            Sigma.push_back(a);
            std::size_t S = Sigma.size();
            for (std::size_t i = 0; i < Q; ++i)
                delta[i].resize(S);
        }
    }

public:
    enfa()
        : Q(0), q0(-1), qf(-1)
    {
        add_letter(Alpha());
    }

    void add_transition(std::size_t q1, Alpha a, std::size_t q2)
    {
        add_state(q1);
        add_state(q2);
        add_letter(a);

        delta[q1][Sigma_index[a]].insert(q2);
    }
    void add_epsilon_transition(std::size_t q1, std::size_t q2)
    {
        add_state(q1);
        add_state(q2);

        delta[q1][0].insert(q2);
    }
    void mark_start_state(std::size_t q)
    {
        add_state(q);
        q0 = q;
    }
    void mark_finish_state(std::size_t q)
    {
        add_state(q);
        qf = q;
    }

    bool accept(std::vector<Alpha> const& str)
    {
        if (q0 + 1 == 0)
            throw std::domain_error("\u03b5-NFA start state not defined");
        if (qf + 1 == 0)
            return false;
        std::set<std::size_t> curr({ q0 });

        for (auto const& alpha : str) {
            std::size_t a = Sigma_index[alpha];
            std::set<std::size_t> next;
            for (std::size_t q : curr) {
                auto const& s = delta[q][a];
                next.insert(s.begin(), s.end());
            }
            while (curr != next) {
                curr = next;
                for (std::size_t q : curr) {
                    auto const& s = delta[q][0];
                    next.insert(s.begin(), s.end());
                }
            }
        }

        return (curr.count(qf) == 1);
    }

    void create_dot_file(char const* filename) const
    {
        std::ofstream f(filename);

        f << "digraph {\n";
        if (q0 + 1 != 0) {
            f << "\tn0 [label=\"\", shape=none, height=0, width=0]\n";
            f << "\tn0 -> " << q0 << " [shape=doublecircle]\n";
        }
        if (qf + 1 != 0)
            f << "\t" << qf << " [shape=doublecircle]\n";
        for (std::size_t q1 = 0; q1 < Q; q1++) {
            auto const& s = delta[q1][0];
            auto it = s.begin();
            if (it != s.end()) {
                f << "\t" << q1 << " -> ";
                for (; std::next(it) != s.end(); ++it)
                    f << *it << ",";
                f << *it << " [label=\"\u03b5\"]\n";
            }
            for (std::size_t a = 1; a < Sigma.size(); ++a) {
                auto const& s = delta[q1][a];
                it = s.begin();
                if (it != s.end()) {
                    f << "\t" << q1 << " -> ";
                    for (; std::next(it) != s.end(); ++it)
                        f << *it << ",";
                    f << *it << " [label=\"" << Sigma[a] << "\"]\n";
                }
            }
        }
        f << "}\n";
    }
};

#endif // ENFA_TCC
